﻿// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.IO;
using System.IO.MemoryMappedFiles;

namespace Microsoft.DotNet.CoreSetup.Test.HostActivation
{
    public static class FileUtils
    {
        public static void EnsureFileDirectoryExists(string filePath)
        {
            EnsureDirectoryExists(Path.GetDirectoryName(filePath));
        }

        public static void EnsureDirectoryExists(string path)
        {
            if (!Directory.Exists(path))
            {
                Directory.CreateDirectory(path);
            }
        }

        public static void CreateEmptyFile(string filePath)
        {
            EnsureFileDirectoryExists(filePath);
            File.WriteAllText(filePath, string.Empty);
        }
    }
}
