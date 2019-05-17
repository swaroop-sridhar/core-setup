// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "bundle_util.h"
#include "error_codes.h"

using namespace bundle;

// Handle the relatively uncommon scenario where the bundle ID or 
// the relative-path of a file within the bundle is longer than 127 bytes
size_t bundle_util_t::get_path_length(int8_t **pptr)
{
    size_t length = 0;

	int8_t first_byte = *((*pptr)++);
    // If the high bit is set, it means there are more bytes to read.
    if ((first_byte & 0x80) == 0)
    {
         length = first_byte;
    }
    else
    {
        int8_t second_byte = *((*pptr)++);

        if (second_byte & 0x80)
        {
            // There can be no more than two bytes in path_length
            trace::error(_X("Failure processing application bundle; possible file corruption."));
            trace::error(_X("Path length encoding read beyond two bytes"));

            throw StatusCode::BundleExtractionFailure;
        }

        length = (second_byte << 7) | (first_byte & 0x7f);
    }

    if (length <= 0 || length > PATH_MAX)
    {
        trace::error(_X("Failure processing application bundle; possible file corruption."));
        trace::error(_X("Path length is zero or too long"));
        throw StatusCode::BundleExtractionFailure;
    }

    return length;
}

void bundle_util_t::read_path_string(pal::string_t &str, int8_t **pptr)
{
	size_t length = get_path_length(pptr);
    uint8_t *buffer = new uint8_t[length + 1];
	memcpy(buffer, *pptr, length);
	*pptr += length;
    buffer[length] = 0; // null-terminator
    pal::clr_palstring(reinterpret_cast<const char*>(buffer), &str);
}

bool bundle_util_t::has_dirs_in_path(const pal::string_t& path)
{
    return path.find_last_of(DIR_SEPARATOR) != pal::string_t::npos;
}

void bundle_util_t::create_directory_tree(const pal::string_t &path)
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

        trace::error(_X("Failure processing application bundle."));
        trace::error(_X("Failed to create directory [%s] for extracting bundled files"), path.c_str());
        throw StatusCode::BundleExtractionIOError;
    }
}

void bundle_util_t::remove_directory_tree(const pal::string_t& path)
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
