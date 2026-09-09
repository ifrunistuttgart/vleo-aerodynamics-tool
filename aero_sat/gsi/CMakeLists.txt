find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

add_library(gsi INTERFACE)

target_include_directories(gsi
	INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(gsi INTERFACE glm::glm spdlog::spdlog aero_load_calculator)

add_subdirectory(cpu)
add_subdirectory(gpu)

target_link_libraries(gsi INTERFACE AeroSat::gsi::cpu)
target_link_libraries(gsi INTERFACE AeroSat::gsi::gpu)

add_library(AeroSat::gsi ALIAS gsi)