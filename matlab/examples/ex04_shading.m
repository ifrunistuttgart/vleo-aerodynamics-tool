%% Example 4: Shading and the hybrid calculator
% Parts of a satellite hide others from the flow. Besides the per-pixel
% calculator, VAT has a second way to account for that: a shading pipeline
% decides for every triangle whether the flow reaches it, and the hybrid
% calculator evaluates the GSI model per visible triangle. Visibility is
% all or nothing per triangle, which is fast and easy to inspect, but a
% triangle that is only partly in the shadow counts fully or not at all.
%
% show_shading opens an interactive 3D window and waits until it is closed.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

conditions      = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
surface_temp__K = 300;
gsi_model       = vat.gsi_models.Sentman(1, 0.9);

%% Geometry: a satellite with one panel turned by 45 deg
obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'soar_satellite.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);
geometry.turn_mesh_around_axis(0, deg2rad(45), [-0.15, 0, -0.05], [0, 0, -1]);

% Flow at 20 deg angle of attack and 40 deg sideslip, so the body shades
% part of the panels.
v_rel = 7800 * [cosd(40) * cosd(20), sind(40) * cosd(20), sind(20)];

%% 1. Shading pipeline: which triangles does the flow reach?
% algorithm 0 = Binary: a triangle is visible if any pixel shows it.
% algorithm 1 = CoP:    a triangle is visible if its centre is not hidden.
num_pixel = 2000;
pipeline_binary = vat.shading.ShadingPipeline(geometry, 0, num_pixel);
pipeline_cop    = vat.shading.ShadingPipeline(geometry, 1, num_pixel);

visibility_binary = pipeline_binary.shade(v_rel);   % one value per triangle, 1 or 0
visibility_cop    = pipeline_cop.shade(v_rel);
fprintf('Visible triangles: %d (Binary), %d (CoP) of %d\n', ...
    nnz(visibility_binary), nnz(visibility_cop), geometry.get_num_triangles());

vat.visualization.show_shading(geometry, visibility_cop, v_rel);

%% 2. Hybrid calculator: GSI model per visible triangle
hybrid_binary = vat.loads.HybridForceTorqueCalculator(geometry, pipeline_binary, gsi_model);
hybrid_cop    = vat.loads.HybridForceTorqueCalculator(geometry, pipeline_cop, gsi_model);
pixel         = vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, num_pixel);

[F_binary, T_binary] = hybrid_binary.calc_aero_load(v_rel, surface_temp__K, conditions);
[F_cop,    T_cop]    = hybrid_cop.calc_aero_load(v_rel, surface_temp__K, conditions);
[F_pixel,  T_pixel]  = pixel.calc_aero_load(v_rel, surface_temp__K, conditions);

fprintf('\n                 force [N]                               torque [Nm]\n');
fprintf('hybrid, Binary  %+.4e %+.4e %+.4e    %+.4e %+.4e %+.4e\n', F_binary, T_binary);
fprintf('hybrid, CoP     %+.4e %+.4e %+.4e    %+.4e %+.4e %+.4e\n', F_cop, T_cop);
fprintf('per pixel       %+.4e %+.4e %+.4e    %+.4e %+.4e %+.4e\n', F_pixel, T_pixel);

% The three differ where shadow edges cut through triangles. The scripts in
% examples/benchmarks compare the methods in detail.
