find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(assimp REQUIRED CONFIG)

add_library(satellites static_mesh_satellite.h static_mesh_satellite.cpp rotatable_mesh_satellite.h rotatable_mesh_satellite.cpp)

target_compile_features(satellites PRIVATE cxx_std_20)
target_include_directories(satellites
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)

target_link_libraries(satellites PUBLIC glm::glm core)
target_link_libraries(satellites PRIVATE spdlog::spdlog assimp::assimp)

add_library(AeroSat::satellites ALIAS satellites)
