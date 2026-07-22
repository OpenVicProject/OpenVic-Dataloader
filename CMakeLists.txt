cmake_minimum_required(VERSION 3.28)
project(openvic-dataloader LANGUAGES CXX)

if(NOT COMMAND openvic_setup_base_flags)
    # scripts is the shared build-tooling repo, fetched once as a pinned
    # tarball (like the other third-party deps). In a composed build the
    # outermost repo fetches + loads it first and this guard skips for the
    # nested repos; a standalone repo fetches it itself. FetchContent also
    # dedups by name, so either path yields exactly one fetch.
    #
    # Raw FetchContent (not openvic_declare_dep) on purpose: openvic_declare_dep
    # lives INSIDE scripts (OpenVicDeps.cmake), so it isn't defined yet here.
    # Local-dev escape hatch: -DFETCHCONTENT_SOURCE_DIR_OPENVIC_SCRIPTS=<path>
    # points this at a working-tree scripts checkout.
    include(FetchContent)
    FetchContent_Declare(
        openvic_scripts
        URL "https://github.com/OpenVicProject/scripts/archive/3fcac2ec942c152bd83b2c5ad279ef192e0125d7.tar.gz"
        URL_HASH SHA256=c0759a8d5eb8c6a8fa7780f01a6bd2bf079a18ef2527f0c7ec7ecc9b87aed635
    )
    FetchContent_MakeAvailable(openvic_scripts)
    include("${openvic_scripts_SOURCE_DIR}/cmake/OpenVicScripts.cmake")
endif()
openvic_setup_base_flags()
openvic_disable_rtti()

set(OPENVIC_DATALOADER_COMPLIANCE
    "loose"
    CACHE STRING
    "Type of encoding compliance to build with (loose|error_replace|error)"
)
set_property(CACHE OPENVIC_DATALOADER_COMPLIANCE PROPERTY STRINGS "loose;error_replace;error")

