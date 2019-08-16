// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __BUNDLE_RUNNER_H__
#define __BUNDLE_RUNNER_H__


#include <cstdint>
#include <memory>
#include "header.h"
#include "manifest.h"
#include "marker.h"
#include "error_codes.h"

namespace bundle
{
    class bundle_runner_t
    {
    public:
        static void seek(FILE* stream, long offset, int origin);
        static void read(void* buf, size_t size, FILE* stream);
        static void write(const void* buf, size_t size, FILE* stream);
        static size_t get_path_length(int8_t first_byte, FILE* stream);
        static void read_string(pal::string_t& str, size_t size, FILE* stream);
    };

}

#endif // __BUNDLE_RUNNER_H__
