[![Builds](https://github.com/OpenVicProject/OpenVic-Dataloader/actions/workflows/builds.yml/badge.svg)](https://github.com/OpenVicProject/OpenVic-Dataloader/actions/workflows/builds.yml)

# OpenVic-Dataloader
Repo of the OpenVic-Dataloader Library for [OpenVic](https://github.com/OpenVicProject/OpenVic)

## Quickstart Guide
For detailed instructions, view the OpenVic Contributor Quickstart Guide [here](https://github.com/OpenVicProject/OpenVic/blob/master/docs/contribution-quickstart-guide.md)

## Required
* [CMake](https://cmake.org/) 3.28+
* [Ninja](https://ninja-build.org/)

## Build Instructions
1. Pick a configure preset from `CMakePresets.json` (`windows-x64-md`, `windows-x64-mt`, `linux-x64`, `macos-universal`).
2. Run `cmake --preset <preset>` in the project root (dependencies are fetched automatically; no submodules needed).
3. Run `cmake --build --preset <preset>-debug` (or `<preset>-release`). The static library, headless executable, and unit tests land in `out/build/<preset>/bin/<Config>/`.
4. Run the tests with `ctest --preset <preset>-debug`.

The headless executable and tests are built by default in standalone builds; disable with `-DOPENVIC_DL_BUILD_HEADLESS=OFF` / `-DOPENVIC_DL_BUILD_TESTS=OFF`. Encoding compliance is selected with `-DOPENVIC_DATALOADER_COMPLIANCE=<loose|error_replace|error>`.

## Link Instructions
Use CMake: `add_subdirectory(openvic-dataloader)` and link against `openvic::dataloader`.
