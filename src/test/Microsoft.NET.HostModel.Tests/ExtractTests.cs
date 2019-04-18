// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.IO;
using Xunit;
using Microsoft.DotNet.Cli.Build.Framework;
using Microsoft.DotNet.CoreSetup.Test;
using Microsoft.NET.HostModel.Bundle;

namespace Microsoft.NET.HostModel.Tests
{
    public class ExtractTests : IClassFixture<ExtractTests.SharedTestState>
    {
        private SharedTestState sharedTestState;

        public ExtractTests(ExtractTests.SharedTestState fixture)
        {
            sharedTestState = fixture;
        }

        [Fact]
        public void ExtractingANonBundleFails()
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            Extractor extractor = new Extractor(fixture.TestProject.AppExe, Directory.GetCurrentDirectory());
            Assert.Throws<BundleException>(() => extractor.ExtractFiles());
        }


        public class SharedTestState : IDisposable
        {
            public TestProjectFixture TestFixture { get; set; }
            public RepoDirectoriesProvider RepoDirectories { get; set; }

            public SharedTestState()
            {
                RepoDirectories = new RepoDirectoriesProvider();

                TestFixture = new TestProjectFixture("StandaloneApp", RepoDirectories);
                TestFixture
                    .EnsureRestoredForRid(TestFixture.CurrentRid, RepoDirectories.CorehostPackages)
                    .PublishProject(runtime: TestFixture.CurrentRid);
            }

            public void Dispose()
            {
                TestFixture.Dispose();
            }
        }
    }
}
