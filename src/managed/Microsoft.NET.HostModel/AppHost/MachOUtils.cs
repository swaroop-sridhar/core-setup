// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Runtime.CompilerServices;
using System.Text;

namespace Microsoft.NET.HostModel.AppHost
{
    internal static class MachOUtils
    {
        // The MachO Headers are copied from 
        // https://opensource.apple.com/source/cctools/cctools-870/include/mach-o/loader.h
        //
        // The data fields and enumerations match the structure definitions in the above file,
        // and hence do not conform to C# CoreFx naming style.

        enum Magic : uint
        {
            MH_MAGIC = 0xfeedface,
            MH_CIGAM = 0xcefaedfe,
            MH_MAGIC_64 = 0xfeedfacf,
            MH_CIGAM_64 = 0xcffaedfe
        }

        enum FileType : uint
        {
            MH_EXECUTE = 0x2
        }

#pragma warning disable 0649
        struct MachOHeader
        {
            public Magic magic;
            public int cputype;
            public int cpusubtype;
            public FileType filetype;
            public uint ncmds;
            public uint sizeofcmds;
            public uint flags;
            public uint reserved;

            unsafe public uint Size => (uint)sizeof(MachOHeader);

            public bool Is64BitExecutable()
            {
                return magic == Magic.MH_MAGIC_64 && filetype == FileType.MH_EXECUTE;
            }

            public bool IsValid()
            {
                switch (magic)
                {
                    case Magic.MH_CIGAM:
                    case Magic.MH_CIGAM_64:
                    case Magic.MH_MAGIC:
                    case Magic.MH_MAGIC_64:
                        return true;

                    default:
                        return false;
                }
            }
        }

        enum Command : uint
        {
            LC_SYMTAB = 0x2,
            LC_SEGMENT_64 = 0x19,
            LC_CODE_SIGNATURE = 0x1d
        }

        struct LoadCommand
        {
            public Command cmd;
            public uint cmdsize;

            public bool Is(Command command)
            {
                return cmd == command;
            }

            public unsafe bool IsSegmentCommand64(string name)
            {
                return Is(Command.LC_SEGMENT_64) &&
                    AsSegmentCommand64()->IsNamed(name);
            }

            public unsafe SymtabCommand* AsSymtabCommand()
            {
                fixed (LoadCommand* command = &this)
                {
                    return (SymtabCommand*)command;
                }
            }

            public unsafe LinkEditDataCommand* AsLinkEditDataCommand()
            {
                fixed (LoadCommand* command = &this)
                {
                    return (LinkEditDataCommand*)command;
                }
            }

            public unsafe SegmentCommand64* AsSegmentCommand64()
            {
                fixed (LoadCommand* command = &this)
                {
                    return (SegmentCommand64*)command;
                }
            }
        }

        // The linkedit_data_command contains the offsets and sizes of a blob
        // of data in the __LINKEDIT segment (including LC_CODE_SIGNATURE).  
        struct LinkEditDataCommand
        {
            public Command cmd;
            public uint cmdsize;
            public uint dataoff;
            public uint datasize;

            public ulong EndOffset => dataoff + datasize;
        }

        struct SymtabCommand
        {
            public uint cmd;
            public uint cmdsize;
            public uint symoff;
            public uint nsyms;
            public uint stroff;
            public uint strsize;

            public ulong EndOffset => stroff + strsize;
        };

        unsafe struct SegmentCommand64
        {
            public Command cmd;
            public uint cmdsize;
            public fixed byte segname[16];
            public ulong vmaddr;
            public ulong vmsize;
            public ulong fileoff;
            public ulong filesize;
            public int maxprot;
            public int initprot;
            public uint nsects;
            public uint flags;

            public ulong EndOffset => fileoff + filesize;

            public uint Size => (uint)sizeof(SegmentCommand64);

            public string GetName()
            {
                fixed (byte* str = segname)
                {
                    return GetString(str, 16);
                }
            }

