// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "bundle_util.h"
#include "error_codes.h"

using namespace bundle;

void bundle_util_t::write(const void* buf, size_t size, FILE* stream)
{
	if (fwrite(buf, 1, size, stream) != size)
	{
		trace::error(_X("Failure extracting contents of the application bundle."));
		trace::error(_X("I/O failure when writing extracted files."));
		throw StatusCode::BundleExtractionIOError;
	}
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
