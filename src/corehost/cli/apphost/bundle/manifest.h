// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __MANIFEST_H__
#define __MANIFEST_H__

#include <list>
#include "file_entry.h"

namespace bundle
{
    // Manifest Header contains:
    // Fixed size thunk (represened by manifest_header_fixed_t)
    //   - Major Version     
    //   - Minor Version     
    //   - Number of embedded files
    // Variable size portion:
    //   - Bundle ID length 
    //   - Bundle ID ("Bundle ID length" bytes)

#pragma pack(push, 1)
    struct manifest_header_fixed_t
    {
        uint32_t major_version;
        uint32_t minor_version;
        int32_t num_embedded_files;
    };
#pragma pack(pop)

    struct manifest_header_t
    {
    public:
        manifest_header_t()
            :m_data(nullptr), m_bundle_id()
        {
        }

        bool is_valid();
        static manifest_header_t* read(bundle_reader_t &reader);
        const pal::string_t& bundle_id() { return m_bundle_id; }
        int32_t num_embedded_files() { return m_data->num_embedded_files;  }

    private:
        manifest_header_fixed_t* m_data;
        pal::string_t m_bundle_id;

        static const uint32_t m_current_major_version = 0;
        static const uint32_t m_current_minor_version = 1;
    };

    // Manifest Footer contains:
    //   Manifest header offset
    //   Length-prefixed non-null terminated Bundle Signature ".NetCoreBundle"
#pragma pack(push, 1)
    struct manifest_footer_t
    {
        manifest_footer_t()
            :m_header_offset(0), m_signature_length(0)
        {
            m_signature[0] = 0;
        }

        bool is_valid();
        static manifest_footer_t* read(bundle_reader_t &reader);
        int64_t manifest_header_offset() { return m_header_offset; }

    private:
        int64_t m_header_offset;
        uint8_t m_signature_length;
        char m_signature[14];

    private:

        static const char* m_expected_signature;
    };
#pragma pack(pop)


    // Bundle Manifest contains:
    //     Series of file entries (for each embedded file)

    class manifest_t
    {
    public:
        manifest_t()
            :files()
        {}

        std::list<file_entry_t*> files;

        static manifest_t* read(bundle_reader_t &reader, int32_t num_files);
    };
}
#endif // __MANIFEST_H__
