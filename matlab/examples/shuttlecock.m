% Some interesting use cases VLEO torque computation
div = "=========================================";
disp(div);
disp("Starting aerodynamic and shading tests...");
disp(div);


% Aerodynamic model
aero.model = vat.Sentman(1);
disp(div);
disp("Created Sentman model.");
disp(div)

T_env   = 934;  % termperature
rho     = 1e-9; % density
ao_mass = 16 * 1.6605390689252e-27;
alpha_e = 0.95;

aero.conditions = vat.AeroConditions(rho, T_env, ao_mass, alpha_e);

fp.current_folder  = fileparts(mfilename('fullpath'));
fp.obj_file        = fullfile(fp.current_folder, ...
    "geometries/shuttlecock_15k.obj");

satellite.geometry = vat.RotatableMeshGeometry(fp.obj_file);
% satellite.geometry = vat.StaticMeshGeometry(fp.obj_file);
satellite.verts    = satellite.geometry.get_vertices;

n_tri = satellite.geometry.get_num_triangles;

satellite.geometry.turn_mesh_around_axis(2, deg2rad(20), [0;0;0], [1;0;0])


vat.show_meshes(satellite.geometry)

hinges(1).mesh_id = 4;
hinges(1).origin = [-0.15; 0; -0.05]
hinges(1).axis = [0;-1;0]
vat.show_hinges(satellite.geometry, hinges)
