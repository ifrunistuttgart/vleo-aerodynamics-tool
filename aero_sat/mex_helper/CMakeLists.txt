find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY MEX_LIBRARY ENG_LIBRARY)

add_library(MexHelper
    BaseObjectMex.h
    MatlabLogger.cpp
    Handle.cpp)

target_compile_features(MexHelper PRIVATE cxx_std_20)
target_include_directories(MexHelper
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(MexHelper PUBLIC Matlab::mex Matlab::mx Matlab::eng)

add_library(Sentman_mex_test::MexHelper ALIAS MexHelper)
