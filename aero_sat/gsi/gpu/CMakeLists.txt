find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

add_library(gsi_gpu
        newton.cpp

)

target_compile_features(gsi_gpu PRIVATE cxx_std_20)
target_include_directories(gsi_gpu
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(gsi_gpu PUBLIC aero_load_calculator)
target_link_libraries(gsi_gpu PRIVATE glm::glm spdlog::spdlog)

add_library(AeroSat::gsi::gpu ALIAS gsi_gpu)