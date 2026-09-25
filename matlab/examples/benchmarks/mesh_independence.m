%% Benchmark: does the result depend on the mesh?
% The same shuttlecock, meshed with 96 up to 61440 triangles, evaluated by
% all three methods at the same resolution. The hybrid calculator decides
% visibility per triangle, so its result depends on how the surface is
% meshed: where a shadow edge cuts through a triangle, the triangle counts
% as fully visible, and a triangle too thin to cover a pixel centre counts
% as hidden. The finest meshes have such slivers, so there the hybrid drag
% drops until num_pixel is large enough to resolve them. The per-pixel
% calculator cuts the shadow edge through the triangles and does not
% depend on the mesh.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

aero_cond = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Sentman(1, 0.95);
T_wall    = 300;
num_pixel = 2000;

% Flow from the side, so the body and the wings shade each other.
v_rel = 7800 * [cosd(30), sind(30), 0];

folder  = fullfile(fileparts(mfilename('fullpath')), '..', 'geometries');
meshes  = [96 240 960 3840 15360 61440];
methods = {'hybrid, Binary', 'hybrid, CoP', 'per pixel'};
drag    = zeros(numel(meshes), numel(methods));

for i = 1:numel(meshes)
    geometry = vat.geometry.RotatableMeshGeometry(fullfile(folder, sprintf('shuttlecock_%d.obj', meshes(i))));
    calculators = {
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 0, num_pixel), gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 1, num_pixel), gsi_model)
        vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, num_pixel)
    };
    for m = 1:numel(calculators)
        F = calculators{m}.calc_aero_load(v_rel, T_wall, aero_cond);
        drag(i, m) = -dot(F, v_rel) / norm(v_rel);
    end
end

%% Results
% Deviation from the per-pixel result on the finest mesh.
reference = drag(end, 3);
deviation = 100 * (drag - reference) / reference;

fprintf('\n triangles');
fprintf('  %16s', methods{:});
fprintf('   (drag deviation from per pixel on the finest mesh, %%)\n');
for i = 1:numel(meshes)
    fprintf(' %9d', meshes(i));
    fprintf('  %16.3f', deviation(i, :));
    fprintf('\n');
end

figure('Name', 'Mesh independence', 'Position', [100 100 700 600]);
tiledlayout(2, 1, 'TileSpacing', 'compact');

nexttile;
semilogx(meshes, 1e6 * drag, '-o', 'LineWidth', 1.5);
grid on;
ylabel('drag [\muN]');
legend(methods, 'Location', 'best');
title(sprintf('Same shuttlecock, different meshes, %d x %d pixels', num_pixel, num_pixel));

nexttile;
semilogx(meshes, deviation, '-o', 'LineWidth', 1.5);
grid on;
xlabel('triangles in the mesh');
ylabel('deviation [%]');
