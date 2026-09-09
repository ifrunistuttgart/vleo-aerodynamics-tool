%% Computation time vs mesh fidelity, for several render resolutions
%
% Times one full force/torque evaluation on the shuttlecock model at increasing
% triangle counts, once per render resolution, and plots the result.
%
% Requires the MEX bindings to be built for the current source:
%   pixi run build-matlab

clear; clc;

script_dir = fileparts(mfilename('fullpath'));
addpath(fullfile(script_dir, '..'));
addpath(fullfile(script_dir, '..', 'bin'));

% Per-call logging goes through the MATLAB engine and would dominate the timings.
setLogLevel("error");

mesh_name = "shuttlecock_240.obj";
resolution = 1000;

% Atomic oxygen at roughly 300 km, flow along +x in the body frame.
v_rel__m_per_s  = [7800, 0, 0];
surface_temp__K = 300.0;
aero_conditions = AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
gsi_model_cpu   = Newton();
gsi_model_gpu   = GPUNewton();

satellite         = RotatableMeshSatellite(fullfile(script_dir, "geometries", mesh_name));
pipeline          = ShadingPipeline(satellite, 1, resolution);  % 1 = CoP shader
calculator_hybrid = HybridAeroLoadCalculator(satellite, pipeline, gsi_model_cpu);
calculator_gpu    = GPUAeroLoadCalculator(satellite,gsi_model_gpu,resolution);

% Warm-up: the first call fills the geometry cache and the GPU state.
[F_hyb, T_hyb] = calculator_hybrid.calc_aero_load(v_rel__m_per_s, surface_temp__K, aero_conditions);
[F_gpu, T_gpu] = calculator_gpu.calc_aero_torque_force(v_rel__m_per_s,surface_temp__K,aero_conditions);

fprintf('Hybrid Force [%d,%d,%d], GPU Force [%d,%d,%d]\n', F_hyb, F_gpu);
fprintf('Hybrid Torque [%d,%d,%d], GPU Torque [%d,%d,%d]\n', F_hyb, F_gpu);
