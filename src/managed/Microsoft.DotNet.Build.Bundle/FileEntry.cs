﻿// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.IO;

namespace Microsoft.DotNet.Build.Bundle
{
    /// <summary>
    /// FileEntry: Records information about embedded files.
    /// 
    /// The bundle manifest records the following meta-data for each 
    /// file embedded in the bundle:
    /// * Type       (1 byte)
    /// * NameLength (7-bit extension encoding, typically 1 byte)
    /// * Name       (<NameLength> Bytes)
    /// * Offset     (Int64)
    /// * Size       (Int64)
    /// </summary>
    public class FileEntry
    {
        public FileType Type;
        public string Name;
        public long Offset;
        public long Size;

        public FileEntry(FileType fileType, string name, long offset, long size)
        {
            Type = fileType;
            Name = name;
            Offset = offset;
            Size = size;
        }

        public void Write(BinaryWriter writer)
        {
            writer.Write((byte) Type);
            writer.Write(Name);
            writer.Write(Offset);
            writer.Write(Size);
        }

        public static FileEntry Read(BinaryReader reader)
        {
            FileType type = (FileType)reader.ReadByte();
            string fileName = reader.ReadString();
            long offset = reader.ReadInt64();
            long size = reader.ReadInt64();
            return new FileEntry(type, fileName, offset, size);
        }

        public override string ToString()
        {
            return String.Format($"{Name} [{Type}] @{Offset} Sz={Size}");
        }
    }
}

