// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "runner.h"
#include "extractor.h"
#include "pal.h"
#include "trace.h"
#include "utils.h"

using namespace bundle;

// Compute the final extraction location as:
// m_extraction_dir = $DOTNET_BUNDLE_EXTRACT_BASE_DIR/<app>/<id>/...
//
// If DOTNET_BUNDLE_EXTRACT_BASE_DIR is not set in the environment, the 
// base directory defaults to $TMPDIR/.net
void bundle_runner_t::determine_extraction_dir()
{
    if (!pal::getenv(_X("DOTNET_BUNDLE_EXTRACT_BASE_DIR"), &m_extraction_dir))
    {
        if (!pal::get_temp_directory(m_extraction_dir))
        {
            trace::error(_X("Failure processing application bundle."));
            trace::error(_X("Failed to determine location for extracting embedded files"));
            throw StatusCode::BundleExtractionFailure;
        }

        append_path(&m_extraction_dir, _X(".net"));
    }

    pal::string_t host_name = strip_executable_ext(get_filename(m_bundle_path));
    append_path(&m_extraction_dir, host_name.c_str());
    append_path(&m_extraction_dir, bundle_id().c_str());

    trace::info(_X("Files embedded within the bundled will be extracted to [%s] directory"), m_extraction_dir.c_str());
}

// Compute the worker extraction location for this process, before the 
// extracted files are committed to the final location
// m_working_extraction_dir = $DOTNET_BUNDLE_EXTRACT_BASE_DIR/<app>/<proc-id-hex>
void bundle_runner_t::create_working_extraction_dir()
{
    // Set the working extraction path
    m_working_extraction_dir = get_directory(m_extraction_dir);
    pal::char_t pid[32];
    pal::snwprintf(pid, 32, _X("%x"), pal::get_pid());
    append_path(&m_working_extraction_dir, pid);

    create_directory_tree(m_working_extraction_dir);

    trace::info(_X("Temporary directory used to extract bundled files is [%s]"), m_working_extraction_dir.c_str());
}

// Create a file to be extracted out on disk, including any intermediate sub-directories.
FILE* bundle_runner_t::create_extraction_file(const pal::string_t& relative_path)
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
        trace::error(_X("Failure processing application bundle."));
        trace::error(_X("Failed to open file [%s] for writing"), file_path.c_str());
        throw StatusCode::BundleExtractionIOError;
    }

    return file;
}

// Extract one file from the bundle to disk.
void bundle_runner_t::extract_file(const file_entry_t& entry)
{
    FILE* file = create_extraction_file(entry.relative_path());
    const int64_t buffer_size = 8 * 1024; // Copy the file in 8KB chunks
    uint8_t buffer[buffer_size];
    int64_t file_size = entry.size();

    seek(m_bundle_stream, entry.offset(), SEEK_SET);
    do {
        int64_t copy_size = (file_size <= buffer_size) ? file_size : buffer_size;
        read(buffer, copy_size, m_bundle_stream);
        write(buffer, copy_size, file);
        file_size -= copy_size;
    } while (file_size > 0);

    fclose(file);
}

bool bundle_runner_t::can_reuse_extraction()
{
    // In this version, the extracted files are assumed to be 
    // correct by construction.
    // 
    // Files embedded in the bundle are first extracted to m_working_extraction_dir
    // Once all files are successfully extracted, the extraction location is 
    // committed (renamed) to m_extraction_dir. Therefore, the presence of 
    // m_extraction_dir means that the files are pre-extracted. 


    return pal::directory_exists(m_extraction_dir);
}

void extractor_t::begin()
{
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

     determine_working_extraction_dir();
}

 void extractor_t::commit()
{
    // Commit files to the final extraction directory
    if (pal::rename(m_working_extraction_dir.c_str(), m_extraction_dir.c_str()) != 0)
    {
        if (can_reuse_extraction())
        {
            // Another process successfully extracted the dependencies

             trace::info(_X("Extraction completed by another process, aborting current extracion."));

             util_t::remove_directory_tree(m_working_extraction_dir);
        }

        trace::error(_X("Failure processing application bundle."));
        trace::error(_X("Failed to commit extracted to files to directory [%s]"), m_extraction_dir.c_str());
        throw StatusCode::BundleExtractionFailure;
    }
}