            public bool IsNamed(string name)
            {
                return GetName().Equals(name);
            }

            public void Init(string name, ulong offset, ulong size)
            {
                cmd = Command.LC_SEGMENT_64;
                cmdsize = Size;

                fixed(byte *str = segname)
                {
                    WriteString(name, str, 16);
                }

                vmaddr = 0;
                vmsize = size;
                fileoff = offset;
                filesize = size;
                maxprot = 0;
                nsects = 0;
                flags = 0x4; // No Relocation
            }
        }

        unsafe struct Section64
        { 
            public fixed byte sectname[16];
            public fixed byte segname[16];
            public ulong addr;
            public ulong size;
            public uint offset;
            public uint align;
            public uint reloff;
            public uint nreloc;
            public uint flags;
            public uint reserved1;
            public uint reserved2;
            public uint reserved3;

            public ulong EndOffset => offset + size;

            public uint Size => (uint)sizeof(Section64);

            public string GetName()
            {
                fixed (byte* str = sectname)
                {
                    return GetString(str, 16);
                }
            }

            public string GetSegName()
            {
                fixed (byte* str = segname)
                {
                    return GetString(str, 16);
                }
            }

            public bool IsNamed(string name)
            {
                return GetName().Equals(name);
            }

            public bool IsIn(SegmentCommand64 *segment)
            {
                return GetSegName().Equals(segment->GetName());
            }
        };

#pragma warning restore 0649

        unsafe static string GetString(byte* str, uint length)
        {
            int count = 0;
            while (*(str + count) != 0 && count++ < length) ;

            try
            {
                return Encoding.UTF8.GetString(str, count);
            }
            catch (ArgumentException)
            {
                throw new AppHostMachOFormatException(MachOFormatError.InvalidUTF8);
            }
        }

        unsafe static void WriteString(string src, byte* dest, uint maxLen)
        {
            byte[] bytes = Encoding.UTF8.GetBytes(src);
            Verify(bytes.Length < maxLen, MachOFormatError.InvalidUTF8);

            foreach(var b in bytes)
            {
                *dest++ = b;
            }
        }

        private static void Verify(bool condition, MachOFormatError error)
        {
            if (!condition)
            {
                throw new AppHostMachOFormatException(error);
            }
        }

