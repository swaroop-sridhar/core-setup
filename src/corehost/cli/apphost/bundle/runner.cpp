// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "bundle_runner.h"
#include "pal.h"
#include "trace.h"
#include "utils.h"

using namespace bundle;

void bundle_runner_t::reopen_host_for_reading()
{
    m_bundle_stream = pal::file_open(m_bundle_path, _X("rb"));
    if (m_bundle_stream == nullptr)
    {
        trace::error(_X("Failure processing application bundle."));
        trace::error(_X("Couldn't open host binary for reading contents"));
        throw StatusCode::BundleExtractionIOError;
    }
}

// Current support for executing single-file bundles involves 
// extraction of embedded files to actual files on disk. 
// This method implements the file extraction functionality at startup.
StatusCode bundle_runner_t::extract()
{
    try
    {
        reopen_host_for_reading();

        // Read the bundle header
        seek(m_bundle_stream, marker_t::header_offset(), SEEK_SET);
        m_header = header_t::read(m_bundle_stream);

        // Determine if embedded files are already extracted, and available for reuse
        determine_extraction_dir();
        if (can_reuse_extraction())
        {
            return StatusCode::Success;
        }

        // Extract files to temporary working directory
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
        
        create_working_extraction_dir();

        m_manifest = manifest_t::read(m_bundle_stream, num_embedded_files());

        for (const file_entry_t & entry : m_manifest.files) {
            extract_file(entry);
        }

        // Commit files to the final extraction directory
        // Retry the move operation with some wait in between the attempts. This is to workaround for possible file locking
        // caused by AV software. Basically the extraction process above writes a bunch of executable files to disk
        // and some AV software may decide to scan them on write. If this happens the files will be locked which blocks
        // our ablity to move them.
        int retry_count = 500;
        while (true)
        {
            if (pal::rename(m_working_extraction_dir.c_str(), m_extraction_dir.c_str()) == 0)
                break;

            bool should_retry = errno == EACCES;
            if (can_reuse_extraction())
            {
                // Another process successfully extracted the dependencies
                trace::info(_X("Extraction completed by another process, aborting current extraction."));

                remove_directory_tree(m_working_extraction_dir);
                break;
            }

            if (should_retry && (retry_count--) > 0)
            {
                trace::info(_X("Retrying extraction due to EACCES trying to rename the extraction folder to [%s]."), m_extraction_dir.c_str());
                pal::sleep(100);
                continue;
            }
            else
            {
                trace::error(_X("Failure processing application bundle."));
                trace::error(_X("Failed to commit extracted files to directory [%s]"), m_extraction_dir.c_str());
                throw StatusCode::BundleExtractionFailure;
            }
        }

        fclose(m_bundle_stream);
        return StatusCode::Success;
    }
    catch (StatusCode e)
    {
        fclose(m_bundle_stream);
        return e;
    }
}
