clearvars;

% compare shading times for ISS model
fp.current_folder = fileparts(mfilename('fullpath'));
fp.geometry_dir   = fullfile(fp.current_folder, "geometries");
% fp.names          = ["iss.obj"];
fp.names          = ["iss_26.obj"];
% fp.names          = ["iss_106.obj"];
% fp.names          = ["iss.obj", "iss_26.obj", "iss_106.obj"];

setLogLevel("warn");

for i=1:numel(fp.names)
    filepath = fullfile(fp.geometry_dir, fp.names(i));

    geometry = RotatableMeshSatellite(filepath);
    velocity = [cos(0.3)*7800,cos(0.01),sin(0.3)*7800];
    shader = ShadingPipeline(geometry, 0, 4000);
    tris = shader.shade(velocity);
    gsi = Sentman(1);
    aero_conditions = AeroConditions(1e-9, 934, ...
        16 * 1.6605390689252e-27, 0.95);

    show_mesh(geometry, tris, velocity);

    satellite = HybridAeroLoadCalculator(geometry, shader, gsi);
    num_calc = 1000;
    tic;
    for k=1:num_calc
        satellite.calc_aero_load(velocity, 300, aero_conditions);
    end
    time = toc;
    time_per_calc = time/num_calc
end