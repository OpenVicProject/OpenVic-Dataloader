#!/usr/bin/env python

import os

BINDIR = "bin"

env = SConscript("scripts/SConstruct", exports={"gen_dir": "include/openvic-dataloader/gen"})

env.PrependENVPath("PATH", os.getenv("PATH"))

if env.is_standalone:
    env.VariantDir(env["build_dir"], env.Dir("."), duplicate=False)

SConscript("deps/SCsub", "env", variant_dir=env["build_dir"].Dir("deps"), duplicate=False)

env["name_prefix"] = "dl"
gen_commit_info = env.Git(
    "commit_info.gen.hpp",
    env.Value(env.GetGitInfo()),
)
Default(gen_commit_info)

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

env.AddLibraryIncludes("include", add_variant_dir=True)
env.AddLibraryIncludes("src", False)
env.AddLibraryIncludes("src/openvic-dataloader", False)
env.AddLibrarySources("src/openvic-dataloader")

if env["build_ovdl_library"]:
    env.BuildBaseLibrary(os.path.join(BINDIR, "libopenvic-dataloader"))

if env["build_ovdl_headless"]:
    env.BuildHeadlessProgram(
        target=os.path.join(BINDIR, "openvic-dataloader"),
        src_dir="src/headless",
        defines_prefix="openvic_dataloader",
        include_lib_src=not env["build_ovdl_library"],
    )

if env["build_ovdl_tests"]:
    tests_env = SConscript("tests/SCsub", "env", variant_dir=env["build_dir"].Dir("tests"), duplicate=False)

    if env["run_ovdl_tests"]:
        tests_env.RunUnitTest()

Return("env")
