find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(assimp REQUIRED CONFIG)

add_library(satellite static_mesh_satellite.h static_mesh_satellite.cpp rotatable_mesh_satellite.h rotatable_mesh_satellite.cpp)

target_compile_features(satellite PRIVATE cxx_std_20)
target_include_directories(satellite
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)

target_link_libraries(satellite PUBLIC glm::glm core)
target_link_libraries(satellite PRIVATE spdlog::spdlog assimp::assimp)

add_library(AeroSat::satellite ALIAS satellite)
