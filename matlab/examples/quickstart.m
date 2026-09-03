%% quick start
% Computes the aerodynamic force and torque acting on a satellite in VLEO,
% then visualizes which surfaces the oncoming flow actually reaches.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;

%% 1. Atmosphere
% Representative conditions for roughly 300 km altitude. Atomic oxygen is
% the dominant species in VLEO, hence the particle mass of 16 u.
rho__kg_per_m3    = 1.2482e-11;
T_atmospheric__K  = 934.0;
particle_mass__kg = 16 * 1.6605390689252e-27;
surface_temp__K   = 300.0;

conditions = AeroConditions(rho__kg_per_m3, T_atmospheric__K, particle_mass__kg);

%% 2. Gas-surface interaction model
% Sentman(temperature_ratio_method, alpha_e), where alpha_e is the energy
% accommodation coefficient. Any other GSI model (Maxwell, Cook,
% SchaafChambre, Storch, Newton) can be dropped in here unchanged.
gsi_model = Sentman(1, 0.9);

%% 3. Geometry
obj_file  = fullfile(fileparts(mfilename('fullpath')), ...
    'geometries', 'shuttlecock_15k.obj');
satellite = RotatableMeshSatellite(obj_file);
fprintf('Loaded %d triangles\n', satellite.get_num_triangles());

%% 4. Shading pipeline
% Determines which triangles are exposed to the flow and which are hidden
% behind other parts of the satellite.
%   algorithm: 0 = Binary, 1 = CoP
%   num_pixel: raster resolution, the accuracy/runtime knob
num_pixel = 1000;
pipeline  = ShadingPipeline(satellite, 1, num_pixel);

%% 5. Load calculator
% Combines geometry, shading, and the GSI model.
calculator = HybridAeroLoadCalculator(satellite, pipeline, gsi_model);

%% 6. Flow direction
% v_rel is the velocity of the satellite relative to the atmosphere,
% expressed in the body frame. A surface is windward when
% dot(normal, v_rel) > 0. Vectors are row vectors, [x y z].
v_rel__m_per_s = [7800, 0, 0];

[force__N, torque__Nm] = calculator.calc_aero_load( ...
    v_rel__m_per_s, surface_temp__K, conditions);

fprintf('Force  [N]  : %+.4e %+.4e %+.4e\n', force__N);
fprintf('Torque [Nm] : %+.4e %+.4e %+.4e\n', torque__Nm);

%% 7. Visualize the shading result
% Exposed triangles are coloured differently from shadowed ones. The
% pipeline is initialized once and can be re-shaded cheaply for any number
% of further flow directions, which is what makes attitude sweeps practical.
visibility = pipeline.shade(v_rel__m_per_s);
show_mesh(satellite, visibility, v_rel__m_per_s);
