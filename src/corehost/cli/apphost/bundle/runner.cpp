// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "runner.h"
#include "extractor.h"
#include "pal.h"
#include "trace.h"
#include "utils.h"

using namespace bundle;

manifest_t* runner_t::s_manifest = nullptr;
int32_t runner_t::s_num_embedded_files = 0;
pal::string_t runner_t::bundle_path;
pal::string_t runner_t::s_bundle_id;
pal::string_t runner_t::s_extraction_path;
int8_t* runner_t::s_bundle_map = nullptr;
size_t runner_t::s_bundle_length = 0;

void runner_t::map_host()
{
    s_bundle_map = (int8_t *) pal::map_file_readonly(bundle_path, s_bundle_length);

    if (s_bundle_map == nullptr)
    {
        trace::error(_X("Failure processing application bundle."));
        trace::error(_X("Couldn't memory map the bundle file for reading"));
        throw StatusCode::BundleExtractionIOError;
    }
}

void runner_t::process_manifest_footer(reader_t &reader)
{
    reader.set_offset(s_bundle_length - sizeof(manifest_footer_t));
    manifest_footer_t* footer = manifest_footer_t::read(reader);

    if (!footer->is_valid())
    {
        trace::info(_X("This executable is not recognized as a bundle."));
        throw StatusCode::AppHostExeNotBundle;
    }

    reader.set_offset(footer->manifest_header_offset());
}

void runner_t::process_manifest_header(reader_t &reader)
{
    manifest_header_t* header = manifest_header_t::read(reader);

    s_num_embedded_files = header->num_embedded_files();
    s_bundle_id = header->bundle_id();
}

bool runner_t::read_bundled_file(const char* name, const void** buffer, size_t* size)
{
    for (file_entry_t* entry : s_manifest->files) 
    {
        pal::string_t s = name;
        
        if (strcmp(entry->relative_path().data(), name) == 0)
        {
            *buffer = s_bundle_map + entry->offset;
            size = entry->size;
            return true;
        }
    }

    return false;
}

// Current support for executing single-file bundles involves 
// extraction of embedded files to actual files on disk. 
// This method implements the file extraction functionality at startup.
StatusCode runner_t::extract()
{
    try
    {
        // Determine if the current executable is a bundle
        map_host();

        //  If the current AppHost is a bundle, it's layout will be 
        //    AppHost binary 
        //    Embedded Files: including the app, its configuration files, 
        //                    dependencies, and possibly the runtime.
        //    Bundle Manifest

        reader_t reader(s_bundle_map);
        process_manifest_footer(reader);
        process_manifest_header(reader);

        extractor_t extractor(s_bundle_id, bundle_path);
        s_extraction_path = extractor.extraction_dir();

        // Determine if embedded files are already extracted, and available for reuse
        if (extractor.can_reuse_extraction())
        {
            return StatusCode::Success;
        }

        s_manifest = manifest_t::read(reader, s_num_embedded_files);

        extractor.begin();
        for (file_entry_t* entry : s_manifest->files) {
            extractor.extract(entry, reader);
        }
        extractor.commit();

        return StatusCode::Success;
    }
    catch (StatusCode e)
    {
        return e;
    }
}
