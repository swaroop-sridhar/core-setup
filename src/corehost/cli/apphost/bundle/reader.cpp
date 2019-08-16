// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "bundle_runner.h"
#include "pal.h"
#include "trace.h"
#include "utils.h"

using namespace bundle;

void bundle_runner_t::seek(FILE* stream, long offset, int origin)
{
    if (fseek(stream, offset, origin) != 0)
    {
        trace::error(_X("Failure processing application bundle; possible file corruption."));
        trace::error(_X("I/O seek failure within the bundle."));
        throw StatusCode::BundleExtractionIOError;
    }
}

void bundle_runner_t::write(const void* buf, size_t size, FILE *stream)
{
    if (fwrite(buf, 1, size, stream) != size)
    {
        trace::error(_X("Failure extracting contents of the application bundle."));
        trace::error(_X("I/O failure when writing extracted files."));
        throw StatusCode::BundleExtractionIOError;
    }
}

void bundle_runner_t::read(void* buf, size_t size, FILE* stream)
{
    if (fread(buf, 1, size, stream) != size)
    {
        trace::error(_X("Failure processing application bundle; possible file corruption."));
        trace::error(_X("I/O failure reading contents of the bundle."));
        throw StatusCode::BundleExtractionIOError;
    }
}

// Handle the relatively uncommon scenario where the bundle ID or 
// the relative-path of a file within the bundle is longer than 127 bytes
size_t bundle_runner_t::get_path_length(int8_t first_byte, FILE* stream)
{
    size_t length = 0;

    // If the high bit is set, it means there are more bytes to read.
    if ((first_byte & 0x80) == 0)
    {
         length = first_byte;
    }
    else
    {
        int8_t second_byte = 0;
        read(&second_byte, 1, stream);

        if (second_byte & 0x80)
        {
            // There can be no more than two bytes in path_length
            trace::error(_X("Failure processing application bundle; possible file corruption."));
            trace::error(_X("Path length encoding read beyond two bytes"));

            throw StatusCode::BundleExtractionFailure;
        }

        length = (second_byte << 7) | (first_byte & 0x7f);
    }

    if (length <= 0 || length > PATH_MAX)
    {
        trace::error(_X("Failure processing application bundle; possible file corruption."));
        trace::error(_X("Path length is zero or too long"));
        throw StatusCode::BundleExtractionFailure;
    }

    return length;
}

// Read a non-null terminated fixed length UTF8 string from a byte-stream
// and transform it to pal::string_t
void bundle_runner_t::read_string(pal::string_t &str, size_t size, FILE* stream)
{
    std::unique_ptr<uint8_t[]> buffer{new uint8_t[size + 1]};
    read(buffer.get(), size, stream);
    buffer[size] = 0; // null-terminator
    pal::clr_palstring(reinterpret_cast<const char*>(buffer.get()), &str);
}