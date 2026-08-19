# Copies a target's resolved runtime DLL dependencies (spdlog, VTK, assimp, GLEW,
# GLFW, etc. from the pixi/conda-forge environment) next to it in DEST_DIR.
#
# Why this is needed: MexGateway.mexw64 is loaded by a plain MATLAB session that has
# no idea the pixi environment exists, so its ~15 DLL dependencies (which only exist
# in the pixi env's Library/bin) would otherwise be unfindable. Windows checks a
# DLL's own folder before PATH, so once every dependency is copied next to
# MexGateway.mexw64, no PATH change is needed at all.
#
# Uses file(GET_RUNTIME_DEPENDENCIES) instead of a hardcoded DLL list because it
# walks the actual PE import table recursively -- it stays correct automatically
# when VTK/assimp/etc. versions change, rather than silently going stale.
#
# Invoked in script mode (see matlab/CMakeLists.txt):
#   cmake -DTARGET_FILE=... -DDEST_DIR=... -DSEARCH_DIR=... -P copy_runtime_deps.cmake

if(NOT DEFINED TARGET_FILE OR NOT DEFINED DEST_DIR OR NOT DEFINED SEARCH_DIR)
    message(FATAL_ERROR "copy_runtime_deps.cmake requires TARGET_FILE, DEST_DIR, and SEARCH_DIR to be set")
endif()

# GET_RUNTIME_DEPENDENCIES normalizes paths before matching regexes/exclusions;
# this is the behavior we want (avoids spurious mismatches from mixed \ and /).
cmake_policy(SET CMP0207 NEW)

file(GET_RUNTIME_DEPENDENCIES
    MODULES "${TARGET_FILE}"
    DIRECTORIES "${SEARCH_DIR}"
    RESOLVED_DEPENDENCIES_VAR resolved_deps
    UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
    # Skip Windows' own "API set" forwarder DLLs and anything already in System32 --
    # those come from the OS / VC++ redistributable, not from our environment, and
    # shouldn't be vendored alongside the mex file. Also skip MATLAB's own runtime
    # libraries (libmx/libmex/libmat/libMatlabDataArray/libMatlabEngine) -- those
    # live in MATLAB's own install directory, which MATLAB always has on its own
    # search path whenever it loads a MEX file, so they're never actually missing.
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*" "libmx.*" "libmex.*" "libmat.*" "libMatlab.*"
    POST_EXCLUDE_REGEXES ".*[Ss]ystem32.*"
)

if(unresolved_deps)
    message(WARNING "copy_runtime_deps.cmake: could not resolve dependencies for ${TARGET_FILE}: ${unresolved_deps}")
endif()

file(MAKE_DIRECTORY "${DEST_DIR}")
foreach(dep IN LISTS resolved_deps)
    file(COPY "${dep}" DESTINATION "${DEST_DIR}")
endforeach()

list(LENGTH resolved_deps num_deps)
message(STATUS "copy_runtime_deps.cmake: copied ${num_deps} runtime DLL(s) into ${DEST_DIR}")
