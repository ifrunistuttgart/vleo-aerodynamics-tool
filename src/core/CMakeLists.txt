find_package(glm REQUIRED CONFIG)

add_library(core INTERFACE)

target_include_directories(core INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>
)
target_link_libraries(core INTERFACE glm::glm)

add_library(AeroSat::core ALIAS core)