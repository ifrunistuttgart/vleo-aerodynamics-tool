%% remeshing a satellite
% Replaces a model's triangles with near-equilateral ones of one common size,
% saves the result, and uses it with an automatically chosen num_pixel.
%
% Why: a triangle is lit or shadowed as a whole, so a long thin sliver
% across a shadow edge counts as fully lit or fully dark. CAD exports are
% full of slivers. After remeshing, shading needs far fewer pixels for the
% same accuracy, and every triangle costs about the same.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

%% 1. Import and inspect
obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'soar_satellite.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

% Aspect ratio is 1 for an equilateral triangle. The narrowest width
% (min_altitude__m), not the area, decides whether the raster resolves a
% triangle.
quality = geometry.get_mesh_quality();
fprintf('imported : %6d triangles, aspect ratio median %.2f, max %.0f\n', ...
    quality.num_triangles, quality.aspect_ratio.median, quality.aspect_ratio.max);

% The triangle edges are drawn over the surfaces (T toggles them).
vat.visualization.show_meshes(geometry);

%% 2. Choose a size
% Give either TriangleCount or EdgeLength [m]. predict_remesh is cheap, so
% try sizes with it first. Every load evaluation pays for every triangle,
% so ask for what you need: remesh warns above 250k triangles and refuses
% more than 1M. The real count lands somewhat above the prediction -- sharp
% edges and panel rims are kept exactly and resist coarsening.
prediction = geometry.predict_remesh(TriangleCount=20000);
fprintf('predicted: %6d triangles of edge %.1f mm, %.1f MB\n', ...
    prediction.predicted_triangles, 1e3 * prediction.target_edge_length__m, ...
    prediction.predicted_memory__MB);

%% 3. Remesh
% The input is left as it is. Every mesh keeps its mesh_id and name, so
% hinge definitions made for the imported file still apply. No triangle
% is flipped: its winding decides which side the flow loads.
[remeshed, report] = geometry.remesh(TriangleCount=20000);
fprintf('remeshed : %6d triangles, aspect ratio median %.2f, max %.2f, area change %+.2f %%\n', ...
    report.after.num_triangles, report.after.aspect_ratio.median, ...
    report.after.aspect_ratio.max, report.area_change__percent);

vat.visualization.show_meshes(remeshed);

%% 4. Save it, and load that from now on
% Remeshing takes seconds; loading the result takes milliseconds. Here it
% goes to tempdir -- in your own work, keep it next to the original.
remeshed_file = fullfile(tempdir, 'soar_satellite_remeshed.obj');
remeshed.export_obj(remeshed_file);
geometry = vat.geometry.RotatableMeshGeometry(remeshed_file);

%% 5. Shading and loads
% Leave out num_pixel and the pipeline picks it from the mesh: about seven
% pixels across a triangle for CoP (1), three for Binary (0). CoP is the
% more accurate of the two on a well-shaped mesh.
pipeline = vat.shading.ShadingPipeline(geometry, 1);
fprintf('num_pixel: %d\n', pipeline.get_num_pixel());

conditions = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
calculator = vat.loads.HybridForceTorqueCalculator(geometry, pipeline, ...
    vat.gsi_models.Sentman(1, 0.9));

v_rel__m_per_s = [7800, 0, 0];
[force__N, torque__Nm] = calculator.calc_aero_load(v_rel__m_per_s, 300.0, conditions);
fprintf('Force  [N]  : %+.4e %+.4e %+.4e\n', force__N);
fprintf('Torque [Nm] : %+.4e %+.4e %+.4e\n', torque__Nm);
