// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __RUNNER_H__
#define __RUNNER_H__

#include "manifest.h"
#include "error_codes.h"

namespace bundle
{
    class runner_t
    {
    public:
        static StatusCode extract();

        static pal::string_t bundle_path;

        static pal::string_t extraction_path()
        {
            return s_extraction_path;
        }

        static bool read_bundled_file(const pal::char_t* name, const void** buffer, size_t* size);

    private:
        static void map_host();

        static void process_manifest_footer(reader_t &reader);
        static void process_manifest_header(reader_t &reader);

        static manifest_t* s_manifest;
        static int32_t s_num_embedded_files;
        static pal::string_t s_bundle_id;
        static pal::string_t s_extraction_path;
        static int8_t* s_bundle_map;
        static size_t s_bundle_length;
    };
}

#endif // __RUNNER_H__
