%% Per-pixel loads do not depend on the mesh
% The same shuttlecock, meshed with 96 up to 61440 triangles. The hybrid
% calculator decides visibility per triangle, so a triangle that is only
% partly shaded counts fully or not at all, and its result changes with the
% mesh. The per-pixel calculator cuts shadow edges through the triangles,
% so it gives the same load for every mesh.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

aero_cond = vat.AeroConditions(1e-9, 934, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Sentman(1, 0.95);

% Flow from the side, so the wings shade each other and the body.
v_rel     = 7800 * [cosd(30), sind(30), 0.2];
T_wall    = 300;
num_pixel = 2000;

meshes = [96 240 960 3840 15360 61440];
drag   = zeros(numel(meshes), 3);   % columns: hybrid binary, hybrid CoP, pixel

for i = 1:numel(meshes)
    obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', ...
        sprintf('shuttlecock_%d.obj', meshes(i)));
    geometry = vat.geometry.RotatableMeshGeometry(obj_file);

    for algorithm = [0 1]   % 0 = Binary, 1 = CoP
        pipeline = vat.shading.ShadingPipeline(geometry, algorithm, num_pixel);
        hybrid   = vat.loads.HybridForceTorqueCalculator(geometry, pipeline, gsi_model);
        F = hybrid.calc_aero_load(v_rel, T_wall, aero_cond);
        drag(i, algorithm + 1) = -dot(F, v_rel) / norm(v_rel);
    end

    pixel = vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, num_pixel);
    F = pixel.calc_aero_load(v_rel, T_wall, aero_cond);
    drag(i, 3) = -dot(F, v_rel) / norm(v_rel);

    fprintf('%6d triangles: drag %.4e (binary)  %.4e (CoP)  %.4e (pixel) N\n', ...
        meshes(i), drag(i, :));
end

figure('Name', 'Mesh independence');
semilogx(meshes, drag * 1e6, '-o', 'LineWidth', 1.5);
grid on;
xlabel('triangles in the mesh');
ylabel('drag [\muN]');
legend('hybrid, Binary shading', 'hybrid, CoP shading', 'per pixel', 'Location', 'best');
title(sprintf('Same shuttlecock, different meshes (%d x %d pixels)', num_pixel, num_pixel));
