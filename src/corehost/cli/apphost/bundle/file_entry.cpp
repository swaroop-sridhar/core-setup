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

bool file_entry_t::is_valid()
{
    return m_offset > 0 && m_size > 0 &&
        static_cast<file_type_t>(m_type) < file_type_t::__last;
}

file_entry_t* file_entry_t::read(bundle_reader_t &reader)
{
	// First read the fixed-sized portion of file-entry
	file_entry_fixed_t* fixed_data = (file_entry_fixed_t*)reader.direct_read(sizeof(file_entry_fixed_t));
    file_entry_t* entry = new file_entry_t(fixed_data);

    if (!entry->is_valid())
    {
        trace::error(_X("Failure processing application bundle; possible file corruption."));
        trace::error(_X("Invalid FileEntry detected."));
        throw StatusCode::BundleExtractionFailure;
    }

	pal::string_t &path = entry->m_relative_path;
	reader.read_path_string(path);

    // Fixup the relative-path to have current platform's directory separator.
    if (bundle_dir_separator != DIR_SEPARATOR)
    {
        for (size_t pos = path.find(bundle_dir_separator);
            pos != pal::string_t::npos;
            pos = path.find(bundle_dir_separator, pos))
        {
            path[pos] = DIR_SEPARATOR;
        }
    }

    return entry;
}