        /// <summary>
        /// This Method is a utility to remove the code-signature (if any)
        /// from a MachO AppHost binary.
        /// 
        /// The tool assumes the following layout of the executable:
        /// 
        /// * MachoHeader (64-bit, executable, not swapped integers)
        /// * LoadCommands
        ///     LC_SEGMENT_64 (__PAGEZERO)
        ///     LC_SEGMENT_64 (__TEXT)
        ///     LC_SEGMENT_64 (__DATA)
        ///     LC_SEGMENT_64 (__LINKEDIT)
        ///     ...
        ///     LC_SYMTAB 
        ///     ...
        ///     LC_CODE_SIGNATURE (last)
        ///     
        ///  * ... Different Segments ...
        ///  
        ///  * The __LINKEDIT Segment (last)
        ///      * ... Different sections ... 
        ///      * SYMTAB 
        ///      * (Some alignment bytes)
        ///      * The Code-signature
        ///      
        /// In order to remove the signature, the method:
        /// - Removes (zeros out) the LC_CODE_SIGNATURE command
        /// - Adjusts the size and count of the load commands in the header
        /// - Truncates the size of the __LINKEDIT segment to the end of SYMTAB
        /// - Truncates the apphost file to the end of the __LINKEDIT segment
        /// 
        /// </summary>
        /// <param name="filePath">Path to the AppHost</param>
        /// <returns>
        ///  True if 
        ///    - The input is a MachO binary, and 
        ///    - It is a signed binary, and 
        ///    - The signature was successfully removed
        ///   False otherwise 
        /// </returns>
        /// <exception cref="AppHostMachOFormatException">
        /// The input is a MachO file, but doesn't match the expect format of the AppHost.
        /// </exception>
        unsafe public static bool RemoveSignature(string filePath)
        {
            using (var stream = new FileStream(filePath, FileMode.Open, FileAccess.ReadWrite))
            {
                uint signatureSize = 0;
                using (var mappedFile = MemoryMappedFile.CreateFromFile(stream,
                                                                        mapName: null,
                                                                        capacity: 0,
                                                                        MemoryMappedFileAccess.ReadWrite,
                                                                        HandleInheritability.None,
                                                                        leaveOpen: true))
                {
                    using (var accessor = mappedFile.CreateViewAccessor())
                    {
                        byte* file = null;
                        RuntimeHelpers.PrepareConstrainedRegions();
                        try
                        {
                            accessor.SafeMemoryMappedViewHandle.AcquirePointer(ref file);
                            Verify(file != null, MachOFormatError.MemoryMapAccessFault);

                            MachOHeader* header = (MachOHeader*)file;

                            if (!header->IsValid())
                            {
                                // Not a MachO file.
                                return false;
                            }

                            Verify(header->Is64BitExecutable(), MachOFormatError.Not64BitExe);

                            file += header->Size;
                            SegmentCommand64* linkEdit = null;
                            SymtabCommand* symtab = null;
                            LinkEditDataCommand* signature = null;

                            for (uint i = 0; i < header->ncmds; i++)
                            {
                                LoadCommand* command = (LoadCommand*)file;
                                if (command->IsSegmentCommand64("__LINKEDIT"))
                                {
                                    Verify(linkEdit == null, MachOFormatError.DuplicateSegment);
                                    linkEdit = command->AsSegmentCommand64();
                                }
                                else if (command->Is(Command.LC_SYMTAB))
                                {
                                    Verify(symtab == null, MachOFormatError.DuplicateSymtab);
                                    symtab = command->AsSymtabCommand();
                                }
                                else if (command->Is(Command.LC_CODE_SIGNATURE))
                                {
                                    Verify(i == header->ncmds - 1, MachOFormatError.SignCommandNotLast);
                                    signature = command->AsLinkEditDataCommand();
                                    break;
                                }

                                file += command->cmdsize;
                            }

                            if (signature != null)
                            {
                                Verify(linkEdit != null, MachOFormatError.MissingLinkEditSegment);
                                Verify(symtab != null, MachOFormatError.MissingSymtab);

                                var fileEnd = (ulong)stream.Length;

                                Verify(linkEdit->EndOffset == fileEnd, MachOFormatError.LinkEditNotLast);
                                Verify(signature->EndOffset == fileEnd, MachOFormatError.SignBlobNotLast);

                                Verify(symtab->symoff > linkEdit->fileoff, MachOFormatError.SymtabNotInLinkEdit);
                                Verify(signature->dataoff > linkEdit->fileoff, MachOFormatError.SignNotInLinkEdit);

                                // The signature blob immediately follows the symtab blob, 
                                // except for a few bytes of padding.
                                Verify(signature->dataoff >= symtab->EndOffset && signature->dataoff - symtab->EndOffset < 32, MachOFormatError.SignBlobNotLast);

                                // Remove the signature command
                                header->ncmds--;
                                header->sizeofcmds -= signature->cmdsize;
                                Unsafe.InitBlock(signature, 0, signature->cmdsize);

                                // Remove the signature blob (note for truncation)
                                signatureSize = (uint)(fileEnd - symtab->EndOffset);

                                // Adjust the __LINKEDIT segment load command
                                linkEdit->filesize -= signatureSize;

                                // codesign --remove-signature doesn't reset the vmsize.
                                // Setting the vmsize here makes the output bin-equal with the original 
                                // unsigned apphost (and not bin-equal with a signed-unsigned-apphost).
                                linkEdit->vmsize = linkEdit->filesize;
                            }
                        }
                        finally
                        {
                            if(file != null)
                            {
                                accessor.SafeMemoryMappedViewHandle.ReleasePointer();
                            }
                        }
                    }
                }

                if (signatureSize != 0)
                {
                    // The signature was removed, update the file length
                    stream.SetLength(stream.Length - signatureSize);
                    return true;
                }

                return false;
            }
        }

