// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __BUNDLE_READER_H__
#define __BUNDLE_READER_H__

#include <cstdint>

namespace bundle
{
	struct bundle_reader_t
	{
		bundle_reader_t(const int8_t* ptr)
		{
			m_ptr = ptr;
		}

	public:
		int8_t* get_ptr()
		{
			return m_ptr;
		}

		int8_t* set_ptr(const int8_t* ptr)
		{
			return m_ptr;
		}

		int8_t* operator +=(int64_t offset)
		{
			m_ptr += offset;
		}

		int8 read()
		{
			return *m_ptr++;
		}

		void read(void* dest, int64_t len)
		{
			memcpy(dest, m_ptr, len);
			m_ptr += len;
		}

		void* direct_read(int64_t len)
		{
			int8_t* ptr = m_ptr;
			m_ptr += len;

			return ptr;
		}

		size_t read_path_length();
		void read_path_string(pal::string_t& str);

	private:
		const int8_t* m_ptr;
    };
}

#endif // __BUNDLE_READER_H__
