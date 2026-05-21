find_package(glm CONFIG REQUIRED)

add_library(visualization show_mesh.h show_mesh.cpp)

target_compile_features(visualization PRIVATE cxx_std_20)
target_include_directories(visualization
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>
)

target_link_libraries(visualization PUBLIC core glm::glm)

add_library(AeroSat::visualization ALIAS visualization)