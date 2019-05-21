// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __FILE_ENTRY_H__
#define __FILE_ENTRY_H__

#include "file_type.h"
#include "pal.h"

namespace bundle
{

    // FileEntry: Records information about embedded files.
    // 
    // The bundle manifest records the following meta-data for each 
    // file embedded in the bundle:
    // Fixed size portion (represented by file_entry_inner_t)
    //   - Offset     
    //   - Size       
    //   - File Entry Type       
    // Variable Size portion
	//   - path-length    (7-bit extension encoding)
	//   - relative path  ("path-length" Bytes)

#pragma pack(push, 1)
	struct
	{
		int64_t offset;
		int64_t size;
		file_type_t type;
	} file_entry_fixed_t;
#pragma pack(pop)


    class file_entry_t
    {
    public:
        file_entry_t()
            :m_relative_path()
        {
        }

        const pal::string_t& relative_path() { return m_relative_path; }
        int64_t offset() { return m_offset; }
        int64_t size() { return m_size; }
        file_type_t type() { return m_type; }

        static file_entry_t* read(int8_t* ptr);

    private:
		int64_t m_offset;
		int64_t m_size;
		file_type_t m_type;
		pal::string_t m_relative_path; // Path of an embedded file, relative to the extraction directory.
		static const pal::char_t bundle_dir_separator = '/';
        bool is_valid();
    };
}
#endif // __FILE_ENTRY_H__
