find_package(glm REQUIRED CONFIG)

add_library(shading shading_pipeline.h shading_pipeline.cpp shading_algorithm_factory.h shading_algorithm_factory.cpp)

target_compile_features(shading PRIVATE cxx_std_20)
target_include_directories(shading
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(shading PUBLIC core glm::glm loads Vat::gl)

add_subdirectory(binary_shader)
add_subdirectory(cop_shader)

add_library(Vat::shading ALIAS shading)
