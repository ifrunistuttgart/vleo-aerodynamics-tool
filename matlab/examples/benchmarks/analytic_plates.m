%% Benchmark: accuracy against an analytic result
% Two two-sided flat plates face +x, with the upstream plate partly
% shadowing the +x face of the downstream plate. The leeward faces are
% never shadowed. The reference is assembled from direct GSI evaluations,
% using the visible area and centroid of every surface.
% Compares the hybrid calculator (Binary and CoP shading) and the
% per-pixel calculator
%   1. over num_pixel at a fixed flow angle, and
%   2. over the flow angle, which moves the shadow across the back plate,
% each with the second plate meshed coarsely and finely.

clear; close all;
vat.setLogLevel("warn");

rho       = 1.2482e-11;
aero_cond = vat.AeroConditions(rho, 934.0, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Newton();
speed     = 7800;
T_wall    = 300;

folder  = fullfile(fileparts(mfilename('fullpath')), '..', 'geometries');
meshes  = {'two_two_sided_plates.obj', '2 triangles per face'; ...
           'two_two_sided_plates_fine.obj', 'fine two-sided plates'};
methods = {'hybrid, Binary', 'hybrid, CoP'};

%% 1. Error over num_pixel, flow at 12 deg
% At 12 deg the shadow edge on the back plate cuts through the cells of the
% fine mesh; at angles where it falls exactly on a cell boundary, the
% hybrid calculator would be exact by coincidence.
alpha_fixed = 12;
num_pixels  = [250 500 1000 2000 4000];
[F_exact, T_exact] = exact_loads(alpha_fixed, gsi_model, speed, T_wall, aero_cond);

%% 2. Error over the flow angle, num_pixel = 2000
alphas = -89:1:89;   % [deg]; the shadow slides across the whole back plate
error_alpha = zeros(numel(num_pixels),numel(alphas), numel(methods), size(meshes, 1));
for g = 1:size(meshes, 1)
    geometry = vat.geometry.RotatableMeshGeometry(fullfile(folder, meshes{g, 1}));
    for p  = 1:numel(num_pixels)
        calculators = make_calculators(geometry, gsi_model, num_pixels(p));
        for i = 1:numel(alphas)
            F_ref = exact_loads(alphas(i), gsi_model, speed, T_wall, aero_cond);
            for m = 1:numel(calculators)
                F = calculators{m}.calc_aero_load(flow(alphas(i), speed), T_wall, aero_cond).';   % returned as a column
                error_alpha(p,i, m, g) = norm(F - F_ref) / norm(F_ref);
            end
        end
    end
end

avg_error = mean(error_alpha, 2);
std_error = std(error_alpha,1,2);

%% Results
colors = lines(numel(methods));
styles = {'-o', '--s'};
figure('Name', 'Error over num_pixel');

% Achsen explizit erstellen, um Log-Skalierung mit errorbar sauber zu steuern
ax = axes;
hold on;

for g = 1:size(meshes, 1)
    for m = 1:numel(methods)
        y_val = 100 * avg_error(:, :, m, g);
        y_err = 100 * std_error(:, :, m, g);
        
        offset_factor = (m - (numel(methods) + 1) / 2) * 15; 
        x_shifted = num_pixels + offset_factor;
        
        errorbar(x_shifted, y_val, y_err, styles{g}, ...
            'Color', colors(m, :), 'LineWidth', 1.5, ...
            'DisplayName', sprintf('%s, %s', methods{m}, meshes{g, 2}));
    end
end

grid on;
xticks(num_pixels);
xlabel('num\_pixel');
ylabel('force error [%]');
legend('Location', 'southwest');
title(sprintf('Two plates, average relative error'));
%%

figure('Name', 'Error over flow angle');
for g = 1:size(meshes, 1)
    for m = 1:numel(methods)
        plot(alphas, 100 * error_alpha(end,:, m, g), styles{g}, 'Color', colors(m, :), 'LineWidth', 1, ...
            'MarkerSize', 3, 'DisplayName', sprintf('%s, %s', methods{m}, meshes{g, 2}));
        hold on;
    end
end
grid on;
xlabel('flow angle in the x-y plane [deg]');
ylabel('force error [%]');
legend('Location', 'northwest');
title('Two plates, num\_pixel = 4000');

%% Local functions
function v = flow(alpha__deg, speed)
    v = speed * [cosd(alpha__deg), sind(alpha__deg), 0];
end

function calculators = make_calculators(geometry, gsi_model, num_pixel)
    % Hybrid with Binary shading, hybrid with CoP shading, per pixel.
    calculators = {
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 0, num_pixel), gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 1, num_pixel), gsi_model)
    };
end

function [F, T] = exact_loads(alpha__deg, gsi_model, speed, T_wall, aero_cond)
    v_rel = flow(alpha__deg, speed);
    [visible_area, visible_centroid] = downstream_visible_surface(alpha__deg);

    % Plate 1 is at x = 0 and plate 2 at x = 0.5. The +x face of plate 1
    % shadows plate 2; both -x faces are leeward and fully exposed.
    surfaces = {
        1.0, [1, 0, 0], [0, 0, 0]; ...
        1.0, [-1, 0, 0], [0, 0, 0]; ...
        visible_area, [1, 0, 0], visible_centroid; ...
        1.0, [-1, 0, 0], [0.5, 0, 0.5]
    };
    F = zeros(1, 3);
    T = zeros(1, 3);
    for i = 1:size(surfaces, 1)
        [force, torque] = gsi_model.calc_aero_force_torque( ...
            surfaces{i, 1}, surfaces{i, 2}, surfaces{i, 3}, ...
            v_rel, T_wall, aero_cond);
        F = F + force(:).';
        T = T + torque(:).';
    end
end

function [visible_area, visible_centroid] = downstream_visible_surface(alpha__deg)
    % The x spacing translates the upstream plate shadow by this amount in y.
    shift = 0.5 * tand(alpha__deg);
    overlap_lo = max(-0.5, -0.5 + shift);
    overlap_hi = min(0.5, 0.5 + shift);
    overlap_width = max(0, overlap_hi - overlap_lo);
    overlap_area = 0.5 * overlap_width;  % overlap in z is [0, 0.5]
    overlap_centroid = [0.5, 0.5 * (overlap_lo + overlap_hi), 0.25];

    visible_area = 1.0 - overlap_area;
    visible_centroid = ([0.5, 0, 0.5] - overlap_area * overlap_centroid) / visible_area;
end
