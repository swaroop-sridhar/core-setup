// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifdef FEATURE_APPHOST

#include "bdl_processor.h"
#include "pal.h"
#include "trace.h"
#include "utils.h"

void bdl_processor_t::seek(long offset, int origin)
{
	if (fseek(m_bundle, offset, origin) != 0)
	{
		trace::error(_X("Bundle extraction: Seek failure."));
		throw StatusCode::BundleExtractionIOError;
	}
}

void bdl_processor_t::write(const void* buf, size_t size, FILE *stream)
{
	if (fwrite(buf, 1, size, stream) != size)
	{
		trace::error(_X("Bundle extraction: Write failure."));
		throw StatusCode::BundleExtractionIOError;
	}
}

void bdl_processor_t::read(void* buf, size_t size, FILE* stream)
{
	if (fread(buf, 1, size, stream) != size)
	{
		trace::error(_X("Bundle extraction: Read failure."));
		throw StatusCode::BundleExtractionIOError;
	}
}

// Read a non-null terminated fixed length UTF8 string from a byte-stream
// and transform it to pal::string_t
void bdl_processor_t::read_string(pal::string_t &str, size_t size, FILE* stream)
{
	uint8_t *buffer = new uint8_t[size + 1]; 
	read(buffer, size, stream);
	buffer[size] = 0; // null-terminator
	pal::clr_palstring((const char*)buffer, &str);
}

static bool has_dirs_in_path(const pal::string_t& path)
{
	return path.find_last_of(DIR_SEPARATOR) != pal::string_t::npos;
}

static void create_directory_tree(const pal::string_t &path)
{
	if (path.empty())
	{
		return;
	}

	if (pal::directory_exists(path))
	{
		return;
	}

	if (has_dirs_in_path(path))
	{
		create_directory_tree(get_directory(path));
	}

	if (!pal::mkdir(path.c_str(), 0700))
	{
		if (pal::directory_exists(path))
		{
			// The directory was created since we last checked.
			return;
		}

		trace::error(_X("Failed to create directory [%s]"), path.c_str());
		throw StatusCode::BundleExtractionIOError;
	}
}

static void remove_directory_tree(const pal::string_t& path)
{
	if (path.empty())
	{
		return;
	}

	std::vector<pal::string_t> dirs;
	pal::readdir_onlydirectories(path, &dirs);

	for (pal::string_t dir : dirs)
	{
		remove_directory_tree(dir);
	}

	std::vector<pal::string_t> files;
	pal::readdir(path, &files);

	for (pal::string_t file : files)
	{
		if (!pal::remove(file.c_str()))
		{
			trace::error(_X("Error removing file [%s]"), file.c_str());
			throw StatusCode::BundleExtractionIOError;
		}
	}

	if (!pal::rmdir(path.c_str()))
	{
		trace::error(_X("Error removing directory [%s]"), path.c_str());
		throw StatusCode::BundleExtractionIOError;
	}
}

void bdl_processor_t::reopen_host_for_reading()
{
	m_bundle = pal::file_open(m_bundle_path, _X("rb"));
	if (m_bundle == nullptr)
	{
		trace::error(_X("Host file descriptor invalid"));
		throw StatusCode::BundleExtractionIOError;
	}
}

void bdl_processor_t::process_manifest_footer(int64_t& header_offset)
{
	seek(-manifest_footer_t::num_bytes_read(), SEEK_END);

	manifest_footer_t* footer = manifest_footer_t::read(m_bundle);
	header_offset = footer->header_offset;
}

void bdl_processor_t::process_manifest_header(int64_t header_offset)
{
	seek(header_offset, SEEK_SET);

	manifest_header_t* header = manifest_header_t::read(m_bundle);

	m_num_embedded_files = header->data.num_embedded_files;
	m_bundle_id = header->bundle_id;
}

// Compute the final extraction location as:
// m_extraction_dir = $DOTNET_BUNDLE_EXTRACT_BASE_DIR/<app>/<id>/...
//
// If DOTNET_BUNDLE_EXTRACT_BASE_DIR is not set in the environment, the 
// base directory defaults to $TMPDIR/.net
void bdl_processor_t::determine_extraction_dir()
{
	if (!pal::getenv(_X("DOTNET_BUNDLE_EXTRACT_BASE_DIR"), &m_extraction_dir))
	{
		if (!pal::get_temp_directory(m_extraction_dir))
		{
			trace::error(_X("Failed to get temp_dir"));
			throw StatusCode::BundleExtractionFailure;
		}

		append_path(&m_extraction_dir, _X(".net"));
	}

	pal::string_t host_name = strip_executable_ext(get_filename(m_bundle_path));
	append_path(&m_extraction_dir, host_name.c_str());
	append_path(&m_extraction_dir, m_bundle_id.c_str());

	trace::info(_X("Extraction Location [%s]"), m_extraction_dir.c_str());
}

