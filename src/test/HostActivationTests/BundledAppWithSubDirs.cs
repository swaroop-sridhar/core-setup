// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.IO;
using Xunit;
using Microsoft.DotNet.Cli.Build.Framework;

namespace Microsoft.DotNet.CoreSetup.Test.HostActivation
{
    public class BundledAppWithSubDirs : IClassFixture<BundledAppWithSubDirs.SharedTestState>
    {
        private SharedTestState sharedTestState;

        public BundledAppWithSubDirs(SharedTestState fixture)
        {
            sharedTestState = fixture;
        }

        // Bundle to a single-file
        // This step should be removed in favor of publishing with /p:PublishSingleFile=true
        // once associated changes in SDK repo are checked in.
        string BundleApp(TestProjectFixture fixture)
        {
            var hostName = Path.GetFileName(fixture.TestProject.AppExe);
            string bundleDir = Path.Combine(fixture.TestProject.ProjectDirectory, "bundle");
            Directory.CreateDirectory(bundleDir);
            var bundler = new Microsoft.NET.HostModel.Bundle.Bundler(hostName, bundleDir);
            string singleFile = bundler.GenerateBundle(fixture.TestProject.OutputDirectory);
            return singleFile;
        }

        [Fact]
        private void Bundled_Framework_dependent_App_Run_Succeeds()
        {
            var fixture = sharedTestState.TestFrameworkDependentFixture.Copy();
            var singleFile = BundleApp(fixture);

            // Run the bundled app (extract files)
            Command.Create(singleFile)
                .CaptureStdErr()
                .CaptureStdOut()
                .Execute()
                .Should()
                .Pass()
                .And
                .HaveStdOutContaining("Wow! We now say hello to the big world and you.");

            // Run the bundled app again (reuse extracted files)
            Command.Create(singleFile)
                .CaptureStdErr()
                .CaptureStdOut()
                .Execute()
                .Should()
                .Pass()
                .And
                .HaveStdOutContaining("Wow! We now say hello to the big world and you.");
        }

        /* [Fact]
        private void Bundled_Self_Contained_App_Run_Succeeds()
        {
            var fixture = sharedTestState.TestSelfContainedFixture.Copy();
            var singleFile = BundleApp(fixture);

            // Run the bundled app (extract files)
            Command.Create(singleFile)
                .CaptureStdErr()
                .CaptureStdOut()
                .Execute()
                .Should()
                .Pass()
                .And
                .HaveStdOutContaining("Wow! We now say hello to the big world and you.");

            // Run the bundled app again (reuse extracted files)
            Command.Create(singleFile)
                .CaptureStdErr()
                .CaptureStdOut()
                .Execute()
                .Should()
                .Pass()
                .And
                .HaveStdOutContaining("Wow! We now say hello to the big world and you.");
        } */

        public class SharedTestState : IDisposable
        {
            public TestProjectFixture TestFrameworkDependentFixture { get; set; }
            public TestProjectFixture TestSelfContainedFixture { get; set; }
            public RepoDirectoriesProvider RepoDirectories { get; set; }

            public SharedTestState()
            {
                RepoDirectories = new RepoDirectoriesProvider();

                TestFrameworkDependentFixture = new TestProjectFixture("StandaloneAppWithSubDirs", RepoDirectories);
                TestFrameworkDependentFixture
                    .EnsureRestoredForRid(TestFrameworkDependentFixture.CurrentRid, RepoDirectories.CorehostPackages)
                    .PublishProject(runtime: TestFrameworkDependentFixture.CurrentRid, selfContained: "false"); 
                    
                /* TestSelfContainedFixture = new TestProjectFixture("StandaloneAppWithSubDirs", RepoDirectories);
                TestSelfContainedFixture
                    .EnsureRestoredForRid(TestSelfContainedFixture.CurrentRid, RepoDirectories.CorehostPackages)
                    .PublishProject(runtime: TestSelfContainedFixture.CurrentRid); */
            }

            public void Dispose()
            {
                //TestFrameworkDependentFixture.Dispose();
                //TestSelfContainedFixture.Dispose();
            }
        }
    }
}
