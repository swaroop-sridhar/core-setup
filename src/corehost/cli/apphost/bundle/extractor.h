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
        pal::string_t get_extraction_dir()
        {
            return m_extraction_dir;
        }

    private:
        pal::string_t m_bundle_path;
        pal::string_t m_extraction_dir;
        pal::string_t m_working_extraction_dir;
    };

}

#endif // __BUNDLE_RUNNER_H__
