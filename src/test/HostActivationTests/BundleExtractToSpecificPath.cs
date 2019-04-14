// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using Xunit;
using Microsoft.DotNet.Cli.Build.Framework;

namespace Microsoft.DotNet.CoreSetup.Test.HostActivation
{
    public class BundleExtractToSpecificPath : IClassFixture<BundledAppWithSubDirs.SharedTestState>
    {
        private SharedTestState sharedTestState;

        public BundleExtractToSpecificPath(SharedTestState fixture)
        {
            sharedTestState = fixture;
        }

        [Fact]
        private void Bundle_Extraction_To_Specific_Path_Succeeds()
        {
            var fixture = sharedTestState.TestFixture.Copy();
            var appName = Path.GetFileNameWithoutExtension(fixture.TestProject.AppDll);
            var hostName = Path.GetFileName(fixture.TestProject.AppExe);

            string publishPath = fixture.TestProject.OutputDirectory;
            string bundlePath = Path.Combine(fixture.TestProject.ProjectDirectory, "bundle");
            var bundleDir = Directory.CreateDirectory(bundlePath);

            // Bundle the publish directory.
            var bundler = new Microsoft.NET.HostModel.Bundle.Bundler(hostName, bundlePath);
            string singleFile = bundler.GenerateBundle(publishPath);
            var bundledFiles = new List<string>(bundler.BundleManifest.Files.Count);
            foreach (var file in bundler.BundleManifest.Files)
            {
                bundledFiles.Add(file.RelativePath);
            }

            bundleDir.Should().HaveFile(hostName);
            bundleDir.Should().NotHaveFiles(bundledFiles);

            string extractBasePath = Path.Combine(fixture.TestProject.ProjectDirectory, "extract");
            var extractBaseDir = Directory.CreateDirectory(extractBasePath);

            extractBaseDir.Should().NotHaveDirectory(appName);

            // Run the bunded app for the first time, and extract files to 
            // $DOTNET_BUNDLE_EXTRACT_BASE_DIR/<app>/bundle-id
            Environment.SetEnvironmentVariable("DOTNET_BUNDLE_EXTRACT_BASE_DIR", extractBasePath);
            Command.Create(singleFile)
                .CaptureStdErr()
                .CaptureStdOut()
                .Execute()
                .Should()
                .Pass()
                .And
                .HaveStdOutContaining("Hello World");

            string extractPath = Path.Combine(extractBasePath, appName, bundler.BundleManifest.BundleID);
            var extractDir = new DirectoryInfo(extractPath);
            extractDir.Should().OnlyHaveFiles(bundledFiles);
            extractDir.Should().NotHaveFile(hostName);

            extractDir.Refresh();
            DateTime firstWriteTime = extractDir.LastWriteTimeUtc;
            while(DateTime.Now <= firstWriteTime)
            {
                Thread.Sleep(1);
            }

            // Run the bundled app again (reuse extracted files)
            Command.Create(singleFile)
                .CaptureStdErr()
                .CaptureStdOut()
                .Execute()
                .Should()
                .Pass()
                .And
                .HaveStdOutContaining("Hello World");

            extractDir.Should().NotBeModifiedAfter(firstWriteTime);
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
