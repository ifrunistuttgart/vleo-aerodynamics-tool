find_package(spdlog CONFIG REQUIRED)

add_executable(perf_probe perf_probe.cpp)

target_link_libraries(perf_probe PRIVATE
	gsi
	satellite
	shading_pipeline
	spdlog::spdlog
)
