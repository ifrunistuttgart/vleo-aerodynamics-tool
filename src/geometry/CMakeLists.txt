find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(assimp REQUIRED CONFIG)

add_library(geometry static_mesh_geometry.h static_mesh_geometry.cpp rotatable_mesh_geometry.h rotatable_mesh_geometry.cpp)

target_compile_features(geometry PRIVATE cxx_std_20)
target_include_directories(geometry
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)

target_link_libraries(geometry PUBLIC glm::glm core)
target_link_libraries(geometry PRIVATE spdlog::spdlog assimp::assimp)

add_library(AeroSat::geometry ALIAS geometry)
