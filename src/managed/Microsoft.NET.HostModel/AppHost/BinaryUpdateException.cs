﻿// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;

namespace Microsoft.NET.HostModel.AppHost
{
    /// <summary>
    /// This exception is thrown when an AppHost binary update fails due to known user errors.
    /// </summary>
    public class BinaryUpdateException : Exception
    {
        public BinaryUpdateException(string message) :
                base(message)
        {
        }
    }
}

