//
// Created by Jan_L on 08.06.2026.
//
#include "mex.hpp"
#include "matlab_logger.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <format>
#include <array>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include "core.h"
#include "sentman.h"
#include "storch.h"
#include "newton.h"
#include "cook.h"
#include "maxwell.h"
#include "schaaf_chambre.h"
#include "mesh_quality.h"
#include "obj_writer.h"
#include "pixel_sizing.h"
#include "remesh.h"
#include "rotatable_mesh_geometry.h"
#include "shading_pipeline.h"
#include "shading_algorithm_factory.h"
#include "hybrid_aero_load_calculator.h"
#include "show_mesh.h"
#include "custom_spdlog_sink.h"

// MexFunction itself must stay in the global namespace -- MATLAB resolves the
// entry point by that exact name -- so pull the toolbox types in individually
// rather than wrapping this translation unit in a namespace.
using vat::AeroConditions;
using vat::IGSIModel;
using vat::gsi_models::Cook;
using vat::gsi_models::Maxwell;
using vat::gsi_models::Newton;
using vat::gsi_models::SchaafChambre;
using vat::gsi_models::Sentman;
using vat::gsi_models::Storch;
using vat::loads::HybridForceTorqueCalculator;
using vat::geometry::MeshQuality;
using vat::geometry::RotatableMeshGeometry;
using vat::remeshing::RemeshOptions;
using vat::shading::ShadingAlgorithmType;
using vat::shading::ShadingPipeline;
using vat::visualization::Hinge;
using vat::visualization::ShowHinges;
using vat::visualization::ShowMeshes;
using vat::visualization::ShowShading;


class MexFunction : public matlab::mex::Function {
public:
    MexFunction() {
        matlab_logger = std::make_unique<MatlabLogger>(getEngine(),LEVEL_DEBUG);
        auto sink = std::make_shared<MatlabSink<std::mutex>>(*matlab_logger);
        auto logger = std::make_shared<spdlog::logger>("global", sink);
        spdlog::set_default_logger(logger);
        set_spdlog_level(LEVEL_DEBUG); // Default log level
    };
    void set_spdlog_level(int level) {
        // Set spdlog level based on the LOG_LEVEL macro
        if (level == LEVEL_DEBUG) {
            spdlog::set_level(spdlog::level::debug);
        }
        else if (level == LEVEL_INFO) {
            spdlog::set_level(spdlog::level::info);
        }
        else if (level == LEVEL_WARN) {
            spdlog::set_level(spdlog::level::warn);
        }
        else if (level == LEVEL_ERROR) {
            spdlog::set_level(spdlog::level::err);
        }
        else {
            spdlog::set_level(spdlog::level::off);
        }
    }

