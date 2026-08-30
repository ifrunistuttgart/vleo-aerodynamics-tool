find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

add_library(aero_load_calculator
        hybrid_aero_load_calculator.cpp
        hybrid_aero_load_calculator.h
        Igsi_model.h
        Ishading_pipeline.h
        gpu_aero_load_calculator.cpp
        gpu_aero_load_calculator.h)

target_compile_features(aero_load_calculator PRIVATE cxx_std_20)
target_include_directories(aero_load_calculator 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(aero_load_calculator PUBLIC glm::glm spdlog::spdlog core opengl_abstractions)

add_library(AeroSat::aero_load_calculator ALIAS aero_load_calculator)