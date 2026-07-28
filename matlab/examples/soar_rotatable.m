% Some interesting use cases VLEO torque computation
div = "=========================================";
disp(div);
disp("Starting aerodynamic and shading tests...");
disp(div);


% Aerodynamic model
aero.model = Sentman(1);
disp(div);
disp("Created Sentman model.");
disp(div)

T_env   = 934;  % termperature
rho     = 1e-9; % density
ao_mass = 16 * 1.6605390689252e-27;
alpha_e = 0.95;

aero.conditions = AeroConditions(rho, T_env, ao_mass, alpha_e);

fp.current_folder  = fileparts(mfilename('fullpath'));
fp.obj_file        = fullfile(fp.current_folder, ...
    "geometries/soar_satellite.obj");

satellite.geometry = RotatableMeshSatellite(fp.obj_file);
satellite.verts    = satellite.geometry.get_vertices;

% rotate upper panel
p1.angle  = deg2rad(45);
p1.center = [-0.15; 0.00; 0.05];
p1.axis   = [0; 0; -1];
satellite.geometry.turn_surface_around_axis( ...
    0, deg2rad(45), p1.center, p1.axis)


show_mesh(satellite.geometry, ...
    zeros(satellite.geometry.get_num_triangles,1), [7800;0;0])