find_package(glm REQUIRED CONFIG)

add_library(shading shading_pipeline.h shading_pipeline.cpp shading_algorithm_factory.h shading_algorithm_factory.cpp pixel_sizing.h pixel_sizing.cpp visibility_reducer.h visibility_reducer.cpp)

target_compile_features(shading PRIVATE cxx_std_20)
target_include_directories(shading
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(shading PUBLIC core glm::glm loads Vat::gl)
# compute_mesh_quality(), for choosing num_pixel from the mesh.
target_link_libraries(shading PRIVATE geometry)

add_subdirectory(binary_shader)
add_subdirectory(cop_shader)

add_library(Vat::shading ALIAS shading)
