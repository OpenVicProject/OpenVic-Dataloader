#!/usr/bin/env python

import os

BINDIR = "bin"

env = SConscript("scripts/SConstruct")

env.PrependENVPath("PATH", os.getenv("PATH"))

opts = env.SetupOptions()

opts.Add(BoolVariable("build_ovdl_tests", "Build and run the openvic dataloader tests", env.is_standalone))
opts.Add(BoolVariable("run_ovdl_tests", "Run the openvic dataloader tests", False))
opts.Add(
    BoolVariable(
        "build_ovdl_library",
        "Build the openvic dataloader library.",
        env.get("build_ovdl_library", not env.is_standalone),
    )
)
opts.Add(BoolVariable("build_ovdl_headless", "Build the openvic dataloader headless executable", env.is_standalone))
opts.Add(
    EnumVariable(
        "compliance_type", "Type of encoding compliance to build with", "loose", ("loose", "error_replace", "error")
    )
)

env.FinalizeOptions()

suffix = ".{}.{}".format(env["platform"], env["target"])
if env.dev_build:
    suffix += ".dev"
if env["precision"] == "double":
    suffix += ".double"
suffix += "." + env["arch"]
if env["platform"] == "windows":
    if env.get("debug_crt", False):
        suffix += ".mdd"
    elif env.get("use_static_cpp", False):
        suffix += ".mt"
    else:
        suffix += ".md"
if env.get("use_asan", False):
    suffix += ".san"
env["suffix"] = suffix

build_dir = env.Dir("build/" + suffix.lstrip(".")).abspath.replace("\\", "/")
env["build_dir"] = build_dir

env.exposed_includes = []

SConscript("deps/SCsub", "env")

env.openvic_dataloader = {}

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
source_path = "src/openvic-dataloader"
include_path = "include"
# Out-of-source build: variant tree holds object files only, not copies of the
# source. Compile diagnostics therefore reference original source paths.
dataloader_variant = build_dir + "/" + source_path
env.VariantDir(dataloader_variant, source_path, duplicate=False)
env.Append(CPPPATH=[[env.Dir(p) for p in [include_path, dataloader_variant, source_path]]])
sources = env.GlobRecursiveVariant("*.cpp", source_path, dataloader_variant)
env.dataloader_sources = sources

library = None
env["OBJSUFFIX"] = suffix + env["OBJSUFFIX"]
library_name = "libopenvic-dataloader{}{}".format(suffix, env["LIBSUFFIX"])

default_args = []

match env["compliance_type"]:
    case "error_replace":
        env.Append(CPPDEFINES=[("OPENVIC_DATALOADER_ENCODING_COMPLIANCE", 1)])
    case "error":
        env.Append(CPPDEFINES=[("OPENVIC_DATALOADER_ENCODING_COMPLIANCE", 2)])

if env["run_ovdl_tests"]:
    env["build_ovdl_tests"] = True

if env["build_ovdl_tests"]:
    env["build_ovdl_library"] = True

if env["build_ovdl_library"]:
    library = env.StaticLibrary(target=os.path.join(BINDIR, library_name), source=sources)
    default_args += [library]

    env.Append(LIBPATH=[env.Dir(BINDIR)])
    env.Prepend(LIBS=[library_name])

    env.openvic_dataloader["LIBPATH"] = env["LIBPATH"]
    env.openvic_dataloader["LIBS"] = env["LIBS"]
    env.openvic_dataloader["INCPATH"] = [env.Dir(include_path)] + env.exposed_includes

headless_program = None
env["PROGSUFFIX"] = suffix + env["PROGSUFFIX"]

if env["build_ovdl_headless"]:
    headless_name = "openvic-dataloader"
    headless_env = env.Clone()
    headless_src = "src/headless"
    headless_variant = build_dir + "/" + headless_src
    headless_env.VariantDir(headless_variant, headless_src, duplicate=False)
    headless_env.Append(CPPDEFINES=["OPENVIC_DATALOADER_HEADLESS"])
    headless_env.Append(CPPPATH=[headless_env.Dir(headless_variant), headless_env.Dir(headless_src)])
    headless_env.headless_sources = env.GlobRecursiveVariant("*.cpp", headless_src, headless_variant)
    if not env["build_ovdl_library"]:
        headless_env.headless_sources += sources
    headless_program = headless_env.Program(
        target=os.path.join(BINDIR, headless_name),
        source=headless_env.headless_sources,
        PROGSUFFIX=".headless" + env["PROGSUFFIX"],
    )
    default_args += [headless_program]

if env["build_ovdl_tests"]:
    tests_env = SConscript("tests/SCsub", "env")

    if env["run_ovdl_tests"]:
        tests_env.RunUnitTest()

# Add compiledb if the option is set
if env.get("compiledb", False):
    default_args += ["compiledb"]

Default(*default_args)

if "env" in locals():
    # FIXME: This method mixes both cosmetic progress stuff and cache handling...
    env.show_progress(env)

Return("env")
