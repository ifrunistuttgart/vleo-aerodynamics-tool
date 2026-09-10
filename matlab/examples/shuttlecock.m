% Some interesting use cases VLEO torque computation
div = "=========================================";
disp(div);
disp("Starting aerodynamic and shading tests...");
disp(div);


% Aerodynamic model
T_env   = 934;  % termperature
rho     = 1e-9; % density
ao_mass = 16 * 1.6605390689252e-27;
alpha_e = 0.95;

aero.model = vat.gsi_models.Sentman(1,alpha_e);
disp(div);
disp("Created Sentman model.");
disp(div)


aero.conditions = vat.AeroConditions(rho, T_env, ao_mass);

fp.current_folder  = fileparts(mfilename('fullpath'));
fp.obj_file        = fullfile(fp.current_folder, ...
    "geometries/shuttlecock_15k.obj");

satellite.geometry = vat.geometry.RotatableMeshGeometry(fp.obj_file);
satellite.verts    = satellite.geometry.get_vertices;

n_tri = satellite.geometry.get_num_triangles;

% Which mesh_id belongs to which part? The legend reads "[mesh_id] name".
vat.visualization.show_meshes(satellite.geometry)

% Check a hinge before using it: the fields are exactly the arguments
% turn_mesh_around_axis takes, so a hinge that looks right can be applied as is.
hinges(1).mesh_id = 4;
hinges(1).origin  = [-0.15; 0; -0.05];
hinges(1).axis    = [0; -1; 0];
vat.visualization.show_hinges(satellite.geometry, hinges)

satellite.geometry.turn_mesh_around_axis( ...
    hinges(1).mesh_id, deg2rad(20), hinges(1).origin, hinges(1).axis)

vat.visualization.show_shading(satellite.geometry, zeros(n_tri,1), [1;0;0])