# Third-party deps are fetched as pinned tarballs (see OpenVicDeps.cmake) with
# EXCLUDE_FROM_ALL so their install rules stay out of `cmake --install`
# (several of these projects have no off switch for their installs).
set(DRYAD_BUILD_TESTS OFF)
openvic_declare_dep(dryad
    REPO Spartan322/dryad
    SHA 0140ce828c1215f023d3c73651bc0aa4b02c67d6
    SHA256 9567df8b657e6461db48aefdd800382311956b2eda4921ae842450f65dcaed84
)
FetchContent_MakeAvailable(dryad)
if(CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    # clang + MSVC STL defines _MSC_VER, so dryad's builtin_clzll shim takes
    # the MSVC branch, but DRYAD_FORCE_INLINE resolves to
    # [[gnu::always_inline]] there -- an attribute with no inline linkage ->
    # duplicate symbols at link. __forceinline carries linkage like cl.exe.
    # (Upstream dryad bug; the macro is designed to be pre-definable.)
    target_compile_definitions(dryad INTERFACE DRYAD_FORCE_INLINE=__forceinline)
endif()

set(FMT_INSTALL OFF)
openvic_declare_dep(fmt
    REPO fmtlib/fmt
    SHA 40626af88bd7df9a5fb80be7b25ac85b122d6c21
    SHA256 992216a0605e03f0373984ecad11158f42d8569782f1b2c84035272da10f0ff1
)
FetchContent_MakeAvailable(fmt)

# lexy is declared under the same FetchContent name by lexy-vdf too; the first
# declaration wins globally (replacing the old `if(NOT TARGET lexy_file)`
# guard). In the composed build dataloader is added first, so its pin is used.
set(LEXY_ENABLE_INSTALL OFF)
openvic_declare_dep(lexy
    REPO foonathan/lexy
    SHA 34d2adf74a2b25b6bdd760a3bbb931f3fd5e60cd
    SHA256 a45a706f15d565b900280f58e1bd3492020a7103b9d6abf7f54cd51509f8c859
)
FetchContent_MakeAvailable(lexy)
# Build lexy's file.cpp with the unicode database enabled; upstream lexy only
# puts the define on the lexy_unicode INTERFACE target.
target_compile_definitions(lexy_file PRIVATE LEXY_HAS_UNICODE_DATABASE=1)

set(RANGE_V3_TESTS OFF)
set(RANGE_V3_EXAMPLES OFF)
set(RANGE_V3_PERF OFF)
set(RANGE_V3_DOCS OFF)
openvic_declare_dep(range-v3
    REPO spartan322/range-v3
    SHA 96b8c210ca9c783caaac3a50bb508600e28c9247
    SHA256 fc8e9ce8e0baf54e68a7259e357674ace8d12c552c503ee9206434bfd0a1ea64
)
FetchContent_MakeAvailable(range-v3)

# vmcontainer's build lives in its lib/ subdir (SOURCE_SUBDIR); the repo root
# CMakeLists is not what we build.
openvic_declare_dep(vmcontainer
    REPO spartan322/vmcontainer
    SHA f40ae3d6a9385f59000457fb186e34e730baba03
    SHA256 13d35f7aa50f97f88413763fa644e08f5ec72450bd1c2f63709985a41d41824d
    SOURCE_SUBDIR lib
)
FetchContent_MakeAvailable(vmcontainer)

file(GLOB_RECURSE dataloader_sources CONFIGURE_DEPENDS src/openvic-dataloader/*.cpp)

add_library(openvic-dataloader STATIC ${dataloader_sources})
add_library(openvic::dataloader ALIAS openvic-dataloader)

set(ovdl_gen_dir ${CMAKE_CURRENT_BINARY_DIR}/gen-include)
target_include_directories(
    openvic-dataloader
    PUBLIC include $<BUILD_INTERFACE:${ovdl_gen_dir}>
    PRIVATE src/openvic-dataloader
)

# Compile-time project info headers, included as <openvic-dataloader/gen/*.gen.hpp>.
openvic_generate_commit_info(
    TARGET openvic-dataloader
    PREFIX ovdl
    REPO_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT ${ovdl_gen_dir}/openvic-dataloader/gen/commit_info.gen.hpp
)
openvic_generate_license_info(
    TARGET openvic-dataloader
    PREFIX ovdl
    COPYRIGHT ${CMAKE_CURRENT_SOURCE_DIR}/COPYRIGHT
    LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    OUTPUT ${ovdl_gen_dir}/openvic-dataloader/gen/license_info.gen.hpp
)
openvic_generate_author_info(
    TARGET openvic-dataloader
    PREFIX ovdl
    AUTHORS ${CMAKE_CURRENT_SOURCE_DIR}/AUTHORS.md
    OUTPUT ${ovdl_gen_dir}/openvic-dataloader/gen/author_info.gen.hpp
    SECTIONS
        "Senior Developers=AUTHORS_SENIOR_DEVELOPERS"
        "Developers=AUTHORS_DEVELOPERS"
        "Contributors=AUTHORS_CONTRIBUTORS"
        "Consultants=AUTHORS_CONSULTANTS"
)
# PUBLIC deps appear in the public headers (dryad, fmt, range-v3, vmcontainer);
# lexy is an implementation detail and stays private.
target_link_libraries(
    openvic-dataloader
    PUBLIC foonathan::dryad fmt::fmt range-v3::range-v3 vmcontainer::vmcontainer
    PRIVATE foonathan::lexy
)
if(APPLE)
    # ParseHandler's system-encoding detection uses CoreFoundation and the GBK
    # converter uses iconv. PUBLIC: executables linking this static library
    # need them on their own link lines.
    target_link_libraries(openvic-dataloader PUBLIC "-framework CoreFoundation" iconv)
endif()
set_target_properties(openvic-dataloader PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

if(OPENVIC_DATALOADER_COMPLIANCE STREQUAL "error_replace")
    # PUBLIC: the unit tests (and any consumer) adjust their expectations to
    # the compliance mode via this macro, so it must propagate.
    target_compile_definitions(openvic-dataloader PUBLIC OPENVIC_DATALOADER_ENCODING_COMPLIANCE=1)
elseif(OPENVIC_DATALOADER_COMPLIANCE STREQUAL "error")
    target_compile_definitions(openvic-dataloader PUBLIC OPENVIC_DATALOADER_ENCODING_COMPLIANCE=2)
endif()

# Headless executable and unit tests: built by default only when
# openvic-dataloader is the top-level project (composed builds just want the
# library).
option(OPENVIC_DL_BUILD_HEADLESS "Build the openvic-dataloader headless executable" ${PROJECT_IS_TOP_LEVEL})
option(OPENVIC_DL_BUILD_TESTS "Build the openvic-dataloader unit tests" ${PROJECT_IS_TOP_LEVEL})

if(OPENVIC_DL_BUILD_HEADLESS)
    file(GLOB_RECURSE ovdl_headless_sources CONFIGURE_DEPENDS src/headless/*.cpp)
    add_executable(openvic-dataloader-headless ${ovdl_headless_sources})
    target_compile_definitions(openvic-dataloader-headless PRIVATE OPENVIC_DATALOADER_HEADLESS)
    target_include_directories(openvic-dataloader-headless PRIVATE src/headless)
    target_link_libraries(openvic-dataloader-headless PRIVATE openvic::dataloader)
    set_target_properties(
        openvic-dataloader-headless
        PROPERTIES
            OUTPUT_NAME "openvic-dataloader.headless"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    )
endif()

if(OPENVIC_DL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