    void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        try {
            validate_input_size_min(inputs, 1);
            validate_argument(inputs, 0, "string", 1);
            // Split on the LAST dot: the class part mirrors the MATLAB package path
            // ("gsi_models.Sentman.new" -> cls "gsi_models.Sentman", cmd "new"),
            // so it may itself contain dots.
            const std::string cmd_string = inputs[0][0];
            size_t dot = cmd_string.rfind('.');
            std::string cls = cmd_string.substr(0, dot);
            std::string cmd = (dot != std::string::npos) ? cmd_string.substr(dot + 1) : "";
            matlab_logger->log(LEVEL_INFO, "Received command: " + cmd + " for class: " +cls, "mex_gateway.cpp", __LINE__);
            if ((cls == "gsi_models.Newton" || cls == "gsi_models.Sentman" || cls == "gsi_models.Storch" || cls == "gsi_models.Maxwell" || cls == "gsi_models.Cook" || cls == "gsi_models.SchaafChambre") && cmd == "delete") {
                validate_input_size(inputs, 2);
                validate_output_size(outputs, 0);
                validate_argument(inputs, 1, "int", 1);
                const int id = inputs[1][0];
                gsi_map.erase(id);
                return;
            }
            if ((cls == "gsi_models.Newton" || cls == "gsi_models.Sentman" || cls == "gsi_models.Storch" || cls == "gsi_models.Maxwell" || cls == "gsi_models.Cook" || cls == "gsi_models.SchaafChambre") && cmd == "calc_aero_force_torque") {
                validate_input_size(inputs, 8);
                validate_output_size(outputs, 2);
                validate_argument(inputs, 1, "int", 1);
                validate_argument(inputs, 2, "double", 1);
                validate_argument(inputs, 3, "double", 3);
                validate_argument(inputs, 4, "double", 3);
                validate_argument(inputs, 5, "double", 3);
                validate_argument(inputs, 6, "double", 1);
                validate_argument(inputs, 7, "int", 1);

                const int handle = inputs[1][0];
                const int aero_cond_handle = inputs[7][0];
                AeroConditions&  aero_conditions = *aero_conditions_map.at(aero_cond_handle);

                const float area__m2 = inputs[2][0];
                const glm::vec3 normal(inputs[3][0], inputs[3][1], inputs[3][2]);
                const glm::vec3 centroid__m(inputs[4][0], inputs[4][1], inputs[4][2]);
                const glm::vec3 v_rel__m_per_s(inputs[5][0], inputs[5][1], inputs[5][2]);
                const float surf_temp__K = inputs[6][0];

                glm::vec3 aero_force__N(0.0f, 0.0f, 0.0f);
                glm::vec3 aero_torque__Nm(0.0f, 0.0f, 0.0f);

                gsi_map.at(handle).get()->calc_aero_force_and_torque(
                    area__m2,
                    normal,
                    centroid__m,
                    v_rel__m_per_s,
                    surf_temp__K,
                    aero_conditions,
                    aero_force__N,
                    aero_torque__Nm
                );

                outputs[0] = factory.createArray({3}, {aero_force__N.x, aero_force__N.y, aero_force__N.z});
                outputs[1] = factory.createArray({3}, {aero_torque__Nm.x, aero_torque__Nm.y, aero_torque__Nm.z});
                return;
            }
            if ((cls == "gsi_models.Newton" || cls == "gsi_models.Sentman" || cls == "gsi_models.Storch" || cls == "gsi_models.Maxwell" || cls == "gsi_models.Cook" || cls == "gsi_models.SchaafChambre") && cmd == "get_gsi_parameter") {
                validate_input_size_min(inputs, 3);
                validate_argument(inputs, 1, "int", 1);
                validate_argument(inputs, 2, "string", 1);
                float value = gsi_map.at(inputs[1][0]).get()->get_gsi_parameter(inputs[2][0]);
                outputs[0] = factory.createScalar<float>(value);
                return;

            }
            if ((cls == "gsi_models.Newton" || cls == "gsi_models.Sentman" || cls == "gsi_models.Storch" || cls == "gsi_models.Maxwell" || cls == "gsi_models.Cook" || cls == "gsi_models.SchaafChambre") && cmd == "set_gsi_parameter") {
                validate_input_size_min(inputs, 4);
                validate_argument(inputs, 1, "int", 1);
                validate_argument(inputs, 2, "string", 1);
                validate_argument(inputs, 3, "float", 1);
                gsi_map.at(inputs[1][0]).get()->set_gsi_parameter(inputs[2][0], inputs[3][0]);
                return;

            }
            if (cls == "gsi_models.Newton") {
                if (cmd == "new") {
                    matlab_logger->log(LEVEL_INFO, "Creating new Newton instance.", "mex_gateway.cpp", __LINE__);
                    validate_input_size_min(inputs, 1);
                    validate_output_size(outputs, 1);
                    gsi_map.insert({gsi_max_id, std::make_unique<Newton>()});
                    outputs[0] = factory.createScalar<int>(gsi_max_id);
                    gsi_max_id++;
                    return;
                }
            }
            if (cls == "gsi_models.Maxwell") {
                if (cmd == "new") {
                    matlab_logger->log(LEVEL_INFO, "Creating new Maxwell instance.", "mex_gateway.cpp", __LINE__);
                    validate_input_size_min(inputs, 2);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "float", 1);
                    float alpha_e = inputs[1][0];
                    gsi_map.insert({gsi_max_id, std::make_unique<Maxwell>(alpha_e)});
                    outputs[0] = factory.createScalar<int>(gsi_max_id);
                    gsi_max_id++;
                    return;
                }
            }
            if (cls == "gsi_models.Cook") {
                if (cmd == "new") {
                    matlab_logger->log(LEVEL_INFO, "Creating new Cook instance.", "mex_gateway.cpp", __LINE__);
                    validate_input_size_min(inputs, 2);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "float", 1);
                    float alpha_e = inputs[1][0];
                    gsi_map.insert({gsi_max_id, std::make_unique<Cook>(alpha_e)});
                    outputs[0] = factory.createScalar<int>(gsi_max_id);
                    gsi_max_id++;
                    return;
                }
            }
            if (cls == "gsi_models.SchaafChambre") {
                if (cmd == "new") {
                    matlab_logger->log(LEVEL_INFO, "Creating new Schaaf-Chambre instance.", "mex_gateway.cpp", __LINE__);
                    validate_input_size_min(inputs, 3);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "float", 1);
                    validate_argument(inputs, 2, "float", 1);
                    float sigma_n = inputs[1][0];
                    float sigma_t = inputs[2][0];
                    gsi_map.insert({gsi_max_id, std::make_unique<SchaafChambre>(sigma_n, sigma_t)});
                    outputs[0] = factory.createScalar<int>(gsi_max_id);
                    gsi_max_id++;
                    return;
                }
            }
            if (cls == "gsi_models.Sentman") {
                if (cmd == "new") {
                    matlab_logger->log(LEVEL_INFO, "Creating new Sentman instance.", "mex_gateway.cpp", __LINE__);
                    validate_input_size_min(inputs, 3);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 2, "float", 1);

                    const int temperature_ratio_method = inputs[1][0];
                    float alpha_e = inputs[2][0];
                    gsi_map.insert({gsi_max_id, std::make_unique<Sentman>(temperature_ratio_method, alpha_e)});
                    outputs[0] = factory.createScalar<int>(gsi_max_id);
                    gsi_max_id++;
                    return;
                }
            }
            if (cls == "gsi_models.Storch") {
                if (cmd == "new") {
                    matlab_logger->log(LEVEL_INFO, "Creating new Storch instance.", "mex_gateway.cpp", __LINE__);
                    validate_input_size_min(inputs, 4);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "float", 1);
                    validate_argument(inputs, 2, "float", 1);
                    validate_argument(inputs, 3, "float", 1);

                    const float V_w = inputs[1][0];
                    const float sigma_n = inputs[2][0];
                    const float sigma_t = inputs[3][0];
                    gsi_map.insert({gsi_max_id, std::make_unique<Storch>(V_w, sigma_n, sigma_t)});
                    outputs[0] = factory.createScalar<int>(gsi_max_id);
                    gsi_max_id++;
                    return;
                }
            }
            if (cls == "AeroConditions") {
                if (cmd == "new") {
                    matlab_logger->log(LEVEL_INFO, "Creating new AeroConditions instance.","mex_gateway.cpp",__LINE__);
                    validate_input_size_min(inputs, 4);
                    validate_argument(inputs, 1, "double", 1);
                    validate_argument(inputs, 2, "double", 1);
                    validate_argument(inputs, 3, "double", 1);

                    float density__kg_per_m3 = inputs[1][0];
                    float temperature__K = inputs[2][0];
                    float particle_mass__kg = inputs[3][0];

                    aero_conditions_map.insert(
                        {aero_conditions_max_id, std::make_unique<AeroConditions>(
                            density__kg_per_m3,
                            temperature__K,
                            particle_mass__kg)}
                        );
                    outputs[0] = factory.createScalar<int>(aero_conditions_max_id);
                    aero_conditions_max_id++;
                    return;
                }
                if (cmd == "set_density") {
                    validate_input_size(inputs, 3);
                    validate_argument(inputs, 1, "int", 1);
                    // double, matching AeroCond.new and the (1,1) double in AeroConditions.m
                    validate_argument(inputs, 2, "double", 1);
                    const int id = inputs[1][0];
                    aero_conditions_map.at(id)->density__kg_per_m3 = inputs[2][0];
                    return;
                }
                if (cmd == "set_T_atmospheric") {
                    validate_input_size(inputs, 3);
                    validate_argument(inputs, 1, "int", 1);
                    // double, matching AeroCond.new and the (1,1) double in AeroConditions.m
                    validate_argument(inputs, 2, "double", 1);
                    const int id = inputs[1][0];
                    aero_conditions_map.at(id)->T_atmospheric__K = inputs[2][0];
                    return;
                }
                if (cmd == "set_particle_mass") {
                    validate_input_size(inputs, 3);
                    validate_argument(inputs, 1, "int", 1);
                    // double, matching AeroCond.new and the (1,1) double in AeroConditions.m
                    validate_argument(inputs, 2, "double", 1);
                    const int id = inputs[1][0];
                    aero_conditions_map.at(id)->particle_mass__kg = inputs[2][0];
                    return;
                }
                if (cmd == "delete") {
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    const int id = inputs[1][0];
                    aero_conditions_map.erase(id);
                    return;
                }
            }
            if (cls == "geometry.RotatableMeshGeometry") {
                if (cmd == "new") {
                    matlab_logger->log(LEVEL_INFO, "Creating new Geometry instance.","mex_gateway.cpp",__LINE__);
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "string", 1);
                    const std::string geometry_path = inputs[1][0];

                    geometry_map.insert({geometry_max_id, std::make_unique<RotatableMeshGeometry>(geometry_path)});
                    outputs[0] = factory.createScalar<int>(geometry_max_id);
                    geometry_max_id++;
                    return;
                }
                if (cmd == "turn_mesh_around_axis") {
                    validate_input_size(inputs, 6);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 2, "int", 1);
                    validate_argument(inputs, 3, "double", 1);
                    validate_argument(inputs, 4, "double", 3);
                    validate_argument(inputs, 5, "double", 3);

                    const int id = inputs[1][0];
                    RotatableMeshGeometry* geometry = geometry_map.at(id).get();
                    std::array<float, 3> origin{{inputs[4][0], inputs[4][1], inputs[4][2]}};
                    std::array<float, 3> axis{{inputs[5][0], inputs[5][1], inputs[5][2]}};
                    geometry->turn_mesh_around_axis(inputs[2][0], inputs[3][0], origin, axis);
                    return;
                }
                if (cmd=="get_vertices") {
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "int", 1);
                    const int id = inputs[1][0];
                    RotatableMeshGeometry* geometry = geometry_map.at(id).get();
                    std::span<const float> vertices = geometry->get_vertices();
                    outputs[0] = factory.createArray({vertices.size()}, vertices.begin(), vertices.end());
                    return;
                }
                if (cmd=="get_num_triangles") {
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "int", 1);
                    const int id = inputs[1][0];
                    RotatableMeshGeometry* geometry = geometry_map.at(id).get();
                    const unsigned int num_triangles = geometry->get_num_triangles();
                    outputs[0] = factory.createScalar<unsigned int>(num_triangles);
                    return;
                }
                if (cmd == "get_mesh_quality") {
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "int", 1);
                    RotatableMeshGeometry& geometry = *geometry_map.at(static_cast<int>(inputs[1][0]));
                    outputs[0] = quality_struct(vat::geometry::compute_mesh_quality(geometry));
                    return;
                }
                if (cmd == "export_obj") {
                    validate_input_size(inputs, 3);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 2, "string", 1);
                    RotatableMeshGeometry& geometry = *geometry_map.at(static_cast<int>(inputs[1][0]));
                    const std::string path = inputs[2][0];
                    vat::geometry::write_obj(geometry, path);
                    return;
                }
                if (cmd == "predict_remesh") {
                    validate_input_size(inputs, 6);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "int", 1);
                    RotatableMeshGeometry& geometry = *geometry_map.at(static_cast<int>(inputs[1][0]));
                    const vat::remeshing::RemeshPrediction p =
                        vat::remeshing::predict_remesh(geometry, remesh_options(inputs, 2));

                    // No num_pixel here: it follows from the narrowest triangles of the real
                    // mesh, which the ideal tiling behind this prediction cannot foretell.
                    matlab::data::StructArray out = factory.createStructArray({1, 1},
                        {"target_edge_length__m", "predicted_triangles", "predicted_memory__MB",
                         "total_area__m2", "exceeds_limit"});
                    out[0]["target_edge_length__m"] = factory.createScalar<double>(p.target_edge_length__m);
                    out[0]["predicted_triangles"] = factory.createScalar<double>(p.predicted_triangles);
                    out[0]["predicted_memory__MB"] = factory.createScalar<double>(p.predicted_memory__bytes / 1.0e6);
                    out[0]["total_area__m2"] = factory.createScalar<double>(p.total_area__m2);
                    out[0]["exceeds_limit"] = factory.createScalar<bool>(
                        p.predicted_triangles > vat::remeshing::REMESH_MAX_TRIANGLES);
                    outputs[0] = std::move(out);
                    return;
                }
                if (cmd == "remesh") {
                    validate_input_size(inputs, 6);
                    validate_output_size(outputs, 2);
                    validate_argument(inputs, 1, "int", 1);
                    RotatableMeshGeometry& geometry = *geometry_map.at(static_cast<int>(inputs[1][0]));
                    vat::remeshing::RemeshResult result =
                        vat::remeshing::remesh(geometry, remesh_options(inputs, 2));
                    const vat::remeshing::RemeshReport& r = result.report;

                    matlab::data::StructArray report = factory.createStructArray({1, 1},
                        {"target_edge_length__m", "area_change__percent", "suggested_num_pixel", "before",
                         "after", "triangles_per_mesh_before", "triangles_per_mesh_after", "repair"});
                    report[0]["target_edge_length__m"] = factory.createScalar<double>(r.target_edge_length__m);
                    report[0]["area_change__percent"] = factory.createScalar<double>(r.area_change__percent);
                    // Per algorithm: CoP gains accuracy from pixels well past the point
                    // where Binary stops gaining anything.
                    matlab::data::StructArray num_pixel = factory.createStructArray({1, 1}, {"cop", "binary"});
                    num_pixel[0]["cop"] = factory.createScalar<double>(
                        vat::shading::suggest_num_pixel(*result.geometry, ShadingAlgorithmType::CoP));
                    num_pixel[0]["binary"] = factory.createScalar<double>(
                        vat::shading::suggest_num_pixel(*result.geometry, ShadingAlgorithmType::Binary));
                    report[0]["suggested_num_pixel"] = std::move(num_pixel);
                    report[0]["before"] = quality_struct(r.before);
                    report[0]["after"] = quality_struct(r.after);
                    report[0]["triangles_per_mesh_before"] = row_vector(r.triangles_per_mesh_before);
                    report[0]["triangles_per_mesh_after"] = row_vector(r.triangles_per_mesh_after);

                    matlab::data::StructArray repair = factory.createStructArray({1, 1},
                        {"merged_vertices", "removed_unused_vertices", "removed_duplicate_triangles",
                         "removed_degenerate_triangles", "split_vertices"});
                    repair[0]["merged_vertices"] = factory.createScalar<double>(r.repair.merged_vertices);
                    repair[0]["removed_unused_vertices"] = factory.createScalar<double>(r.repair.removed_unused_vertices);
                    repair[0]["removed_duplicate_triangles"] = factory.createScalar<double>(r.repair.removed_duplicate_triangles);
                    repair[0]["removed_degenerate_triangles"] = factory.createScalar<double>(r.repair.removed_degenerate_triangles);
                    repair[0]["split_vertices"] = factory.createScalar<double>(r.repair.split_vertices);
                    report[0]["repair"] = std::move(repair);

                    geometry_map.insert({geometry_max_id, std::move(result.geometry)});
                    outputs[0] = factory.createScalar<int>(geometry_max_id);
                    geometry_max_id++;
                    outputs[1] = std::move(report);
                    return;
                }
                if (cmd == "delete") {
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    const int id = inputs[1][0];
                    geometry_map.erase(id);
                    return;
                }
            }
            if (cls == "shading.ShadingPipeline") {
                if (cmd == "new") {
                    matlab_logger->log(LEVEL_INFO, "Creating new Shading instance.","mex_gateway.cpp",__LINE__);
                    // num_pixel is optional: without it the pipeline chooses it from the mesh.
                    if (inputs.size() != 3 && inputs.size() != 4) {
                        validate_input_size(inputs, 4);
                    }
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 2, "int", 1);

                    const int id = inputs[1][0];
                    RotatableMeshGeometry& geometry = *geometry_map.at(id);
                    const int shading_key = inputs[2][0];
                    ShadingAlgorithmType algorithm_type;
                    switch (shading_key) {
                        case 0:
                            algorithm_type = ShadingAlgorithmType::Binary;
                            break;
                        case 1:
                            algorithm_type = ShadingAlgorithmType::CoP;
                            break;
                        default:
                            matlab_logger->log(LEVEL_ERROR, "Unknown shading algorithm type: " + std::to_string(shading_key),"mex_gateway.cpp",__LINE__);
                            throw std::invalid_argument(std::string("Unknown shading algorithm type: ") + std::to_string(shading_key));
                    };
                    std::unique_ptr<ShadingPipeline> pipeline;
                    if (inputs.size() == 4) {
                        validate_argument(inputs, 3, "int", 1);
                        pipeline = std::make_unique<ShadingPipeline>(geometry, algorithm_type, inputs[3][0]);
                    } else {
                        pipeline = std::make_unique<ShadingPipeline>(geometry, algorithm_type);
                    }
                    shading_pipeline_map.insert({shading_pipeline_max_id, std::move(pipeline)});
                    outputs[0] = factory.createScalar<int>(shading_pipeline_max_id);
                    shading_pipeline_max_id++;
                    return;
                }
                if (cmd == "get_num_pixel") {
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "int", 1);
                    const ShadingPipeline& pipeline = *shading_pipeline_map.at(static_cast<int>(inputs[1][0]));
                    outputs[0] = factory.createScalar<double>(pipeline.get_num_pixel());
                    return;
                }
                if (cmd=="shade"){
                    validate_input_size(inputs, 3);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 2, "double", 3);

                    const int id = inputs[1][0];
                    ShadingPipeline* pipeline = shading_pipeline_map.at(id).get();

                    glm::vec3 velocity__m_per_s(inputs[2][0], inputs[2][1], inputs[2][2]);
                    std::vector<float> triangle_visibility = pipeline->shade(glm::normalize(velocity__m_per_s));

                    outputs[0] = factory.createArray({triangle_visibility.size()}, triangle_visibility.begin(), triangle_visibility.end());
                    return;
                }
                if (cmd=="delete") {
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    const int id = inputs[1][0];
                    shading_pipeline_map.erase(id);
                    return;
                }
            }
            if (cls == "loads.HybridForceTorqueCalculator") {
                if (cmd == "new") {
                    validate_input_size(inputs, 4);
                    validate_output_size(outputs, 1);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 2, "int", 1);
                    validate_argument(inputs, 3, "int", 1);

                    const int geometry_id = inputs[1][0];
                    const int shading_pipeline_id = inputs[2][0];
                    const int gsi_id = inputs[3][0];

                    hybrid_aero_load_calculator_map.insert(
                        {hybrid_aero_max_id,
                        std::make_unique<HybridForceTorqueCalculator>(
                            *geometry_map.at(geometry_id),
                            *shading_pipeline_map.at(shading_pipeline_id),
                            *gsi_map.at(gsi_id)
                        )}
                    );

                    outputs[0] = factory.createScalar<int>(hybrid_aero_max_id);
                    hybrid_aero_max_id++;
                    return;
                }
                if (cmd=="calc_aero_load") {
                    validate_input_size(inputs, 5);
                    validate_output_size(outputs, 2);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 2, "double", 3);
                    validate_argument(inputs, 3, "double", 1);
                    validate_argument(inputs, 4, "int", 1);

                    const int aero_cond_id = inputs[4][0];
                    const int id = inputs[1][0];
                    HybridForceTorqueCalculator* calculator = hybrid_aero_load_calculator_map.at(id).get();
                    AeroConditions&  aero_conditions = *aero_conditions_map.at(aero_cond_id);

                    glm::vec3 velocity__m_per_s(inputs[2][0], inputs[2][1], inputs[2][2]);
                    const float surface_temp__K = inputs[3][0];
                    glm::vec3 torque__Nm(0.0f, 0.0f, 0.0f);
                    glm::vec3 force__N(0.0f, 0.0f, 0.0f);
                    calculator->calc_aero_torque_force(velocity__m_per_s, surface_temp__K, aero_conditions, torque__Nm, force__N);
                    outputs[0] = factory.createArray({3}, {force__N.x, force__N.y, force__N.z});
                    outputs[1] = factory.createArray({3}, {torque__Nm.x, torque__Nm.y, torque__Nm.z});
                    return;
                }
                if (cmd=="delete") {
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    const int id = inputs[1][0];
                    hybrid_aero_load_calculator_map.erase(id);
                    return;
                }
            }
            if (cls == "visualization") {
                if (cmd == "show_shading") {
                    validate_input_size(inputs, 5);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 3, "double", 3);
                    validate_argument(inputs, 4, "logical", 1);

                    const int geometry_id = inputs[1][0];
                    RotatableMeshGeometry& geometry = *geometry_map.at(geometry_id);
                    validate_argument(inputs, 2, "float", geometry.get_num_triangles());

                    matlab::data::TypedArray<float> const typed_array = inputs[2];
                    std::vector<float> triangle_visibility(typed_array.begin(), typed_array.end());
                    glm::vec3 velocity__m_per_s(inputs[3][0], inputs[3][1], inputs[3][2]);

                    ShowShading(geometry, triangle_visibility, velocity__m_per_s,
                        view_options(inputs, 4));
                    return;
                }
                if (cmd == "show_meshes") {
                    validate_input_size(inputs, 3);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 2, "logical", 1);

                    const int geometry_id = inputs[1][0];
                    RotatableMeshGeometry& geometry = *geometry_map.at(geometry_id);

                    ShowMeshes(geometry, view_options(inputs, 2));
                    return;
                }
                if (cmd == "show_hinges") {
                    validate_input_size(inputs, 6);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    validate_argument(inputs, 5, "logical", 1);

                    const int geometry_id = inputs[1][0];
                    RotatableMeshGeometry& geometry = *geometry_map.at(geometry_id);

                    // The MATLAB wrapper flattens its struct array into three parallel
                    // arguments: mesh ids (1xN), origins (3xN) and axes (3xN). MATLAB is
                    // column-major, so iterating a 3xN yields x,y,z per hinge in order.
                    const int num_hinges = static_cast<int>(inputs[2].getNumberOfElements());
                    validate_argument(inputs, 2, "int", num_hinges);
                    validate_argument(inputs, 3, "double", 3 * num_hinges);
                    validate_argument(inputs, 4, "double", 3 * num_hinges);

                    matlab::data::TypedArray<int> const mesh_id_array = inputs[2];
                    matlab::data::TypedArray<double> const origin_array = inputs[3];
                    matlab::data::TypedArray<double> const axis_array = inputs[4];
                    const std::vector<int> mesh_ids(mesh_id_array.begin(), mesh_id_array.end());
                    const std::vector<double> origins(origin_array.begin(), origin_array.end());
                    const std::vector<double> axes(axis_array.begin(), axis_array.end());

                    std::vector<Hinge> hinges;
                    hinges.reserve(static_cast<std::size_t>(num_hinges));
                    for (std::size_t i = 0; i < static_cast<std::size_t>(num_hinges); ++i) {
                        Hinge hinge;
                        hinge.mesh_id = mesh_ids[i];
                        hinge.origin__m = glm::vec3(
                            static_cast<float>(origins[3 * i + 0]),
                            static_cast<float>(origins[3 * i + 1]),
                            static_cast<float>(origins[3 * i + 2]));
                        hinge.axis = glm::vec3(
                            static_cast<float>(axes[3 * i + 0]),
                            static_cast<float>(axes[3 * i + 1]),
                            static_cast<float>(axes[3 * i + 2]));
                        hinges.push_back(hinge);
                    }

                    ShowHinges(geometry, hinges, view_options(inputs, 5));
                    return;
                }
            }
            if (cls == "none") {
                if (cmd == "setLogLevel")
                {
                    validate_input_size(inputs, 2);
                    validate_output_size(outputs, 0);
                    validate_argument(inputs, 1, "int", 1);
                    int log_level = static_cast<int>(inputs[1][0]);
                    matlab_logger->set_log_level(log_level);
                    matlab_logger->log(LEVEL_DEBUG, "Setting log level to: " + std::to_string(log_level),"mex_gateway.cpp",__LINE__);
                    set_spdlog_level(log_level);
                    return;
                }
            }
            matlab_logger->log(LEVEL_ERROR, "Unknown command: " + cmd + " for class: " + cls,"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Unknown command: " + cmd + " for class: " + cls);
        } catch (const std::exception& e) {
            // matlab_logger->log the error to MATLAB console and rethrow so MATLAB receives a proper error
            matlab_logger->log(LEVEL_ERROR, e.what(),"mex_gateway.cpp",__LINE__);
            throw; // propagate the exception back to MATLAB instead of silently returning with no outputs
        }
    }

