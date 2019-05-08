// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __BUNDLE_INFO_H__
#define __BUNDLE_INFO_H__

#include <cstdint>
#include "file_type.h"

namespace bundle
{

	// BundleInfo: Records information about the bundle that the bundle-processor 
	//             communicates to other parts of the host/runtime.
	
	class bundle_info_t
	{
	private:
		FILE* m_bundle_stream;
		std::list<file_entry_t*> m_files;

	public:
		bundle_info_t(FILE *bundle_stream, std::list<file_entry_t*> files)
			:m_bundle_stream(bundle_stream), m_files(files)
		{
		}
	};
}
#endif // __BUNDLE_INFO_H__
