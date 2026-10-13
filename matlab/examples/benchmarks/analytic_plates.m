%% Benchmark: accuracy against an analytic result
% Two flat plates facing +x, the front one partly shading the back one.
% With the Newton model and flow in the x-y plane, force and torque follow
% from the visible area and its centroid, so the exact result is known.
% Compares the hybrid calculator (Binary and CoP shading) and the
% per-pixel calculator
%   1. over num_pixel at a fixed flow angle, and
%   2. over the flow angle, which moves the shadow across the back plate,
% each with the back plate meshed as 2 triangles and as 2048.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

rho       = 1.2482e-11;
aero_cond = vat.AeroConditions(rho, 934.0, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Newton();
speed     = 7800;
T_wall    = 300;

folder  = fullfile(fileparts(mfilename('fullpath')), '..', 'geometries');
meshes  = {'two_plates.obj', '2 triangles per plate'; ...
           'two_plates_fine.obj', 'back plate 2048 triangles'};
methods = {'hybrid, Binary', 'hybrid, CoP', 'per pixel'};

%% 1. Error over num_pixel, flow at 12 deg
% At 12 deg the shadow edge on the back plate cuts through the cells of the
% fine mesh; at angles where it falls exactly on a cell boundary, the
% hybrid calculator would be exact by coincidence.
alpha_fixed = 12;
num_pixels  = [250 500 1000 2000 4000];
[F_exact, T_exact] = exact_loads(alpha_fixed, rho, speed);

error_N = zeros(numel(num_pixels), numel(methods), size(meshes, 1));
for g = 1:size(meshes, 1)
    geometry = vat.geometry.RotatableMeshGeometry(fullfile(folder, meshes{g, 1}));
    for i = 1:numel(num_pixels)
        F = evaluate_all(geometry, gsi_model, num_pixels(i), flow(alpha_fixed, speed), T_wall, aero_cond);
        error_N(i, :, g) = vecnorm(F - F_exact, 2, 2) / norm(F_exact);
    end
end

%% 2. Error over the flow angle, num_pixel = 2000
alphas = 0:1:65;   % [deg]; the shadow slides across the whole back plate
error_alpha = zeros(numel(alphas), numel(methods), size(meshes, 1));
for g = 1:size(meshes, 1)
    geometry = vat.geometry.RotatableMeshGeometry(fullfile(folder, meshes{g, 1}));
    calculators = make_calculators(geometry, gsi_model, 2000);
    for i = 1:numel(alphas)
        F_ref = exact_loads(alphas(i), rho, speed);
        for m = 1:numel(calculators)
            F = calculators{m}.calc_aero_load(flow(alphas(i), speed), T_wall, aero_cond).';   % returned as a column
            error_alpha(i, m, g) = norm(F - F_ref) / norm(F_ref);
        end
    end
end

%% Results
fprintf('\nForce error at %.0f deg flow angle [%%]\n', alpha_fixed);
for g = 1:size(meshes, 1)
    fprintf('%s\n  num_pixel', meshes{g, 2});
    fprintf('  %14s', methods{:});
    fprintf('\n');
    for i = 1:numel(num_pixels)
        fprintf('  %9d', num_pixels(i));
        fprintf('  %14.3f', 100 * error_N(i, :, g));
        fprintf('\n');
    end
end
fprintf('(torque at %.0f deg, exact: %+.3e %+.3e %+.3e Nm)\n', alpha_fixed, T_exact);

colors = lines(numel(methods));
styles = {'-o', '--s'};

figure('Name', 'Error over num_pixel');
for g = 1:size(meshes, 1)
    for m = 1:numel(methods)
        loglog(num_pixels, 100 * error_N(:, m, g), styles{g}, 'Color', colors(m, :), 'LineWidth', 1.5, ...
            'DisplayName', sprintf('%s, %s', methods{m}, meshes{g, 2}));
        hold on;
    end
end
grid on;
xticks(num_pixels);
xlabel('num\_pixel');
ylabel('force error [%]');
legend('Location', 'southwest');
title(sprintf('Two plates, flow at %.0f deg', alpha_fixed));

figure('Name', 'Error over flow angle');
for g = 1:size(meshes, 1)
    for m = 1:numel(methods)
        plot(alphas, 100 * error_alpha(:, m, g), styles{g}, 'Color', colors(m, :), 'LineWidth', 1, ...
            'MarkerSize', 3, 'DisplayName', sprintf('%s, %s', methods{m}, meshes{g, 2}));
        hold on;
    end
end
grid on;
xlabel('flow angle in the x-y plane [deg]');
ylabel('force error [%]');
legend('Location', 'northwest');
title('Two plates, num\_pixel = 2000');

%% Local functions
function v = flow(alpha__deg, speed)
    v = speed * [cosd(alpha__deg), sind(alpha__deg), 0];
end

function calculators = make_calculators(geometry, gsi_model, num_pixel)
    % Hybrid with Binary shading, hybrid with CoP shading, per pixel.
    calculators = {
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 0, num_pixel), gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 1, num_pixel), gsi_model)
        vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, num_pixel)
    };
end

function F = evaluate_all(geometry, gsi_model, num_pixel, v_rel, T_wall, aero_cond)
    calculators = make_calculators(geometry, gsi_model, num_pixel);
    F = zeros(numel(calculators), 3);
    for m = 1:numel(calculators)
        F(m, :) = calculators{m}.calc_aero_load(v_rel, T_wall, aero_cond);
    end
end

function [F, T] = exact_loads(alpha__deg, rho, speed)
    % Newton: every visible point of the plates (normal +x) carries the
    % pressure p = rho v^2 cos^2(alpha) along -x, so F = -p A x and the torque
    % about the origin is p (0, -int z dA, int y dA) over the visible area.
    t = tand(alpha__deg);
    % A point (1, y, z) of the front plate shades (0, y - t, z) on the back one.
    overlap_lo = max(-1, 0.5 - t);
    overlap_hi = min(1, 1.5 - t);
    overlap    = max(0, overlap_hi - overlap_lo);   % times the plate height 1
    overlap_y  = 0.5 * (overlap_lo + overlap_hi);

    area = (4 - overlap) + 1;                       % back plate minus shadow, plus front
    int_y = -overlap * overlap_y + 1.0;             % back plate alone: 0; front: 1 * 1.0
    int_z = -overlap * 0.5 + 0.5;                   % back plate alone: 0; front: 1 * 0.5

    p = rho * speed^2 * cosd(alpha__deg)^2;
    F = [-p * area, 0, 0];
    T = [0, -p * int_z, p * int_y];
end
