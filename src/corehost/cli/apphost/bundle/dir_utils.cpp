// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "bundle_runner.h"
#include "pal.h"
#include "trace.h"
#include "utils.h"

using namespace bundle;


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

    if (!pal::mkdir(path.c_str(), 0700)) // Owner - rwx
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

static void remove_directory_tree(const pal::string_t& path)
{
    if (path.empty())
    {
        return;
    }

    std::vector<pal::string_t> dirs;
    pal::readdir_onlydirectories(path, &dirs);

    for (const pal::string_t &dir : dirs)
    {
        remove_directory_tree(dir);
    }

    std::vector<pal::string_t> files;
    pal::readdir(path, &files);

    for (const pal::string_t &file : files)
    {
        if (!pal::remove(file.c_str()))
        {
            trace::warning(_X("Failed to remove temporary file [%s]."), file.c_str());
        }
    }

    if (!pal::rmdir(path.c_str()))
    {
        trace::warning(_X("Failed to remove temporary directory [%s]."), path.c_str());
    }
}
