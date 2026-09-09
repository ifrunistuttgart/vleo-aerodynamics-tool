find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

add_library(gsi_cpu
        sentman.cpp
        cook.cpp
        maxwell.cpp
        schaaf_chambre.cpp
        newton.cpp
        storch.cpp
)

target_compile_features(gsi_cpu PRIVATE cxx_std_20)
target_include_directories(gsi_cpu
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(gsi_cpu PUBLIC aero_load_calculator)
target_link_libraries(gsi_cpu PRIVATE glm::glm spdlog::spdlog)

add_library(AeroSat::gsi::cpu ALIAS gsi_cpu)