private:
    // Extracted utility checkers to keep subclass neat
    static bool is_int(matlab::mex::ArgumentList& inputs, int idx) {
        const matlab::data::ArrayType type = inputs[idx].getType();
        return (type == matlab::data::ArrayType::INT8 ||
                type == matlab::data::ArrayType::INT16 ||
                type == matlab::data::ArrayType::INT32 ||
                type == matlab::data::ArrayType::INT64);
    };

    static bool is_double(matlab::mex::ArgumentList& inputs, int idx) {
        return (inputs[idx].getType() == matlab::data::ArrayType::DOUBLE &&
        inputs[idx].getType() != matlab::data::ArrayType::COMPLEX_DOUBLE);
    };

    static bool is_float(matlab::mex::ArgumentList& inputs, int idx) {
        return (inputs[idx].getType() == matlab::data::ArrayType::SINGLE &&
        inputs[idx].getType() != matlab::data::ArrayType::COMPLEX_SINGLE);
    };

    static bool is_uint(matlab::mex::ArgumentList& inputs, int idx) {
        const matlab::data::ArrayType type = inputs[idx].getType();
        return (type == matlab::data::ArrayType::UINT8 ||
                type == matlab::data::ArrayType::UINT16 ||
                type == matlab::data::ArrayType::UINT32 ||
                type == matlab::data::ArrayType::UINT64);
    };

    static bool is_string(matlab::mex::ArgumentList& inputs, int idx) {
        return inputs[idx].getType() == matlab::data::ArrayType::MATLAB_STRING;

    };

    // Every view takes the same trailing options argument, so build it in one place.
    static vat::visualization::ViewOptions view_options(matlab::mex::ArgumentList& inputs, int idx) {
        matlab::data::TypedArray<bool> const flags = inputs[idx];
        vat::visualization::ViewOptions options;
        options.show_triangle_edges = flags[0];
        return options;
    };

    // Remesh arguments as the MATLAB wrapper passes them, starting at idx: triangle count
    // (int), edge length (double), feature angle (double), iterations (int).
    RemeshOptions remesh_options(matlab::mex::ArgumentList& inputs, int idx) {
        validate_argument(inputs, idx, "int", 1);
        validate_argument(inputs, idx + 1, "double", 1);
        validate_argument(inputs, idx + 2, "double", 1);
        validate_argument(inputs, idx + 3, "int", 1);
        const int count = inputs[idx][0];
        const int iterations = inputs[idx + 3][0];
        if (count < 0 || iterations < 0) {
            throw std::invalid_argument("TriangleCount and Iterations must not be negative");
        }
        RemeshOptions options;
        options.target_triangle_count = static_cast<unsigned int>(count);
        options.target_edge_length__m = static_cast<float>(static_cast<double>(inputs[idx + 1][0]));
        options.feature_angle__deg = static_cast<float>(static_cast<double>(inputs[idx + 2][0]));
        options.iterations = static_cast<unsigned int>(iterations);
        return options;
    }

    matlab::data::TypedArray<double> row_vector(const std::vector<unsigned int>& values) {
        std::vector<double> as_double(values.begin(), values.end());
        return factory.createArray<double>({1, as_double.size()}, as_double.data(), as_double.data() + as_double.size());
    }

    matlab::data::StructArray distribution_struct(const vat::geometry::Distribution& d) {
        matlab::data::StructArray out = factory.createStructArray({1, 1}, {"min", "p05", "median", "p95", "max"});
        out[0]["min"] = factory.createScalar<double>(d.min);
        out[0]["p05"] = factory.createScalar<double>(d.p05);
        out[0]["median"] = factory.createScalar<double>(d.median);
        out[0]["p95"] = factory.createScalar<double>(d.p95);
        out[0]["max"] = factory.createScalar<double>(d.max);
        return out;
    }

    matlab::data::StructArray quality_struct(const MeshQuality& q) {
        matlab::data::StructArray out = factory.createStructArray({1, 1},
            {"num_triangles", "num_degenerate", "total_area__m2", "mean_area__m2",
             "bounding_sphere_radius__m", "area__m2", "aspect_ratio", "min_altitude__m"});
        out[0]["num_triangles"] = factory.createScalar<double>(q.num_triangles);
        out[0]["num_degenerate"] = factory.createScalar<double>(q.num_degenerate);
        out[0]["total_area__m2"] = factory.createScalar<double>(q.total_area__m2);
        out[0]["mean_area__m2"] = factory.createScalar<double>(q.mean_area__m2);
        out[0]["bounding_sphere_radius__m"] = factory.createScalar<double>(q.bounding_sphere_radius__m);
        out[0]["area__m2"] = distribution_struct(q.area__m2);
        out[0]["aspect_ratio"] = distribution_struct(q.aspect_ratio);
        out[0]["min_altitude__m"] = distribution_struct(q.min_altitude__m);
        return out;
    }

    static bool is_logical(matlab::mex::ArgumentList& inputs, int idx) {
        return inputs[idx].getType() == matlab::data::ArrayType::LOGICAL;

    };

    void validate_argument(matlab::mex::ArgumentList& inputs,
                           int idx,
                           const std::string& expected_type,
                           int expected_size) {
        if (expected_type == "int" && !is_int(inputs, idx)) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected argument at index {} to be an integer.", idx),"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Argument type mismatch: expected integer.");
        }
        if (expected_type == "double" && !is_double(inputs, idx)) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected argument at index {} to be a double.", idx),"mex_gateway.cpp", __LINE__);
            throw std::invalid_argument("Argument type mismatch: expected double.");
        }
        if (expected_type == "float" && !is_float(inputs, idx)) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected argument at index {} to be a float.", idx),"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Argument type mismatch: expected float.");
        }
        if (expected_type == "uint" && !is_uint(inputs, idx)) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected argument at index {} to be an unsigned integer.", idx),"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Argument type mismatch: expected unsigned integer.");
        }
        if (expected_type == "logical" && !is_logical(inputs, idx)) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected argument at index {} to be a logical.", idx),"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Argument type mismatch: expected logical.");
        }
        if (expected_type == "string" && !is_string(inputs, idx)) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected argument at index {} to be a string.", idx),"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Argument type mismatch: expected string.");
        }
        if (expected_size != inputs[idx].getNumberOfElements()) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected argument at index {} to have size {}, but got size {}.", idx, expected_size, inputs[idx].getNumberOfElements()),"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Argument size mismatch.");
        }
    };

    void validate_input_size(matlab::mex::ArgumentList& inputs, int expected_size) {
        if (inputs.size() != expected_size) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected {} input arguments, but got {}.", expected_size, inputs.size()),"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Input argument count mismatch.");
        }
    };

    void validate_output_size(matlab::mex::ArgumentList& outputs, int expected_size) {
        if (outputs.size() != expected_size) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected {} output arguments, but got {}.", expected_size, outputs.size()),"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Output argument count mismatch.");
        }
    };

    void validate_input_size_min(matlab::mex::ArgumentList& inputs, int min_size) {
        if (inputs.size() < min_size) {
            matlab_logger->log(LEVEL_ERROR, std::format("Expected at least {} input arguments, but got {}.", min_size, inputs.size()),"mex_gateway.cpp",__LINE__);
            throw std::invalid_argument("Not enough input arguments.");
        }
    };

    matlab::data::ArrayFactory factory;
    std::unique_ptr<MatlabLogger> matlab_logger;
    std::unordered_map<int, std::unique_ptr<IGSIModel>> gsi_map;
    std::unordered_map<int, std::unique_ptr<AeroConditions>> aero_conditions_map;
    std::unordered_map<int, std::unique_ptr<RotatableMeshGeometry>> geometry_map;
    std::unordered_map<int, std::unique_ptr<ShadingPipeline>> shading_pipeline_map;
    std::unordered_map<int, std::unique_ptr<HybridForceTorqueCalculator>> hybrid_aero_load_calculator_map;
    int gsi_max_id = 0;
    int aero_conditions_max_id = 0;
    int geometry_max_id = 0;
    int shading_pipeline_max_id = 0;
    int hybrid_aero_max_id = 0;
};