        unsafe public static bool AdjustHeadersForBundle(string filePath)
        {
            using (var stream = new FileStream(filePath, FileMode.Open, FileAccess.ReadWrite))
            {
                using (var mappedFile = MemoryMappedFile.CreateFromFile(stream,
                                                                        mapName: null,
                                                                        capacity: 0,
                                                                        MemoryMappedFileAccess.ReadWrite,
                                                                        HandleInheritability.None,
                                                                        leaveOpen: true))
                {
                    using (var accessor = mappedFile.CreateViewAccessor())
                    {
                        byte* file = null;
                        RuntimeHelpers.PrepareConstrainedRegions();
                        try
                        {
                            accessor.SafeMemoryMappedViewHandle.AcquirePointer(ref file);
                            Verify(file != null, MachOFormatError.MemoryMapAccessFault);

                            MachOHeader* header = (MachOHeader*)file;

                            if (!header->IsValid())
                            {
                                // Not a MachO file.
                                return false;
                            }

                            Verify(header->Is64BitExecutable(), MachOFormatError.Not64BitExe);

                            file += header->Size;
                            SegmentCommand64* linkEdit = null;
                            SegmentCommand64* textSegment = null;
                            Section64* textSection = null;

                            for (uint i = 0; i < header->ncmds; i++)
                            {
                                LoadCommand* command = (LoadCommand*)file;
                                if (command->IsSegmentCommand64("__LINKEDIT"))
                                {
                                    Verify(linkEdit == null, MachOFormatError.DuplicateSegment);
                                    linkEdit = command->AsSegmentCommand64();
                                }
                                else if (command->IsSegmentCommand64("__TEXT"))
                                {
                                    Verify(textSegment == null, MachOFormatError.DuplicateSegment);
                                    textSegment = command->AsSegmentCommand64();

                                    byte* sectionIterator = file + textSegment->Size;
                                    for (uint j = 0; j < textSegment->nsects; j++)
                                    {
                                        Section64* section = (Section64*)(sectionIterator);
                                        if (section->IsNamed("__text"))
                                        {
                                            Verify(textSection == null, MachOFormatError.DuplicateSegment);
                                            textSection = section;
                                            Verify(textSection->IsIn(textSegment), MachOFormatError.TextSectNotInTextSeg);
                                        }
                                        sectionIterator += section->Size;
                                    }
                                }

                                Verify(!command->Is(Command.LC_CODE_SIGNATURE), MachOFormatError.SignNotRemoved);
                                file += command->cmdsize;
                            }

                            Verify(textSegment != null, MachOFormatError.MissingTextSegment);
                            Verify(textSection != null, MachOFormatError.MissingTextSection);
                            Verify(linkEdit != null, MachOFormatError.MissingLinkEditSegment);
                            Verify(textSegment->fileoff == 0, MachOFormatError.TextSegmentNotAtStart);
                            Verify(textSection->offset - header->sizeofcmds > sizeof(SegmentCommand64), MachOFormatError.NoSpaceForMoreCommands);

                            byte *endOfCommands = (byte*)header + header->Size + header->sizeofcmds;

                            SegmentCommand64* newSegment = (SegmentCommand64*)endOfCommands;
                            var newSegmentOffset = linkEdit->EndOffset;
                            var newSegmentSize = (ulong)stream.Length - newSegmentOffset;
                            newSegment->Init("__BUNDLE", newSegmentOffset, newSegmentSize);

                            header->ncmds++;
                            header->sizeofcmds += newSegment->cmdsize;
                        }
                        finally
                        {
                            if (file != null)
                            {
                                accessor.SafeMemoryMappedViewHandle.ReleasePointer();
                            }
                        }
                    }
                }

                return true;
            }
        }
    }
}

