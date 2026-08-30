find_package(spdlog CONFIG REQUIRED)

add_executable(benchmark benchmark.cpp)

target_link_libraries(benchmark PRIVATE
        gsi
        satellite
        shading_pipeline
        spdlog::spdlog
)
