%% Example 1: Quick start
% Computes the aerodynamic force and torque on a satellite in VLEO:
% atmosphere, gas-surface interaction model, geometry, load calculator.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

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
% accommodation coefficient. Example 3 compares all six models.
gsi_model = vat.gsi_models.Sentman(1, 0.9);

%% 3. Geometry
obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);
fprintf('Loaded %d triangles\n', geometry.get_num_triangles());

%% 4. Load calculator
% Renders the geometry along the flow and evaluates the GSI model in every
% pixel the flow reaches, so parts in the shadow of others carry no load.
% num_pixel is the accuracy/runtime knob: the render is num_pixel x num_pixel.
num_pixel  = 2000;
calculator = vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, num_pixel);

%% 5. Force and torque
% v_rel is the velocity of the satellite relative to the atmosphere, in the
% body frame, as a row vector [x y z]. A surface faces the flow when
% dot(normal, v_rel) > 0. The torque is about the body origin.
v_rel__m_per_s = [7800, 0, 0];

[force__N, torque__Nm] = calculator.calc_aero_load(v_rel__m_per_s, surface_temp__K, conditions);

fprintf('Force  [N]  : %+.4e %+.4e %+.4e\n', force__N);
fprintf('Torque [Nm] : %+.4e %+.4e %+.4e\n', torque__Nm);

% The calculator is set up once; every further call only renders and sums,
% so sweeping many attitudes is cheap (see example 6).
