# OpenVic-Dataloader
Repo of the OpenVic-Dataloader Library for [OpenVic](https://github.com/OpenVicProject/OpenVic)

## Quickstart Guide
For detailed instructions, view the OpenVic Contributor Quickstart Guide [here](https://github.com/OpenVicProject/OpenVic/blob/master/docs/contribution-quickstart-guide.md)

## Required
* [scons](https://scons.org/)

## Build Instructions
1. Install [scons](https://scons.org/) for your system.
2. Run the command `git submodule update --init --recursive` to retrieve all related submodules.
3. Run `scons build_ovdl_library=yes` in the project root, you should see a libopenvic-dataloader file in `bin`.

## Link Instructions
1. Call `ovdl_env = SConscript("openvic-dataloader/SConstruct")`
2. Use the values stored in the `ovdl_env.openvic_dataloader` to link and compile against:

| Variable Name | Description                               | Correlated ENV variable   |
| ---           | ---                                       | ---                       |
| `LIBPATH`     | Library path list                         | `env["LIBPATH"]`          |
| `LIBS`        | Library files names in the library paths  | `env["LIBS"]`             |
| `INCPATH`     | Library include files                     | `env["CPPPATH"]`          |
