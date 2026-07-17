# snitch honors these as plain variables (CMP0077 NEW via
# openvic_setup_base_flags). Only deltas from upstream defaults are set;
# everything else (SNITCH_DEFINE_MAIN, timings, reporters, ...) already
# matches upstream. Exceptions are additionally forced off at compile time by
# snitch itself when _HAS_EXCEPTIONS=0 / -fno-exceptions is in effect.
set(SNITCH_WITH_EXCEPTIONS OFF)
set(SNITCH_WITH_MULTITHREADING OFF)
openvic_declare_dep(snitch
    REPO snitch-org/snitch
    SHA d6632123cc8d13bdbc5cd60fd6741b9e0f635e82 # v1.2.5+13
    SHA256 09fcd66cba6ab93cc9f43eee9b2d6c0988e6fb8791da9ab2eec612e951221db6
)
FetchContent_MakeAvailable(snitch)

file(GLOB_RECURSE ovdl_tests_sources CONFIGURE_DEPENDS src/*.cpp)

add_executable(openvic-dataloader-tests ${ovdl_tests_sources})
target_compile_definitions(openvic-dataloader-tests PRIVATE OPENVIC_DATALOADER_TESTS)
# Tests include private implementation headers (e.g. <detail/NullBuff.hpp>),
# so add the library's src dir alongside the tests' own.
target_include_directories(
    openvic-dataloader-tests
    PRIVATE src "${CMAKE_CURRENT_SOURCE_DIR}/../src/openvic-dataloader"
)
target_link_libraries(openvic-dataloader-tests PRIVATE openvic::dataloader snitch::snitch)
set_target_properties(
    openvic-dataloader-tests
    PROPERTIES
        OUTPUT_NAME "openvic-dataloader.tests"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

add_test(NAME openvic-dataloader-tests COMMAND openvic-dataloader-tests)
