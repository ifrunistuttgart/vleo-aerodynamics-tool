// Remeshes a model into near-equilateral triangles and writes it as .obj.
//
//   remesh_obj                                  the example shuttlecock, 20000 triangles
//   remesh_obj <in> <triangle_count> <out.obj>  any model assimp reads
//
// Remesh once, keep the written file, and load that from then on: remeshing takes
// seconds, loading takes milliseconds.
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <source_location>
#include <string>

#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "geometry.h"
#include "obj_writer.h"
#include "pixel_sizing.h"
#include "remesh.h"

namespace {

// Resolves a filename relative to this source file's own location on disk.
std::filesystem::path get_path(const std::string& filename) {
	std::filesystem::path source_file(std::source_location::current().file_name());
	return source_file.parent_path() / filename;
}

void print_quality(const char* label, const vat::geometry::MeshQuality& q) {
	std::printf("%-7s %8u triangles   aspect ratio median %6.2f  p95 %6.2f   narrowest width p05 %.3g mm\n",
		label, q.num_triangles, q.aspect_ratio.median, q.aspect_ratio.p95, 1e3 * q.min_altitude__m.p05);
}

} // namespace

int main(int argc, char** argv) {
	if (argc != 1 && argc != 4) {
		std::printf("usage: remesh_obj [<in> <triangle_count> <out.obj>]\n");
		return EXIT_FAILURE;
	}
	const std::string in = argc == 4 ? argv[1] : get_path("../geometry_files/shuttlecock_15k.obj").string();
	const unsigned int count = argc == 4 ? static_cast<unsigned int>(std::stoul(argv[2])) : 20000u;
	// By default next to the executable, i.e. inside the (ignored) build tree.
	const std::string out = argc == 4 ? argv[3]
		: (std::filesystem::path(argv[0]).parent_path() / "shuttlecock_remeshed.obj").string();

	vat::geometry::StaticMeshGeometry geometry(in);

	vat::remeshing::RemeshOptions options;
	options.target_triangle_count = count;

	// Cheap: the same repair and sizing as remesh(), without the remeshing.
	const vat::remeshing::RemeshPrediction prediction = vat::remeshing::predict_remesh(geometry, options);
	std::printf("predicted: %u triangles of edge %.3g mm, ~%.1f MB\n", prediction.predicted_triangles,
		1e3 * prediction.target_edge_length__m, prediction.predicted_memory__bytes / 1e6);

	vat::remeshing::RemeshResult result = vat::remeshing::remesh(geometry, options);
	const vat::remeshing::RemeshReport& report = result.report;
	print_quality("before", report.before);
	print_quality("after", report.after);
	std::printf("area change %+.3g %%\n", report.area_change__percent);
	std::printf("num_pixel to use: CoP %u, Binary %u (or omit it and let ShadingPipeline choose)\n",
		vat::shading::suggest_num_pixel(*result.geometry, vat::shading::ShadingAlgorithmType::CoP),
		vat::shading::suggest_num_pixel(*result.geometry, vat::shading::ShadingAlgorithmType::Binary));

	vat::geometry::write_obj(*result.geometry, out);
	std::printf("written to %s\n", out.c_str());
	return EXIT_SUCCESS;
}
