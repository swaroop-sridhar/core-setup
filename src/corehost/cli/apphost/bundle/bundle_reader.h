// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __BUNDLE_READER_H__
#define __BUNDLE_READER_H__

#include <cstdint>
#include "pal.h"

namespace bundle
{
	struct bundle_reader_t
	{
		bundle_reader_t(const int8_t* base_ptr)
		{
			m_base_ptr = base_ptr;
			m_ptr = base_ptr;
		}

	public:

		void set_offset(int64_t offset)
		{
			m_ptr = m_base_ptr + offset;
		}

		int8_t read()
		{
			return *m_ptr++;
		}

		void read(void* dest, int64_t len)
		{
			memcpy(dest, m_ptr, len);
			m_ptr += len;
		}

		void direct_read(const void* &dest, int64_t len)
		{
			dest = m_ptr;
			m_ptr += len;
		}

		size_t read_path_length();
		void read_path_string(pal::string_t &str);

	private:
		const int8_t* m_ptr;
		const int8_t* m_base_ptr;
	};
}

#endif // __BUNDLE_READER_H__
