%% Convergence of the vat pixel calculator with the raster resolution
% Evaluates the base shuttlecock with vat.loads.PixelForceTorqueCalculator
% (MSAA 8) at several num_pixel over a pitch sweep and measures convergence by
% the successive differences between neighbouring resolutions. Conditions,
% GSI model and flow frames are the same as in verify_shuttlecock_pitch_sweep.m,
% whose vat reference this study quantifies.
%
% Differences:
%   d_F = |F_k - F_(k-1)| / |F_k|            per angle (the force never vanishes)
%   d_M = |M_k - M_(k-1)| / max_theta |M_k|  (the torque is ~0 at 0 deg pitch)
% Observed order p = log2(d_(k-1) / d_k); a pixel-edge error gives p ~ 1.
%
% Requires ADBSat on the MATLAB path (for environment()) and the Aerospace
% Toolbox.

clear; close all;
vat.setLogLevel("error");

assert(exist('environment', 'file') == 2, 'ADBSat is not on the MATLAB path; run ADBSat_dynpath first.');
assert(exist('atmosnrlmsise00', 'file') == 2, 'atmosnrlmsise00 (Aerospace Toolbox) is required.');

%% Inputs
num_pixels = [100,200,400,800,1600,3200]; % raster resolutions, each 2x the previous
num_msaa_samples = 8;
pitch__deg = -30:1:30;               % ADBSat angles of attack [deg]
sideslip__deg = 0;

% Environment and GSI parameters, as in ADBSat's example_GSI.m
param_eq = environment(struct(), 300e3, 0, 0, 1, 0, 140, 140, ones([1, 7]) * 15, 1);
param_eq.alpha = 1;
param_eq.Tw = 300;
vat_setup = adbsat_to_vat(param_eq, 'coeff_sentman');

folder = fileparts(mfilename('fullpath'));
mesh_file = fullfile(folder, 'remeshed', 'shuttlecock_base_rotated_panels.obj');
geometry = vat.geometry.RotatableMeshGeometry(mesh_file);

%% Flow per angle, identical to ADBSat's calc_coeff
num_angles = numel(pitch__deg);
L_gb = [1 0 0; 0 -1 0; 0 0 -1]; % body to geometric (mesh)
v_rel__m_per_s = zeros(num_angles, 3);
for a = 1:num_angles
    aoa = deg2rad(pitch__deg(a));
    aos = deg2rad(sideslip__deg);
    L_wb = [cos(aos) * cos(aoa), sin(aos), sin(aoa) * cos(aos); ...
            -sin(aos) * cos(aoa), cos(aos), -sin(aoa) * sin(aos); ...
            -sin(aoa), 0, cos(aoa)];                 % body to wind
    flow_dir = L_gb * L_wb' * [-1; 0; 0];            % gas velocity direction in the mesh frame
    v_rel__m_per_s(a, :) = -vat_setup.speed__m_per_s * flow_dir';
end

%% Evaluate every resolution
num_res = numel(num_pixels);
force__N = zeros(num_angles, 3, num_res);
torque__Nm = zeros(num_angles, 3, num_res);
runtime__s = zeros(num_res, 1);
for k = 1:num_res
    timer = tic;
    calculator = vat.loads.PixelForceTorqueCalculator(geometry, vat_setup.gsi_model, num_pixels(k), ...
                                                      NumMsaaSamples = num_msaa_samples);
    for a = 1:num_angles
        [force__N(a, :, k), torque__Nm(a, :, k)] = calculator.calc_aero_load(v_rel__m_per_s(a, :), ...
                                                                             vat_setup.T_wall__K, ...
                                                                             vat_setup.aero_cond);
    end
    runtime__s(k) = toc(timer);
end

%% Successive differences
num_pairs = num_res - 1;
diff_force = zeros(num_angles, num_pairs);
diff_torque = zeros(num_angles, num_pairs);
for k = 2:num_res
    diff_force(:, k - 1) = vecnorm(force__N(:, :, k) - force__N(:, :, k - 1), 2, 2) ./ ...
        vecnorm(force__N(:, :, k), 2, 2);
    diff_torque(:, k - 1) = vecnorm(torque__Nm(:, :, k) - torque__Nm(:, :, k - 1), 2, 2) ./ ...
        max(vecnorm(torque__Nm(:, :, k), 2, 2));
