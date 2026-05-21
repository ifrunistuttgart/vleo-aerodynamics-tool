# CMakeList.txt: CMake-Projektdatei der obersten Ebene. Führen Sie hier die globale Konfiguration aus,
# und schließen Sie Unterprojekte ein.
#
cmake_minimum_required (VERSION 3.15...4.1)

# Aktivieren Sie Hot Reload für MSVC-Compiler, sofern unterstützt.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if(DEFINED ENV{VCPKG_ROOT})
    include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
endif()

project (AeroSat)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Schließen Sie Unterprojekte ein.
add_subdirectory("aero_sat")
add_subdirectory("main")

if(CMAKE_PROJECT_NAME STREQUAL AeroSat AND BUILD_TESTING)
    enable_testing()
    add_subdirectory("test")
endif()