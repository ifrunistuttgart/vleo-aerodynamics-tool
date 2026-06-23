find_package(glm REQUIRED CONFIG)

add_library(shading_pipeline shading_pipeline.h shading_pipeline.cpp shading_algorithm_factory.h shading_algorithm_factory.cpp)

target_compile_features(shading_pipeline PRIVATE cxx_std_20)
target_include_directories(shading_pipeline
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(shading_pipeline PUBLIC core glm::glm aero_load_calculator)

add_subdirectory(opengl)
add_subdirectory(binary_shader)
add_subdirectory(cop_shader)

add_library(AeroSat::shading_pipeline ALIAS shading_pipeline)
