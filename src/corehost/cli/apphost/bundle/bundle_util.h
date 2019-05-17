// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __BUNDLE_UTIL_H__
#define __BUNDLE_UTIL_H__

#include <cstdint>
#include "pal.h"
#include "trace.h"
#include "utils.h"

namespace bundle
{
    static class bundle_util_t
    {
    public:
		static size_t get_path_length(int8_t **pptr);
		static void read_path_string(pal::string_t& str, int8_t** pptr);

		static bool has_dirs_in_path(const pal::string_t& path);
		static void remove_directory_tree(const pal::string_t& path);
		static void create_directory_tree(const pal::string_t& path);
    };
}

#endif // __BUNDLE_UTIL_H__
