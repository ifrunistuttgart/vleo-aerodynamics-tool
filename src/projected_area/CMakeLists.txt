find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

add_library(
    projected_area
    pixel_projected_area.cpp
    pixel_projected_area.h
    projected_area_gsi_model.cpp
    projected_area_gsi_model.h
    shaders/projected_area_glsl.h
)

target_compile_features(projected_area PRIVATE cxx_std_20)
target_include_directories(
    projected_area
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(projected_area PUBLIC loads core)
target_link_libraries(projected_area PRIVATE glm::glm spdlog::spdlog)

add_library(Vat::projected_area ALIAS projected_area)
