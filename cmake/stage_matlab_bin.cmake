# Stages MexGateway.mexw64 and every DLL it needs into matlab/bin, so that the
# matlab/ folder is self-contained and can be copied to a machine that has no
# pixi environment (or no toolbox checkout) at all.
#
# Why this is needed: MATLAB loads MexGateway.mexw64 with no idea the pixi
# environment exists, so its ~65 DLL dependencies (which only live in the pixi
# env's Library/bin) would be unfindable. Windows checks a DLL's own folder
# before PATH, so once every dependency sits next to MexGateway.mexw64, no PATH
# change is needed. The MSVC runtime (msvcp140/vcruntime140) is picked up too,
# so the target machine does not even need the VC++ redistributable.
#
# Uses file(GET_RUNTIME_DEPENDENCIES) rather than a hardcoded DLL list because it
# walks the actual PE import table recursively -- it stays correct automatically
# when VTK/assimp/etc. versions change, rather than silently going stale.
#
# IMPORTANT -- TARGET_FILE must NOT live in DEST_DIR.
# GET_RUNTIME_DEPENDENCIES searches the module's *own* directory before it
# searches DIRECTORIES. When MexGateway.mexw64 was linked directly into
# matlab/bin, every dependency therefore resolved to the copy already staged
# there and was then copied onto itself -- so the staged DLLs were frozen at
# whatever the first build produced and never picked up a `pixi update` again.
# MexGateway is now linked into the build tree (which contains no DLLs, since
# all AeroSat libraries are static) and staged here. The check below enforces it.
#
# Invoked in script mode (see matlab/CMakeLists.txt):
#   cmake -DTARGET_FILE=... -DDEST_DIR=... -DSEARCH_DIR=... -P stage_matlab_bin.cmake

if(NOT DEFINED TARGET_FILE OR NOT DEFINED DEST_DIR OR NOT DEFINED SEARCH_DIR)
    message(FATAL_ERROR "stage_matlab_bin.cmake requires TARGET_FILE, DEST_DIR and SEARCH_DIR to be set")
endif()

get_filename_component(DEST_DIR "${DEST_DIR}" ABSOLUTE)
get_filename_component(target_dir "${TARGET_FILE}" DIRECTORY)
get_filename_component(target_dir "${target_dir}" ABSOLUTE)

if(target_dir STREQUAL DEST_DIR)
    message(FATAL_ERROR
        "stage_matlab_bin.cmake: TARGET_FILE must not be linked into DEST_DIR "
        "(${DEST_DIR}); dependencies would resolve to the already-staged copies "
        "and never refresh. See the comment at the top of this file.")
endif()

# Normalize paths to forward slashes before matching the exclude regexes below.
# None of them contain a path separator, so OLD and NEW behave identically here --
# the guard is only to silence CMP0207's author warning without hard-requiring
# CMake 4.3, which an unguarded cmake_policy(SET) would do.
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

file(GET_RUNTIME_DEPENDENCIES
    MODULES "${TARGET_FILE}"
    DIRECTORIES "${SEARCH_DIR}"
    RESOLVED_DEPENDENCIES_VAR resolved_deps
    UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
    # Skip Windows' own "API set" forwarder DLLs and anything in System32 -- those
    # come from the OS / VC++ redistributable, not from our environment. Also skip
    # MATLAB's own runtime libraries (libmx/libmex/libmat/libMatlabDataArray/
    # libMatlabEngine), which live in MATLAB's install directory and are always on
    # MATLAB's own search path whenever it loads a MEX file.
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*" "libmx.*" "libmex.*" "libmat.*" "libMatlab.*"
    POST_EXCLUDE_REGEXES ".*[Ss]ystem32.*"
)

if(unresolved_deps)
    message(WARNING "stage_matlab_bin.cmake: could not resolve dependencies for ${TARGET_FILE}: ${unresolved_deps}")
endif()

# Belt and braces: if anything still resolved out of the staging directory, the
# refresh bug described above is back and the staged DLLs would go stale silently.
foreach(dep IN LISTS resolved_deps)
    get_filename_component(dep_dir "${dep}" DIRECTORY)
    get_filename_component(dep_dir "${dep_dir}" ABSOLUTE)
    if(dep_dir STREQUAL DEST_DIR)
        message(FATAL_ERROR
            "stage_matlab_bin.cmake: ${dep} resolved out of the staging directory "
            "instead of ${SEARCH_DIR}; staged DLLs would never refresh.")
    endif()
endforeach()

file(MAKE_DIRECTORY "${DEST_DIR}")

# Copy the mex file itself alongside its dependencies, so matlab/bin is complete.
set(to_stage "${TARGET_FILE}" ${resolved_deps})

# A MATLAB session that has loaded the mex file keeps it and every dependency
# memory-mapped, and Windows opens a mapped image without write sharing. Collect
# everything that could not be replaced and report it in one actionable message
# rather than failing on the first file. ONLY_IF_DIFFERENT means unchanged DLLs
# are never rewritten, so in practice only the mex file itself ever collides.
set(locked_files "")
set(num_updated 0)
foreach(src IN LISTS to_stage)
    get_filename_component(name "${src}" NAME)
    file(COPY_FILE "${src}" "${DEST_DIR}/${name}" ONLY_IF_DIFFERENT RESULT copy_error)
    if(copy_error)
        list(APPEND locked_files "${name}")
    endif()
endforeach()

# Remove build artifacts we no longer produce and DLLs that are no longer
# dependencies, so a dropped dependency cannot leave a stale copy behind. Safe
# because matlab/bin is owned entirely by this script and is gitignored.
set(expected_names "")
foreach(src IN LISTS to_stage)
    get_filename_component(name "${src}" NAME)
    list(APPEND expected_names "${name}")
endforeach()

get_filename_component(mex_stem "${TARGET_FILE}" NAME_WE)
file(GLOB staged_files "${DEST_DIR}/*.dll" "${DEST_DIR}/${mex_stem}.*")
foreach(staged IN LISTS staged_files)
    get_filename_component(name "${staged}" NAME)
    if(NOT name IN_LIST expected_names)
        file(REMOVE "${staged}")
        message(STATUS "stage_matlab_bin.cmake: removed stale ${name}")
    endif()
endforeach()

if(locked_files)
    list(JOIN locked_files "\n    " locked_list)
    message(FATAL_ERROR
        "stage_matlab_bin.cmake: could not write these files into ${DEST_DIR}:\n"
        "    ${locked_list}\n"
        "They are locked by a running MATLAB that has loaded the toolbox.\n"
        "Run  clear mex  in MATLAB -- you do not need to close it -- and build again.")
endif()

list(LENGTH resolved_deps num_deps)
message(STATUS "stage_matlab_bin.cmake: staged ${mex_stem} + ${num_deps} runtime DLL(s) into ${DEST_DIR}")
