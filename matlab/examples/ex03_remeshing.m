%% Example 3: Mesh Quality and Remeshing
% Demonstrates how to inspect CAD mesh quality, remesh into uniform near-equilateral
% triangles, save and reload cached meshes, and leverage automatic shading raster sizing.
%
% Why remesh:
%   CAD exports typically contain high-aspect-ratio sliver triangles along curved
%   edges and thin rims (e.g. wing leading edges). Because shading determines visibility
%   per triangle, narrow slivers can cause aliasing and require very high pixel counts.
%   Uniform remeshing resolves shadow boundaries cleanly, enables Center-of-Pressure
%   (CoP) shading at lower raster resolutions, and significantly speeds up calculations.
%
% Prerequisites: make sure you have built the matlab binaries with:
%   pixi run build-matlab
% And that you have loaded the Matlab project from Vat.prj

clear; close all;
vat.setLogLevel("warn");

%% 1. Inspect imported mesh quality
% We start with a multi-part shuttlecock satellite geometry.
geometry_folder = fullfile(fileparts(mfilename('fullpath')), 'geometries');
raw_file        = fullfile(geometry_folder, 'shuttlecock_15k.obj');
raw_geometry    = vat.geometry.RotatableMeshGeometry(raw_file);

quality_raw = raw_geometry.get_mesh_quality();
fprintf('--- Imported Geometry Quality ---\n');
fprintf('Triangles             : %d\n', quality_raw.num_triangles);
fprintf('Aspect ratio median   : %.2f (equilateral = 1.0)\n', quality_raw.aspect_ratio.median);
fprintf('Aspect ratio max      : %.1f\n', quality_raw.aspect_ratio.max);
fprintf('Narrowest width (p05) : %.3f mm\n\n', 1e3 * quality_raw.min_altitude__m.p05);

% Inspect the imported mesh in 3D (press 'T' in viewer to toggle triangle wireframe)
% vat.visualization.show_meshes(raw_geometry);

%% 2. Predict remeshing size and memory
% predict_remesh is fast and lightweight. It estimates the triangle count, target edge
% length, and memory footprint without modifying geometry.
prediction = raw_geometry.predict_remesh(TriangleCount=20000);
fprintf('--- Remeshing Prediction ---\n');
fprintf('Target triangles      : %d (predicted: %d)\n', 20000, prediction.predicted_triangles);
fprintf('Target edge length    : %.2f mm\n', 1e3 * prediction.target_edge_length__m);
fprintf('Estimated memory      : %.1f MB\n\n', prediction.predicted_memory__MB);

%% 3. Remesh geometry
% remesh replaces triangles with uniform, near-equilateral elements while preserving
% sharp feature edges, surface area, and sub-mesh identities (mesh IDs / names).
[remeshed, report] = raw_geometry.remesh(TriangleCount=20000);
fprintf('--- Remeshing Report ---\n');
fprintf('Triangles after       : %d\n', report.after.num_triangles);
fprintf('Aspect ratio median   : %.2f, max: %.2f\n', ...
    report.after.aspect_ratio.median, report.after.aspect_ratio.max);
fprintf('Surface area change   : %+.3f %%\n', report.area_change__percent);
fprintf('Suggested num_pixel   : CoP = %d, Binary = %d\n\n', ...
    report.suggested_num_pixel.cop, report.suggested_num_pixel.binary);

%% 4. Multi-body structure & hinge preservation
% Sub-mesh IDs (0 for body, 1-4 for wings) are preserved exactly. Any hinges
% defined on the original mesh work on the remeshed model without modification.
hinges(1).mesh_id = 1;  hinges(1).origin = [-0.15,  0.10,  0.05];  hinges(1).axis = [0,  1,  0]; % bottom wing
hinges(2).mesh_id = 2;  hinges(2).origin = [-0.15, -0.05,  0.10];  hinges(2).axis = [0,  0,  1]; % left wing
hinges(3).mesh_id = 3;  hinges(3).origin = [-0.15,  0.05, -0.10];  hinges(3).axis = [0,  0, -1]; % right wing
hinges(4).mesh_id = 4;  hinges(4).origin = [-0.15, -0.10, -0.05];  hinges(4).axis = [0, -1,  0]; % upper wing

% Deflect wings by 20 deg on both geometries
for i = 1:numel(hinges)
    raw_geometry.turn_mesh_around_axis(hinges(i).mesh_id, deg2rad(20), hinges(i).origin, hinges(i).axis);
    remeshed.turn_mesh_around_axis(hinges(i).mesh_id, deg2rad(20), hinges(i).origin, hinges(i).axis);
end

%% 5. Export and reload cached mesh
% Remeshing takes a few seconds, but loading an exported .obj takes milliseconds.
% In standard workflows, remesh once, export, and load the remeshed file thereafter.
cached_file = fullfile(tempdir, 'shuttlecock_remeshed_20k.obj');
remeshed.export_obj(cached_file);
cached_geometry = vat.geometry.RotatableMeshGeometry(cached_file);

%% 6. Aerodynamic loads & performance comparison
% Compare raw vs. remeshed mesh under flow at an angle of attack (AoA = 15 deg).
% - Raw mesh: requires manual high pixel count (e.g. 4000) with Binary shading to capture thin slivers.
% - Remeshed: uses optimal automatic pixel count with high-accuracy CoP shading.
conditions      = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
surface_temp__K = 300.0;
gsi_model       = vat.gsi_models.Sentman(1, 0.95);
v_rel           = 7800 * [cosd(15), 0, sind(15)];

% Raw pipeline (Binary shading, high pixel count)
pipeline_raw = vat.shading.ShadingPipeline(raw_geometry, 0, 4000);
calc_raw     = vat.loads.HybridForceTorqueCalculator(raw_geometry, pipeline_raw, gsi_model);

% Remeshed pipeline (CoP shading, automatic pixel count)
pipeline_remeshed = vat.shading.ShadingPipeline(cached_geometry, 1);
calc_remeshed     = vat.loads.HybridForceTorqueCalculator(cached_geometry, pipeline_remeshed, gsi_model);

% Warm-up and benchmark execution time
tic; [F_raw, T_raw] = calc_raw.calc_aero_load(v_rel, surface_temp__K, conditions); t_raw = toc;
tic; [F_rem, T_rem] = calc_remeshed.calc_aero_load(v_rel, surface_temp__K, conditions); t_rem = toc;

fprintf('--- Aerodynamic Results Comparison ---\n');
fprintf('Raw mesh      (Binary, 4000 px) : F = [%+.4e, %+.4e, %+.4e] N | Time: %.2f ms\n', F_raw, t_raw * 1e3);
fprintf('Remeshed mesh (CoP,    auto px) : F = [%+.4e, %+.4e, %+.4e] N | Time: %.2f ms\n', F_rem, t_rem * 1e3);
fprintf('Relative force difference       : %.3f %%\n', 100 * norm(F_rem - F_raw) / norm(F_raw));
