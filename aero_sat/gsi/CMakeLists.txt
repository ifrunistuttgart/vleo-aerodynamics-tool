find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

add_library(gsi
		sentman.cpp
		sentman.h
		cook.cpp
		cook.h
        maxwell.cpp
        maxwell.h
        schaaf_chambre.cpp
        schaaf_chambre.h)

target_compile_features(gsi PRIVATE cxx_std_20)
target_include_directories(gsi
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(gsi PUBLIC aero_load_calculator)
target_link_libraries(gsi PRIVATE glm::glm spdlog::spdlog)

add_library(AeroSat::gsi ALIAS gsi)