end
rms_force = sqrt(mean(diff_force.^2, 1))';
max_force = max(diff_force, [], 1)';
rms_torque = sqrt(mean(diff_torque.^2, 1))';
max_torque = max(diff_torque, [], 1)';
order_force = [NaN; log2(rms_force(1:end - 1) ./ rms_force(2:end))];
order_torque = [NaN; log2(rms_torque(1:end - 1) ./ rms_torque(2:end))];

fprintf('Pixel convergence, Sentman, pitch %g..%g deg, MSAA %d, %s\n', pitch__deg(1), pitch__deg(end), ...
        num_msaa_samples, 'shuttlecock_base_rotated_panels.obj');
fprintf('%11s | %10s %10s %7s | %10s %10s %7s\n', 'pair', 'RMS d_F %', 'max d_F %', 'order', ...
        'RMS d_M %', 'max d_M %', 'order');
for p = 1:num_pairs
    fprintf('%5d-%5d | %10.4f %10.4f %7.2f | %10.4f %10.4f %7.2f\n', num_pixels(p), num_pixels(p + 1), ...
            100 * rms_force(p), 100 * max_force(p), order_force(p), ...
            100 * rms_torque(p), 100 * max_torque(p), order_torque(p));
end
fprintf('\nRuntime for %d angles:\n', num_angles);
for k = 1:num_res
    fprintf('  %5d px: %6.2f s\n', num_pixels(k), runtime__s(k));
end

%% Figure 1: convergence
finer_pixels = num_pixels(2:end);
colors = lines(2);
figure('Name', 'Pixel convergence', 'Position', [100 100 800 550]);
loglog(finer_pixels, rms_force, 'o-', 'Color', colors(1, :), 'LineWidth', 1.5, 'DisplayName', 'force, RMS');
hold on;
loglog(finer_pixels, max_force, 'o--', 'Color', colors(1, :), 'DisplayName', 'force, max');
loglog(finer_pixels, rms_torque, 's-', 'Color', colors(2, :), 'LineWidth', 1.5, 'DisplayName', 'torque, RMS');
loglog(finer_pixels, max_torque, 's--', 'Color', colors(2, :), 'DisplayName', 'torque, max');
loglog(finer_pixels, rms_force(1) * finer_pixels(1) ./ finer_pixels, 'k:', 'LineWidth', 1, ...
       'DisplayName', 'slope N^{-1}');
grid on;
xticks(finer_pixels);
xlabel('num\_pixel (finer of each pair)');
ylabel('successive difference');
title(sprintf('Pixel calculator, MSAA %d, pitch %g..%g deg', num_msaa_samples, pitch__deg(1), pitch__deg(end)));
legend('Location', 'southwest');

%% Figure 2: sweep curves
figure('Name', 'Pitch sweep per resolution', 'Position', [120 120 1300 420]);
layout = tiledlayout(1, 3, 'TileSpacing', 'compact');
title(layout, sprintf('Pixel calculator, MSAA %d (light = 250 px, dark = 2000 px)', num_msaa_samples));
res_colors = flipud(parula(num_res + 2));
res_colors = res_colors(2:end - 1, :);
quantities = {force__N, 1, 'F_x [N]'; force__N, 3, 'F_z [N]'; torque__Nm, 2, 'M_y [Nm]'};
for q = 1:size(quantities, 1)
    nexttile;
    hold on;
    for k = 1:num_res
        plot(pitch__deg, squeeze(quantities{q, 1}(:, quantities{q, 2}, k)), '-', 'Color', res_colors(k, :), ...
             'LineWidth', 1.25, 'DisplayName', sprintf('%d px', num_pixels(k)));
    end
    grid on;
    xlabel('Pitch [deg]');
    ylabel(quantities{q, 3});
    if q == 1
        legend('Location', 'best');
    end
end
