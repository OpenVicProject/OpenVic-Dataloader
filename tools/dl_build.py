def exists(env):
    return True


def options(opts):
    from SCons.Variables import BoolVariable, EnumVariable

    opts.Add(BoolVariable("build_ovdl_tests", "Build and run the openvic dataloader tests", True))
    opts.Add(BoolVariable("run_ovdl_tests", "Run the openvic simulation unit tests", False))
    opts.Add(BoolVariable("build_ovdl_library", "Build the openvic simulation library.", False))
    opts.Add(BoolVariable("build_ovdl_headless", "Build the openvic simulation headless executable", True))
    opts.Add(
        EnumVariable(
            "compliance_type", "Type of encoding compliance to build with", "loose", ("loose", "error_replace", "error")
        )
    )


def generate(env):
    if not env.is_standalone:
        env["run_ovdl_tests"] = False
        env["build_ovdl_tests"] = False
        env["build_ovdl_library"] = True
        env["build_ovdl_headless"] = False

    if env["run_ovdl_tests"]:
        env["build_ovdl_tests"] = True

    if env["build_ovdl_tests"]:
        env["build_ovdl_library"] = True

    match env["compliance_type"]:
        case "error_replace":
            env.Append(CPPDEFINES=[("OPENVIC_DATALOADER_ENCODING_COMPLIANCE", 1)])
        case "error":
            env.Append(CPPDEFINES=[("OPENVIC_DATALOADER_ENCODING_COMPLIANCE", 2)])
