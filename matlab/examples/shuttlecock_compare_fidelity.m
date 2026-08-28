% Some interesting use cases VLEO torque computation
div = "=========================================";
disp(div);
disp("Starting aerodynamic and shading tests...");
disp(div);

setLogLevel("warn");

% Aerodynamic model
aero = vat.Sentman(1);
disp(div);
disp("Created Sentman model.");
disp(div)

% Atmospheric conditions
T_env   = 934;  % termperature
rho     = 1e-9; % density
ao_mass = 16 * 1.6605390689252e-27;
alpha_e = 0.95;
aero_conditions = vat.AeroConditions(rho, T_env, ao_mass, alpha_e);

% filepath
fp.current_folder  = fileparts(mfilename('fullpath'));
fp.obj_file        = fullfile(fp.current_folder, ...
    "geometries/shuttlecock_61440.obj");

% Define geometry
geometry = vat.RotatableMeshGeometry(fp.obj_file);

% % Visualize mesh id's
% vat.show_meshes(geometry)

% Rotate upper panel
h(1).mesh_id = 4;
h(1).origin = [-0.15; 0; -0.05];
h(1).axis = [0;-1;0];
geometry.turn_mesh_around_axis( ...
    h.mesh_id, deg2rad(45), h.origin, h.axis);
vat.show_hinges(geometry, h);

% Define shading pipeline
shader = vat.ShadingPipeline(geometry, 0, 2000);
calculator = vat.HybridAeroLoadCalculator(geometry, shader, aero);


% Wind Direction
v_orbital__m_per_s = 7800;
alpha = deg2rad(20);
beta  = deg2rad(40);
rot_mat = wind_relative_dcm(alpha, beta);
v_rel   = rot_mat * [v_orbital__m_per_s; 0; 0];

% Compute aerodynamics

[f, tau] = calculator.calc_aero_load(v_rel, 300, aero_conditions);
vat.show_shading(geometry, shader.shade(v_rel), v_rel)


%% Local Functions
function rot_mat = wind_relative_dcm(alpha, beta)
    % WIND_RELATIVE_DCM DCM mapping the nominal +x flow to a wind
    % direction given an angle of attack (rotation about the y-axis)
    % followed by a sideslip rotation (about the z-axis).
    dcm.pitch = [cos(alpha), 0, -sin(alpha);
                 0,          1,  0;
                 sin(alpha), 0,  cos(alpha)];

    dcm.yaw   = [cos(beta), -sin(beta), 0;
                 sin(beta),  cos(beta), 0;
                 0,          0,         1];

    rot_mat = dcm.yaw * dcm.pitch;
end