// Compute the worker extraction location for this process, before the 
// extracted files are committed to the final location
// m_working_extraction_dir = $DOTNET_BUNDLE_EXTRACT_BASE_DIR/<app>/<proc-id-hex>
void bdl_processor_t::create_working_extraction_dir()
{
	// Set the working extraction path
	m_working_extraction_dir = get_directory(m_extraction_dir);
	pal::char_t pid[32];
	pal::snwprintf(pid, 32, _X("%x"), pal::get_pid());
	append_path(&m_working_extraction_dir, pid);

	create_directory_tree(m_working_extraction_dir);

	trace::info(_X("Temporary Extraction Location [%s]"), m_working_extraction_dir.c_str());
}

// Create a file to be extracted out on disk, including any intermediate sub-directories.
FILE* bdl_processor_t::create_extraction_file(const pal::string_t& relative_path)
{
	pal::string_t file_path = m_working_extraction_dir;
	append_path(&file_path, relative_path.c_str());

	// m_working_extraction_dir is assumed to exist, 
	// so we only create sub-directories if relative_path contains directories
	if (has_dirs_in_path(relative_path))
	{
		create_directory_tree(get_directory(file_path));
	}

	FILE* file = pal::file_open(file_path.c_str(), _X("wb"));

	if (file == nullptr)
	{
		trace::error(_X("Failed to open file [%s] for writing"), file_path.c_str());
		throw StatusCode::BundleExtractionIOError;
	}

	return file;
}

// Extract one file from the bundle to disk.
void bdl_processor_t::extract_file(file_entry_t *entry)
{
	FILE* file = create_extraction_file(entry->relative_path);
	uint8_t* buffer = new uint8_t[entry->data.size];

	seek(entry->data.offset, SEEK_SET);
	read(buffer, entry->data.size, m_bundle);
	write(buffer, entry->data.size, file);

	fclose(file);
}

bool bdl_processor_t::can_reuse_extraction()
{
	// Should the pre-extracted files be checked for validity? 
	// For example:
	//   (a) Verify that all files specified in the manifest exist, 
	//       and their content match a pre-computed hash.
	//   (a) Verify that all files specified in the manifest exist, 
	//       and are of the correct size
	//   (c) Spot checking: check that the <app.dll> exists, and is of the correct size.
	//   (d) Trust that if the extraction directory with the correct path 
	//       <base-path>/<app-nmae>/<bundle-id> exists, then the exctracted files are valid
	//
	// To optimize for startup, the check should be quick, and not involve:
	//   * Processing the entire bundle manifest, or
	//   * Opening/reading all the files in extracted directory.
	//
	// In this version, we choose option (d) above.

	return pal::directory_exists(m_extraction_dir);
}

// Current support for executing single-file bundles involves 
// extraction of embedded files to actual files on disk. 
// This method implements the file extraction functionality at startup.
//
// Files are extracted to a specific deterministic location on disk
// on first run, and are available for reuse by subsequent similar runs.
//
// The extraction should be fault tolerant with respect to:
//  * Failures/crashes during extraction which result in partial-extraction
//  * Race between two or more processes concurrently attempting extraction
//
// In order to solve these issues, we implement a extraction as a two-phase approach:
// 1) Files embedded in a bundle are extracted to a process-specific temporary
//    extraction location (m_working_extraction_dir)
// 2) Upon successful extraction, m_working_extraction_dir is renamed to the actual
//    extraction location (m_extraction_dir)
//    
// This effectively creates a file-lock to protect against races and failed extractions.

StatusCode bdl_processor_t::extract()
{
	try
	{
		// Determine if the current executable is a bundle
		reopen_host_for_reading();

		int64_t manifest_header_offset;
		process_manifest_footer(manifest_header_offset);
		process_manifest_header(manifest_header_offset);

		// Determine if embedded files are already extracted, and available for reuse
		determine_extraction_dir();
		if (can_reuse_extraction())
		{
			return StatusCode::Success;
		}

		// Extract files to temporary working directory
		create_working_extraction_dir();
		m_manifest = manifest_t::read(m_bundle, m_num_embedded_files);

		for (file_entry_t* entry : m_manifest->files) {
			extract_file(entry);
		}

		// Commit files to the final extraction directory
		if (pal::rename(m_working_extraction_dir.c_str(), m_extraction_dir.c_str()) != 0)
		{
			if (can_reuse_extraction())
			{
				// Another process successfully extracted the dependencies
				trace::info(_X("Extraction completed by another process"));

				remove_directory_tree(m_working_extraction_dir);
				return StatusCode::Success;
			}

			trace::error(_X("Failed to switch extraction directory to [%s]"), m_extraction_dir.c_str());
			throw StatusCode::BundleExtractionFailure;
		}

		fclose(m_bundle);
		return StatusCode::Success;
	}
	catch (StatusCode e)
	{
		fclose(m_bundle);
		return e;
	}
}

#endif // FEATURE_APPHOST
