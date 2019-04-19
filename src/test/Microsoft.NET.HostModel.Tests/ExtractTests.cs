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

        private static string GetHostName(TestProjectFixture fixture)
        {
            return Path.GetFileName(fixture.TestProject.AppExe);
        }

        private static string GetPublishPath(TestProjectFixture fixture)
        {
            return Path.Combine(fixture.TestProject.ProjectDirectory, "publish");
        }

        private static DirectoryInfo GetBundleDir(TestProjectFixture fixture)
        {
            return Directory.CreateDirectory(Path.Combine(fixture.TestProject.ProjectDirectory, "bundle"));
        }

        [Fact]
        public void ExtractingANonBundleFails()
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            var hostName = GetHostName(fixture);
            var hostExe = Path.Combine(GetPublishPath(fixture), hostName);

            var bundleDir = GetBundleDir(fixture);
            Extractor extractor = new Extractor(hostExe, "extract");
            Assert.Throws<BundleException>(() => extractor.ExtractFiles());
        }

        [Fact]
        public void AllBundledFilesAreExtracted()
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            var hostName = GetHostName(fixture);
            var bundleDir = GetBundleDir(fixture);

            var bundler = new Bundler(hostName, bundleDir.FullName);
            string singleFile = bundler.GenerateBundle(GetPublishPath(fixture));

            var expectedFiles = new List<string>(bundler.BundleManifest.Files.Count);
            expectedFiles.Add(hostName);
            bundler.BundleManifest.Files.ForEach(file => expectedFiles.Add(file.RelativePath));

            new Extractor(singleFile, bundleDir.FullName).ExtractFiles();
            bundleDir.Should().OnlyHaveFiles(expectedFiles);
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
                    .PublishProject(runtime: TestFixture.CurrentRid,
                                    outputDirectory: GetPublishPath(TestFixture));
            }

            public void Dispose()
            {
                TestFixture.Dispose();
            }
        }
    }
}
