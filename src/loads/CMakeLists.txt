find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

add_library(loads hybrid_aero_load_calculator.cpp hybrid_aero_load_calculator.h Igsi_model.h Ishading_pipeline.h)

target_compile_features(loads PRIVATE cxx_std_20)
target_include_directories(loads 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(loads PUBLIC glm::glm spdlog::spdlog core)

add_library(Vat::loads ALIAS loads)