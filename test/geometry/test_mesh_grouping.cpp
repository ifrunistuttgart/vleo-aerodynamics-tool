#include <gtest/gtest.h>
#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "static_mesh_geometry.h"
#include "test_helpers.h"

using namespace vat;

namespace {

// Captures whatever the loader logs, so the group warning can be asserted on rather
// than just assumed. The sink is attached to the default logger for one load and
// removed again, because spdlog's default logger is global state shared by every test.
class LogCapture {
public:
    LogCapture() : m_sink(std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(64)) {
        m_logger = spdlog::default_logger();
        m_previous_level = m_logger->level();
        m_logger->sinks().push_back(m_sink);
        m_logger->set_level(spdlog::level::debug);
    }

    ~LogCapture() {
        std::vector<spdlog::sink_ptr>& sinks = m_logger->sinks();
        sinks.erase(std::remove(sinks.begin(), sinks.end(), m_sink), sinks.end());
        m_logger->set_level(m_previous_level);
    }

    bool contains(const std::string& needle) const {
        for (const std::string& message : m_sink->last_formatted()) {
            if (message.find(needle) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

private:
    std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> m_sink;
    std::shared_ptr<spdlog::logger> m_logger;
    spdlog::level::level_enum m_previous_level;
};

unsigned int SumTriangles(IGeometryShadingData& geometry) {
    const std::span<const unsigned int> per_mesh = geometry.get_num_triangles_per_mesh();
    return std::accumulate(per_mesh.begin(), per_mesh.end(), 0u);
}

} // namespace

/*
 * One mesh of the toolbox is one `o` object of an .obj file, and that is the unit
 * turn_mesh_around_axis() rotates. Groups (`g`) are a second, parallel way of dividing
 * an .obj that assimp does not merge with objects, so a file built from groups yields
 * one mesh per box face and every mesh_id then addresses a fragment of a part. The
 * importer cannot repair that, so it warns instead -- these tests pin down both the
 * expected mesh split and the warning.
 */

// The example model separates its parts with `o`, which is the supported way.
TEST(MeshGroupingTest, ObjWithObjectsGivesOneMeshPerObject) {
    const std::string obj_path =
        GetTestDataPath(__FILE__, "../../examples/geometry_files/shuttlecock_15k.obj");
    StaticMeshGeometry geometry(obj_path);

    const std::span<const std::string> names = geometry.get_mesh_names();
    ASSERT_EQ(names.size(), 5);
    EXPECT_EQ(names[0], "Body.004");
    EXPECT_EQ(names[1], "wing_bottom.004");
    EXPECT_EQ(names[2], "wing_left.004");
    EXPECT_EQ(names[3], "wing_right.004");
    EXPECT_EQ(names[4], "wing_up.004");

    // One model matrix per mesh, so every mesh_id is independently rotatable.
    EXPECT_EQ(geometry.get_model_matrices().size(), 5);
    EXPECT_EQ(geometry.get_num_triangles_per_mesh().size(), 5);

    // The split must not lose or duplicate triangles.
    EXPECT_EQ(SumTriangles(geometry), geometry.get_num_triangles());
}

// A well-formed file must not nag the user.
TEST(MeshGroupingTest, ObjWithObjectsDoesNotWarn) {
    const std::string obj_path =
        GetTestDataPath(__FILE__, "../../examples/geometry_files/shuttlecock_15k.obj");

    LogCapture log;
    StaticMeshGeometry geometry(obj_path);

    EXPECT_FALSE(log.contains("Do not use groups"));
}

// soar_satellite.obj also uses `o` only, with five panels.
TEST(MeshGroupingTest, SecondObjWithObjectsGivesOneMeshPerObject) {
    const std::string obj_path =
        GetTestDataPath(__FILE__, "../../matlab/examples/geometries/soar_satellite.obj");
    StaticMeshGeometry geometry(obj_path);

    EXPECT_EQ(geometry.get_mesh_names().size(), 5);
    EXPECT_EQ(geometry.get_model_matrices().size(), 5);
    EXPECT_EQ(SumTriangles(geometry), geometry.get_num_triangles());
}

// An .stl carries no structure at all: one unnamed mesh, which has to fall back to an
// index-based name rather than an empty legend entry.
TEST(MeshGroupingTest, StlBecomesASingleNamedMesh) {
    const std::string stl_path = GetTestDataPath(__FILE__, "geometries/Assembled_ISS.stl");
    StaticMeshGeometry geometry(stl_path);

    const std::span<const std::string> names = geometry.get_mesh_names();
    ASSERT_EQ(names.size(), 1);
    EXPECT_EQ(names[0], "Mesh 0");
    EXPECT_EQ(geometry.get_num_triangles_per_mesh()[0], geometry.get_num_triangles());
}

// grouped_box.obj is the badly exported case: five parts written as `g` groups, each
// holding the six `o` faces of a box. It still loads, but as thirty single-face meshes,
// so the user has to be told.
TEST(MeshGroupingTest, ObjWithGroupsWarns) {
    const std::string obj_path = GetTestDataPath(__FILE__, "geometries/grouped_box.obj");

    LogCapture log;
    StaticMeshGeometry geometry(obj_path);

    EXPECT_TRUE(log.contains("Do not use groups"))
        << "a grouped .obj must tell the user to export with 'o' instead";

    // Documents the consequence the warning is about: thirty meshes of two triangles,
    // one per box face, rather than the five parts the file names.
    EXPECT_EQ(geometry.get_mesh_names().size(), 30);
    EXPECT_EQ(geometry.get_num_triangles(), 60);
    for (const unsigned int count : geometry.get_num_triangles_per_mesh()) {
        EXPECT_EQ(count, 2);
    }
}

// get_mesh_names() has to stay aligned with the other per-mesh arrays, because the
// visualization pairs them by index to label each mesh.
TEST(MeshGroupingTest, PerMeshArraysAreAllTheSameLength) {
    const std::string obj_path =
        GetTestDataPath(__FILE__, "../../examples/geometry_files/shuttlecock_15k.obj");
    StaticMeshGeometry geometry(obj_path);

    const std::size_t num_meshes = geometry.get_mesh_names().size();
    EXPECT_EQ(geometry.get_num_triangles_per_mesh().size(), num_meshes);
    EXPECT_EQ(geometry.get_model_matrices().size(), num_meshes);
}
