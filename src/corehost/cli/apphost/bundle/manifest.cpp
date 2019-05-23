// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "bundle_runner.h"
#include "bundle_util.h"
#include "pal.h"
#include "error_codes.h"
#include "trace.h"
#include "utils.h"

using namespace bundle;

bool manifest_header_t::is_valid()
{
    return m_data->major_version == m_current_major_version &&
           m_data->minor_version == m_current_minor_version &&
           m_data->num_embedded_files > 0;
}

manifest_header_t* manifest_header_t::read(bundle_reader_t &reader)
{
    manifest_header_t* header = new manifest_header_t();

    // First read the fixed size portion of the header
	const void* data;
	reader.direct_read(data, sizeof(manifest_header_fixed_t));
	header->m_data = (manifest_header_fixed_t*)data;

    if (!header->is_valid())
    {
        trace::error(_X("Failure processing application bundle."));
        trace::error(_X("Manifest header version compatibility check failed"));

        throw StatusCode::BundleExtractionFailure;
    }

    // Next read the bundle-ID string, given its length
	reader.read_path_string(header->m_bundle_id);

    return header;
}

const char* manifest_footer_t::m_expected_signature = ".NetCoreBundle";

bool manifest_footer_t::is_valid()
{
    return m_header_offset > 0 &&
        m_signature_length == 14 &&
        strncmp(m_signature, m_expected_signature, m_signature_length) == 0;
}

manifest_footer_t* manifest_footer_t::read(bundle_reader_t &reader)
{
	const void* data;
	reader.direct_read(data, sizeof(manifest_footer_t));
	manifest_footer_t* footer = (manifest_footer_t*)data;

    if (!footer->is_valid())
    {
        trace::info(_X("This executable is not recognized as a bundle."));

        throw StatusCode::AppHostExeNotBundle;
    }

    return footer;
}

manifest_t* manifest_t::read(bundle_reader_t& reader, int32_t num_files)
{
    manifest_t* manifest = new manifest_t();

    for (int32_t i = 0; i < num_files; i++)
    {
        file_entry_t* entry = file_entry_t::read(reader);
        if (entry == nullptr)
        {
            return nullptr;
        }

        manifest->files.push_back(entry);
    }

    return manifest;
}
