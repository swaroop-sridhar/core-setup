// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __BUNDLE_UTIL_H__
#define __BUNDLE_UTIL_H__

#include <cstdint>

namespace bundle
{
    static class bundle_util_t
    {
    public:
		static void seek(FILE* stream, long offset, int origin);
		static void read(void* buf, size_t size, FILE* stream);
        static void write(const void* buf, size_t size, FILE* stream);
        static void read_string(pal::string_t& str, size_t size, FILE* stream);

		static size_t get_path_length(int8_t first_byte, FILE* stream);
		static bool has_dirs_in_path(const pal::string_t& path);
		static void remove_directory_tree(const pal::string_t& path);
		static void create_directory_tree(const pal::string_t& path);
    };
}

#endif // __BUNDLE_UTIL_H__
