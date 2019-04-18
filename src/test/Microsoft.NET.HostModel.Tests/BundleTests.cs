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
    public class BundleTests : IClassFixture<BundleTests.SharedTestState>
    {
        private SharedTestState sharedTestState;

        public BundleTests(BundleTests.SharedTestState fixture)
        {
            sharedTestState = fixture;
        }

        private DirectoryInfo GetBundleOutputDir(TestProjectFixture fixture)
        {
            string singleFileDir = Path.Combine(fixture.TestProject.ProjectDirectory, "bundle");
            return Directory.CreateDirectory(singleFileDir);
        }

        [Fact]
        public void TestWithEmptySpecFails()
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            var hostName = Path.GetFileName(fixture.TestProject.AppExe);
            Bundler bundler = new Bundler(hostName, GetBundleOutputDir(fixture).FullName);

            FileSpec[][] invalidSpecs =
            {
                new FileSpec[] {new FileSpec(hostName, null) },
                new FileSpec[] {new FileSpec(hostName, "") },
                new FileSpec[] {new FileSpec(hostName, "    ") }
            };

            foreach (var invalidSpec in invalidSpecs)
            {
                Assert.Throws<ArgumentException>(() => bundler.GenerateBundle(invalidSpec));
            }
        }

        [Fact]
        public void TestWithoutSpecifyingHostFails()
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            string publishDir = fixture.TestProject.OutputDirectory;
            var hostName = Path.GetFileName(fixture.TestProject.AppExe);
            var appName = Path.GetFileNameWithoutExtension(fixture.TestProject.AppExe);

            // Generate a file specification without the apphost
            var fileSpecs = new List<FileSpec>();
            string[] files = { $"{appName}.dll", $"{appName}.deps.json", $"{appName}.runtimeconfig.json" };
            Array.ForEach(files, x => fileSpecs.Add(new FileSpec(x, x)));

            Bundler bundler = new Bundler(hostName, GetBundleOutputDir(fixture).FullName);
            Assert.Throws<ArgumentException>(() => bundler.GenerateBundle(fileSpecs));
        }

        [InlineData(true)]
        [InlineData(false)]
        [Theory]
        public void TestFilesInOutputDir(bool embedPDBs)
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            string publishDir = fixture.TestProject.OutputDirectory;
            var hostName = Path.GetFileName(fixture.TestProject.AppExe);
            var appName = Path.GetFileNameWithoutExtension(fixture.TestProject.AppExe);
            var bundleDir = GetBundleOutputDir(fixture);

            new Bundler(hostName, bundleDir.FullName, embedPDBs).GenerateBundle(publishDir);

            var expectedFiles = new List<string>(2);
            expectedFiles.Add(hostName);

            if(!embedPDBs)
            {
                expectedFiles.Add($"{appName}.pdb");
            }

            bundleDir.Should().OnlyHaveFiles(expectedFiles);
        }


        [InlineData(true)]
        [InlineData(false)]
        [Theory]
        public void TestFilesNotBundled(bool embedPDBs)
        {
            var fixture = sharedTestState.TestFixture
                .Copy();

            string publishDir = fixture.TestProject.OutputDirectory;
            var hostName = Path.GetFileName(fixture.TestProject.AppExe);
            var appName = Path.GetFileNameWithoutExtension(fixture.TestProject.AppExe);
            var bundleDir = GetBundleOutputDir(fixture);

            // Make up a app.runtimeconfig.dev.json file in the publish directory.
            File.Copy(Path.Combine(publishDir, $"{appName}.runtimeconfig.json"), 
                      Path.Combine(publishDir, $"{appName}.runtimeconfig.dev.json"));

            var singleFile = new Bundler(hostName, bundleDir.FullName, embedPDBs).GenerateBundle(publishDir);
            new Extractor(singleFile, bundleDir.FullName).ExtractFiles();

            bundleDir.Should().NotHaveFile($"{appName}.runtimeconfig.dev.json");

            if (!embedPDBs)
            {
                bundleDir.Should().NotHaveFile($"{appName}.pdb");
            }
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
                                    outputDirectory: Path.Combine(TestFixture.TestProject.ProjectDirectory, "publish"));
            }

            public void Dispose()
            {
                TestFixture.Dispose();
            }
        }
    }
}
