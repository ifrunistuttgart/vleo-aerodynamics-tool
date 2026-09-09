find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

add_library(gsi_models
		sentman.cpp
		sentman.h
		cook.cpp
		cook.h
        maxwell.cpp
        maxwell.h
        schaaf_chambre.cpp
        schaaf_chambre.h
		newton.cpp
		newton.h
		storch.cpp
		storch.h)

target_compile_features(gsi_models PRIVATE cxx_std_20)
target_include_directories(gsi_models
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(gsi_models PUBLIC loads)
target_link_libraries(gsi_models PRIVATE glm::glm spdlog::spdlog)

add_library(AeroSat::gsi_models ALIAS gsi_models)