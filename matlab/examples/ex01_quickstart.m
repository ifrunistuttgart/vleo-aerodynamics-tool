%% Example 1: Quick start
% Computes the aerodynamic force and torque on a satellite in VLEO:
% atmosphere, gas-surface interaction model, geometry, shading pipeline,
% load calculator.
%
% Prerequisites: make sure you have build the matlab binaries with:
% pixi run build-matlab
% And that you have loaded the Matlab project from Vat.prj

clear; close all;
vat.setLogLevel("warn");   % "debug", "info", "warn", "error" or "off"

%% 1. Atmosphere
% Representative conditions for roughly 300 km altitude. Atomic oxygen is
% the dominant species in VLEO, hence the particle mass of 16 u.
rho__kg_per_m3    = 1.2482e-11;
T_atmospheric__K  = 934.0;
particle_mass__kg = 16 * 1.6605390689252e-27;
surface_temp__K   = 300.0;

conditions = vat.AeroConditions(rho__kg_per_m3, T_atmospheric__K, particle_mass__kg);

%% 2. Gas-surface interaction model
% Sentman(temperature_ratio_method, alpha_e), alpha_e being the energy
% accommodation coefficient. Example 4 compares all six models.
gsi_model = vat.gsi_models.Sentman(1, 0.9);

%% 3. Geometry
obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);
fprintf('Loaded %d triangles\n', geometry.get_num_triangles());

%% 4. Shading pipeline
% The shading pipeline renders the geometry along the flow and decides for
% every triangle whether the flow reaches it, so parts in the shadow of
% others carry no load. Two algorithms decide which triangles are visible:
%   algorithm: 0 = Binary, 1 = CoP (example 5 compares them)
%   num_pixel: the accuracy/runtime knob, the render is num_pixel x num_pixel
num_pixel  = 2000;
pipeline   = vat.shading.ShadingPipeline(geometry, 1, num_pixel);

%% 5. Load calculator
% Two calculators are available:
% 1. HybridForceTorqueCalculator (used here) takes the visibility from the
%    shading pipeline and evaluates the GSI model on every visible triangle.
% 2. PixelForceTorqueCalculator renders the geometry along the flow itself
%    and evaluates the GSI model in every pixel, so it needs no pipeline:
%        calculator = vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, num_pixel);
%    It is used in exactly the same way as the Hybrid calculator.

calculator = vat.loads.HybridForceTorqueCalculator(geometry, pipeline, gsi_model);

%% 6. Force and torque
% v_rel is the velocity of the satellite relative to the atmosphere, in the
% body frame, as a row vector [x y z]. A surface faces the flow when
% dot(normal, v_rel) > 0. The torque is about the body origin.
v_rel__m_per_s = [7800, 0, 0];

[force__N, torque__Nm] = calculator.calc_aero_load(v_rel__m_per_s, surface_temp__K, conditions);

fprintf('Force  [N]  : %+.4e %+.4e %+.4e\n', force__N);
fprintf('Torque [Nm] : %+.4e %+.4e %+.4e\n', torque__Nm);

% The calculator is set up once; every further call only renders and sums,
% so sweeping many attitudes is cheap (see example 6).
