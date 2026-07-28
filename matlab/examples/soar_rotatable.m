%% Some interesting use cases VLEO torque computation
div = "=========================================";
disp(div);
disp("Starting aerodynamic and shading tests...");
disp(div);

%% Aerodynamic Model
aero.model = Sentman(1);
disp(div);
disp("Created Sentman model.");
disp(div)

T_env   = 934;  % temperature
rho     = 1e-9; % density
ao_mass = 16 * 1.6605390689252e-27;
alpha_e = 0.95;

aero.conditions = AeroConditions(rho, T_env, ao_mass, alpha_e);

%% Load Satellite Geometry
fp.current_folder = fileparts(mfilename('fullpath'));
fp.obj_file       = fullfile(fp.current_folder, ...
    "geometries/soar_satellite.obj");

satellite.geometry = RotatableMeshSatellite(fp.obj_file);
satellite.verts    = satellite.geometry.get_vertices;

% Rotate the upper panel
p1.angle  = deg2rad(45);
p1.center = [-0.15; 0.00; 0.05];
p1.axis   = [0; 0; -1];
satellite.geometry.turn_surface_around_axis( ...
    0, p1.angle, p1.center, p1.axis);

%% Setup Shading Pipeline
shader = ShadingPipeline(satellite.geometry, 1, 1000);


%% Wind Direction
% Angle of attack (rotation in the body x-z plane) and sideslip angle
% (rotation in the body x-y plane), applied to the nominal +x flow.
v_orbital__m_per_s = 7800;
alpha = deg2rad(20);
beta  = deg2rad(40);

% DCM built from an angle-of-attack rotation about the y-axis followed
% by a sideslip rotation about the z-axis.
dcm.pitch = [cos(alpha), 0, -sin(alpha);
             0,          1,  0;
             sin(alpha), 0,  cos(alpha)];

dcm.yaw   = [cos(beta), -sin(beta), 0;
             sin(beta),  cos(beta), 0;
             0,          0,         1];

rot_mat = dcm.yaw * dcm.pitch;
v_rel   = rot_mat * [v_orbital__m_per_s; 0; 0];

%% Compute Panel Visibility
panel_visibility = shader.shade(v_rel);

%% Visualize Panel Visibility Result
show_mesh(satellite.geometry, panel_visibility, v_rel);

