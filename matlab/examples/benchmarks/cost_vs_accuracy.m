%% Benchmark: computation time and accuracy
% What does each method cost, and what do you get for it?
%   1. Time per evaluation over num_pixel.
%   2. Error over time per evaluation, to pick a method and a resolution.
%   3. Time per evaluation over the number of triangles.
% Setting up a calculator is done once and not timed; for sweeps over
% attitudes or wing angles only the cost of each further evaluation counts.
% The error is measured against the per-pixel result at 8000 pixels, which
% benchmarks/analytic_plates.m checks against an exact result.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

aero_cond = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Newton();
T_wall    = 300;
v_rel     = 7800 * [cosd(20), sind(10), 0];

folder  = fullfile(fileparts(mfilename('fullpath')), '..', 'geometries');
methods = {'hybrid, Binary', 'hybrid, CoP', 'per pixel'};

%% 1. and 2. Time and error over num_pixel
geometry   = vat.geometry.RotatableMeshGeometry(fullfile(folder, 'shuttlecock_15360.obj'));
num_pixels = [250 500 1000 2000 4000 8000];
time_ms    = zeros(numel(num_pixels), numel(methods));
force      = zeros(numel(num_pixels), numel(methods), 3);

for i = 1:numel(num_pixels)
    calculators = make_calculators(geometry, gsi_model, num_pixels(i));
    for m = 1:numel(calculators)
        force(i, m, :) = calculators{m}.calc_aero_load(v_rel, T_wall, aero_cond);
        % timeit calls the evaluation repeatedly and returns the median.
        time_ms(i, m) = 1e3 * timeit(@() calculators{m}.calc_aero_load(v_rel, T_wall, aero_cond));
    end
    fprintf('%5d pixels: %7.2f ms (Binary)  %7.2f ms (CoP)  %7.2f ms (pixel)\n', num_pixels(i), time_ms(i, :));
end

reference = squeeze(force(end, 3, :))';
force_error = zeros(numel(num_pixels) - 1, numel(methods));
for m = 1:numel(methods)
    F = squeeze(force(1:end-1, m, :));
    force_error(:, m) = 100 * vecnorm(F - reference, 2, 2) / norm(reference);
end

%% 3. Time over the number of triangles, num_pixel = 2000
meshes = [96 240 960 3840 15360 61440];
time_mesh_ms = zeros(numel(meshes), numel(methods));
for i = 1:numel(meshes)
    mesh_geometry = vat.geometry.RotatableMeshGeometry(fullfile(folder, sprintf('shuttlecock_%d.obj', meshes(i))));
    calculators = make_calculators(mesh_geometry, gsi_model, 2000);
    for m = 1:numel(calculators)
        time_mesh_ms(i, m) = 1e3 * timeit(@() calculators{m}.calc_aero_load(v_rel, T_wall, aero_cond));
    end
end

%% Plots
figure('Name', 'Time over num_pixel');
plot(num_pixels, time_ms, '-o', 'LineWidth', 1.5);
grid on;
xticks(num_pixels);
xlabel('num\_pixel (render is num\_pixel x num\_pixel)');
ylabel('time per evaluation [ms]');
legend(methods, 'Location', 'northwest');
title(sprintf('%d triangles, Newton', geometry.get_num_triangles()));

figure('Name', 'Error over time');
loglog(time_ms(1:end-1, :), force_error, '-o', 'LineWidth', 1.5);
hold on;
for i = 1:numel(num_pixels) - 1
    text(time_ms(i, 3), force_error(i, 3), sprintf('  %d px', num_pixels(i)), 'FontSize', 8);
end
grid on;
xlabel('time per evaluation [ms]');
ylabel('force error [%]');
legend(methods, 'Location', 'southwest');
title('Lower left is better');

figure('Name', 'Time over triangles');
loglog(meshes, time_mesh_ms, '-o', 'LineWidth', 1.5);
grid on;
xlabel('triangles in the mesh');
ylabel('time per evaluation [ms]');
legend(methods, 'Location', 'northwest');
title('Shuttlecock meshes, 2000 x 2000 pixels');

%% Local functions
function calculators = make_calculators(geometry, gsi_model, num_pixel)
    % Hybrid with Binary shading, hybrid with CoP shading, per pixel.
    calculators = {
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 0, num_pixel), gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 1, num_pixel), gsi_model)
        vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, num_pixel)
    };
end
