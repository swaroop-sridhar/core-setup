// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.IO;

namespace Microsoft.NET.HostModel.MachO
{
    public struct MachHeader
    {
        uint magic;
        int cputype;
        int cpusubtype;
        uint filetype;
        uint ncmds;
        uint sizeofcmds;
        uint flags;
    };

    public struct mach_header_64
    {
        uint magic;
        int cputype;
        int cpusubtype;
        uint filetype;
        uint ncmds;
        uint sizeofcmds;
        uint flags;
        uint reserved;
    };

}

