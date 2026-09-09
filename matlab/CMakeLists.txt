find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY MEX_LIBRARY ENG_LIBRARY)
find_package(glm REQUIRED CONFIG)

message(STATUS "MATLAB root directory found: ${Matlab_ROOT_DIR}.")

matlab_add_mex(
    NAME MexGateway
    SRC mex_gateway.cpp
    LINK_TO ${Matlab_MX_LIBRARY} ${Matlab_MEX_LIBRARY} ${Matlab_ENG_LIBRARY}
            glm::glm gsi_models core satellites shading loads visualization
)

set(AEROSAT_MATLAB_BIN_DIR "${PROJECT_SOURCE_DIR}/matlab/bin")

# MexGateway deliberately links into the build tree, NOT into matlab/bin.
# matlab/bin is a staging directory populated entirely by the step below. Keeping
# the two apart matters twice over:
#
#   1. file(GET_RUNTIME_DEPENDENCIES) searches a module's own directory before the
#      directories it is given. With the mex file linked into matlab/bin, every
#      dependency resolved to the copy already staged there and got copied onto
#      itself, so the staged DLLs never refreshed after the first build.
#   2. A MATLAB session that has loaded the toolbox keeps matlab/bin/MexGateway.mexw64
#      mapped, which made the *link* fail with a bare LNK1168. Linking elsewhere
#      moves that failure into the staging script, which reports which files are
#      locked and that `clear mex` releases them.
if (WIN32 AND DEFINED ENV{CONDA_PREFIX})
    # Driven by a stamp file rather than a POST_BUILD step so the build system
    # actually tracks the staging directory. The stamp lives inside matlab/bin, so
    # deleting that folder re-triggers staging; a POST_BUILD step would not, because
    # it only runs when the mex file itself relinks. Depending on pixi.lock covers
    # the other way matlab/bin goes stale: `pixi update` changes the DLLs in the
    # environment without touching any source file.
    #
    # Staging costs ~6 s (it content-compares ~170 MB), which is why it is skipped
    # when nothing relevant changed rather than run unconditionally.
    set(stage_stamp "${AEROSAT_MATLAB_BIN_DIR}/.aerosat_staged")
    add_custom_command(
        OUTPUT "${stage_stamp}"
        COMMAND ${CMAKE_COMMAND}
            "-DTARGET_FILE=$<TARGET_FILE:MexGateway>"
            "-DDEST_DIR=${AEROSAT_MATLAB_BIN_DIR}"
            "-DSEARCH_DIR=$ENV{CONDA_PREFIX}/Library/bin"
            -P "${PROJECT_SOURCE_DIR}/cmake/stage_matlab_bin.cmake"
        COMMAND ${CMAKE_COMMAND} -E touch "${stage_stamp}"
        DEPENDS MexGateway "${PROJECT_SOURCE_DIR}/pixi.lock"
        COMMENT "Staging MexGateway.mexw64 + runtime DLLs into matlab/bin"
        VERBATIM
    )
    add_custom_target(stage_matlab_bin ALL DEPENDS "${stage_stamp}")
    message(STATUS "Mex file will be staged into ${AEROSAT_MATLAB_BIN_DIR}")
else ()
    # Without the pixi environment we cannot find the DLLs to vendor, so matlab/bin
    # would hold a mex file MATLAB cannot load. Warn rather than fail silently.
    message(WARNING
        "Not staging MexGateway's runtime DLLs: this is only supported on Windows "
        "inside a pixi environment (CONDA_PREFIX is unset). The mex file will be "
        "left in the build tree and will not load from a plain MATLAB session.")
endif ()
