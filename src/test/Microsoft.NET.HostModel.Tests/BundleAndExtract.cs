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
    public class BundleAndExtract : IClassFixture<BundleAndExtract.SharedTestState>
    {
        private SharedTestState sharedTestState;

        public BundleAndExtract(BundleAndExtract.SharedTestState fixture)
        {
            sharedTestState = fixture;
        }

        public void RunTheApp(string path)
        {
            Command.Create(path)
                .CaptureStdErr()
                .CaptureStdOut()
                .Execute()
                .Should()
                .Pass()
                .And
                .HaveStdOutContaining("Wow! We now say hello to the big world and you.");
        }

        private void BundleExtractRun(TestProjectFixture fixture, string publishDir, string singleFileDir)
        {
            var hostName = Path.GetFileName(fixture.TestProject.AppExe);

            // Run the App normally
            RunTheApp(Path.Combine(publishDir, hostName));

            // Bundle to a single-file
            Bundler bundler = new Bundler(hostName, singleFileDir);
            string singleFile = bundler.GenerateBundle(publishDir);

            // Extract the file
            Extractor extractor = new Extractor(singleFile, singleFileDir);
            extractor.ExtractFiles();

            // Run the extracted app
            RunTheApp(singleFile);
        }

        private string GetBundleOutputDir(TestProjectFixture fixture)
        {
            // Create a directory for bundle/extraction output.
            // This directory shouldn't be within TestProject.OutputDirectory, since the bundler
            // will (attempt to) embed all files below the TestProject.OutputDirectory tree into one file.

            string singleFileDir = Path.Combine(fixture.TestProject.ProjectDirectory, "bundle");
            Directory.CreateDirectory(singleFileDir);

            return singleFileDir;
        }

        private string RelativePath(string path)
        {
            return Path.GetRelativePath(Directory.GetCurrentDirectory(), path)
                       .TrimEnd(Path.DirectorySeparatorChar);
        }

        [Fact]
        public void TestWithAbsolutePaths()
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            string publishDir = fixture.TestProject.OutputDirectory;
            string outputDir = GetBundleOutputDir(fixture);

            BundleExtractRun(fixture, publishDir, outputDir);
        }

        [Fact]
        public void TestWithRelativePaths()
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            string publishDir = RelativePath(fixture.TestProject.OutputDirectory);
            string outputDir = RelativePath(GetBundleOutputDir(fixture));

            BundleExtractRun(fixture, publishDir, outputDir);
        }

        [Fact]
        public void TestWithRelativePathsDirSeparator()
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            string publishDir = RelativePath(fixture.TestProject.OutputDirectory) + Path.DirectorySeparatorChar;
            string outputDir = RelativePath(GetBundleOutputDir(fixture)) + Path.DirectorySeparatorChar;

            BundleExtractRun(fixture, publishDir, outputDir);
        }

        public class SharedTestState : IDisposable
        {
            public TestProjectFixture TestFixture { get; set; }
            public RepoDirectoriesProvider RepoDirectories { get; set; }

            public SharedTestState()
            {
                RepoDirectories = new RepoDirectoriesProvider();

                TestFixture = new TestProjectFixture("StandaloneAppWithSubDirs", RepoDirectories);
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
