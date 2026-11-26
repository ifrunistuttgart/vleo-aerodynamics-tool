%% Verification against the analytic solution: rectangular shuttlecock, pitch sweep
% Compares the vat pixel calculator and ADBSat with the exact Sentman load of
% rect_shuttlecock_analytical.m (derivation in rect_shuttlecock_analytical.md)
% on every mesh level written by make_rect_shuttlecock.m.
%
%   1. Sanity check, shading off: analytic vs vat (per-triangle sum of its
%      Sentman model) and ADBSat (shadow flag 0) on the coarsest mesh. Both
%      must agree to ~1e-6, which confirms geometry, frames and GSI mapping.
%   2. Pitch sweep with shading: errors of vat and ADBSat against the exact
%      solution per mesh level (force normalised per angle, torque by the
%      largest exact torque over the sweep), a convergence plot and the
%      sweep curves.
%
% Requires ADBSat on the MATLAB path (run ADBSat_dynpath) and the Aerospace
% Toolbox. ADBSat results are cached per mesh in tempdir.

clear; close all;
vat.setLogLevel("warn");

assert(exist('ADBSatFcn', 'file') == 2 && exist('environment', 'file') == 2, ...
       'ADBSat is not on the MATLAB path; run ADBSat_dynpath first.');
assert(exist('atmosnrlmsise00', 'file') == 2, 'atmosnrlmsise00 (Aerospace Toolbox) is required.');

%% Inputs
pitch__deg = -30:1:30;  % ADBSat angles of attack [deg], |pitch| < 45
num_pixel = 2000;       % vat raster resolution
num_msaa_samples = 8;   % vat MSAA samples

param_eq = environment(struct(), 300e3, 0, 0, 1, 0, 140, 140, ones([1, 7]) * 15, 1);
param_eq.gsi_model = 'sentman';
param_eq.alpha = 1;
param_eq.Tw = 300;
vat_setup = adbsat_to_vat(param_eq, 'coeff_sentman');
q__Pa = vat_setup.dynamic_pressure__Pa;

folder = fileparts(mfilename('fullpath'));
files = dir(fullfile(folder, 'rect_shuttlecock', 'rect_shuttlecock_L*.obj'));
assert(~isempty(files), 'No meshes in rect_shuttlecock/; run make_rect_shuttlecock.m first.');
levels = cellfun(@(name) str2double(regexp(name, '_L(\d+)_', 'tokens', 'once')), {files.name});
[levels, order] = sort(levels);
files = files(order);
num_meshes = numel(files);

cache_folder = fullfile(tempdir, 'vat_adbsat_verification', 'rect_shuttlecock');
if ~isfolder(cache_folder)
    mkdir(cache_folder);
end

%% Flow per angle, identical to ADBSat's calc_coeff
num_angles = numel(pitch__deg);
L_gb = [1 0 0; 0 -1 0; 0 0 -1];
L_gw = zeros(3, 3, num_angles);
v_rel__m_per_s = zeros(num_angles, 3);
for a = 1:num_angles
    aoa = deg2rad(pitch__deg(a));
    L_wb = [cos(aoa), 0, sin(aoa); 0, 1, 0; -sin(aoa), 0, cos(aoa)]; % body to wind, sideslip 0
    L_gw(:, :, a) = L_gb * L_wb';
    v_rel__m_per_s(a, :) = -vat_setup.speed__m_per_s * (L_gw(:, :, a) * [-1; 0; 0])';
end

%% Exact solution
force_exact__N = zeros(num_angles, 3);
torque_exact__Nm = zeros(num_angles, 3);
force_exact_unshaded__N = zeros(num_angles, 3);
for a = 1:num_angles
    [force_exact__N(a, :), torque_exact__Nm(a, :)] = rect_shuttlecock_analytical(pitch__deg(a), param_eq);
    force_exact_unshaded__N(a, :) = rect_shuttlecock_analytical(pitch__deg(a), param_eq, Shading = false);
end

%% 1. Sanity check without shading on the coarsest mesh
coarse_file = fullfile(files(1).folder, files(1).name);
coarse_mat = ADBSatImport(coarse_file, cache_folder, 0);
meshdata = load(coarse_mat, 'meshdata').meshdata;
unshaded = load(ADBSatFcn(coarse_mat, fullfile(cache_folder, 'unshaded'), param_eq, pitch__deg, 0, ...
                          0, 0, 0, 1, 0), 'aedb').aedb;
error_unshaded_vat = 0;
error_unshaded_adb = 0;
for a = 1:num_angles
    force_vat_sum__N = zeros(1, 3);
    for i = 1:numel(meshdata.Areas)
        force_vat_sum__N = force_vat_sum__N + reshape(vat_setup.gsi_model.calc_aero_force_torque( ...
            meshdata.Areas(i), meshdata.SurfN(:, i)', meshdata.BariC(:, i)', v_rel__m_per_s(a, :), ...
            vat_setup.T_wall__K, vat_setup.aero_cond), 1, 3);
    end
    force_adb__N = q__Pa * unshaded.AreaRef * (L_gw(:, :, a) * ...
        [unshaded.aero.Cf_wX(a, 1); unshaded.aero.Cf_wY(a, 1); unshaded.aero.Cf_wZ(a, 1)])';
    reference = norm(force_exact_unshaded__N(a, :));
    error_unshaded_vat = max(error_unshaded_vat, norm(force_vat_sum__N - force_exact_unshaded__N(a, :)) / reference);
    error_unshaded_adb = max(error_unshaded_adb, norm(force_adb__N - force_exact_unshaded__N(a, :)) / reference);
end
fprintf('Sanity check without shading (%s): max |dF|/|F| vat %.2e, ADBSat %.2e\n\n', ...
        files(1).name, error_unshaded_vat, error_unshaded_adb);

%% 2. vat and ADBSat on every mesh, with shading
num_triangles = zeros(num_meshes, 1);
force_vat__N = zeros(num_angles, 3, num_meshes);
torque_vat__Nm = zeros(num_angles, 3, num_meshes);
force_adb__N = zeros(num_angles, 3, num_meshes);
torque_adb__Nm = zeros(num_angles, 3, num_meshes);
for k = 1:num_meshes
    mesh_file = fullfile(files(k).folder, files(k).name);
    [~, mesh_name] = fileparts(mesh_file);

    geometry = vat.geometry.RotatableMeshGeometry(mesh_file);
    quality = geometry.get_mesh_quality();
    num_triangles(k) = quality.num_triangles;
    calculator = vat.loads.PixelForceTorqueCalculator(geometry, vat_setup.gsi_model, num_pixel, ...
                                                      NumMsaaSamples = num_msaa_samples);
    for a = 1:num_angles
        [force_vat__N(a, :, k), torque_vat__Nm(a, :, k)] = calculator.calc_aero_load( ...
            v_rel__m_per_s(a, :), vat_setup.T_wall__K, vat_setup.aero_cond);
    end

    cache_file = fullfile(cache_folder, ['sweep_' mesh_name '.mat']);
    cached = false;
    if isfile(cache_file)
        cache = load(cache_file);
        cached = isequal(cache.pitch__deg, pitch__deg);
    end
    if ~cached
        fprintf('ADBSat: %s ...\n', mesh_name);
        mat_file = ADBSatImport(mesh_file, cache_folder, 0);
        result_file = ADBSatFcn(mat_file, fullfile(cache_folder, mesh_name), param_eq, pitch__deg, 0, ...
                                1, 0, 0, 1, 0);
        cache = struct('aedb', load(result_file, 'aedb').aedb, 'pitch__deg', pitch__deg);
        save(cache_file, '-struct', 'cache');
    end
    aedb = cache.aedb;
    for a = 1:num_angles
        Cf_w = [aedb.aero.Cf_wX(a, 1); aedb.aero.Cf_wY(a, 1); aedb.aero.Cf_wZ(a, 1)];
        Cm_B = [aedb.aero.Cm_BX(a, 1); aedb.aero.Cm_BY(a, 1); aedb.aero.Cm_BZ(a, 1)];
        force_adb__N(a, :, k) = q__Pa * aedb.AreaRef * (L_gw(:, :, a) * Cf_w);
        torque_adb__Nm(a, :, k) = q__Pa * aedb.AreaRef * aedb.LenRef * (L_gb * Cm_B);
    end
end

%% Errors against the exact solution
[error_force_vat, error_torque_vat] = relative_errors(force_vat__N, torque_vat__Nm, force_exact__N, torque_exact__Nm);
[error_force_adb, error_torque_adb] = relative_errors(force_adb__N, torque_adb__Nm, force_exact__N, torque_exact__Nm);
rms = @(x) sqrt(mean(x.^2, 1))';
peak = @(x) max(x, [], 1)';

fprintf('Sentman, pitch %g..%g deg (%d angles); vat pixel %d px, MSAA %d\n', pitch__deg(1), pitch__deg(end), ...
        num_angles, num_pixel, num_msaa_samples);
fprintf('Errors against the analytic solution [%%]\n');
fprintf('%5s %9s | %9s %9s %9s %9s | %9s %9s %9s %9s\n', 'level', 'triangles', 'ADB rmsF', 'ADB maxF', ...
        'ADB rmsM', 'ADB maxM', 'vat rmsF', 'vat maxF', 'vat rmsM', 'vat maxM');
table_rows = 100 * [rms(error_force_adb), peak(error_force_adb), rms(error_torque_adb), peak(error_torque_adb), ...
                    rms(error_force_vat), peak(error_force_vat), rms(error_torque_vat), peak(error_torque_vat)];
for k = 1:num_meshes
    fprintf('%5s %9d | %9.4f %9.4f %9.4f %9.4f | %9.4f %9.4f %9.4f %9.4f\n', sprintf('L%d', levels(k)), ...
            num_triangles(k), table_rows(k, :));
end

%% Figure 1: convergence against the exact solution
colors = lines(4);
figure('Name', 'Convergence to the analytic solution', 'Position', [100 100 850 560]);
loglog(num_triangles, rms(error_force_adb), 'o-', 'Color', colors(1, :), 'LineWidth', 1.5, ...
       'DisplayName', 'ADBSat force, RMS');
hold on;
loglog(num_triangles, peak(error_force_adb), 'o--', 'Color', colors(1, :), 'DisplayName', 'ADBSat force, max');
loglog(num_triangles, rms(error_torque_adb), 's-', 'Color', colors(2, :), 'LineWidth', 1.5, ...
       'DisplayName', 'ADBSat torque, RMS');
loglog(num_triangles, peak(error_torque_adb), 's--', 'Color', colors(2, :), 'DisplayName', 'ADBSat torque, max');
loglog(num_triangles, rms(error_force_vat), 'd-', 'Color', colors(3, :), 'LineWidth', 1.5, ...
       'DisplayName', 'vat force, RMS');
loglog(num_triangles, peak(error_force_vat), 'd--', 'Color', colors(3, :), 'DisplayName', 'vat force, max');
loglog(num_triangles, rms(error_torque_vat), '^-', 'Color', colors(4, :), 'LineWidth', 1.5, ...
       'DisplayName', 'vat torque, RMS');
loglog(num_triangles, peak(error_torque_vat), '^--', 'Color', colors(4, :), 'DisplayName', 'vat torque, max');
guide = rms(error_force_adb);
loglog(num_triangles, guide(1) * sqrt(num_triangles(1) ./ num_triangles), 'k:', 'DisplayName', 'slope N^{-1/2}');
grid on;
xlabel('Triangle count');
ylabel('|F - F_{exact}| / |F_{exact}|,   |M - M_{exact}| / max |M_{exact}|');
title(sprintf('Rectangular shuttlecock, Sentman, pitch %g..%g deg', pitch__deg(1), pitch__deg(end)));
legend('Location', 'southwest');

%% Figure 2: sweep curves
figure('Name', 'Pitch sweep against the analytic solution', 'Position', [120 120 1300 420]);
layout = tiledlayout(1, 3, 'TileSpacing', 'compact');
title(layout, 'Analytic (black), ADBSat levels (light = coarse, dark = fine), vat on the finest mesh (dashed)');
mesh_colors = flipud(parula(num_meshes + 2));
mesh_colors = mesh_colors(2:end - 1, :);
quantities = {force_adb__N, force_vat__N, force_exact__N, 1, 'F_x [N]'; ...
              force_adb__N, force_vat__N, force_exact__N, 3, 'F_z [N]'; ...
              torque_adb__Nm, torque_vat__Nm, torque_exact__Nm, 2, 'M_y [Nm]'};
alpha_star__deg = atand(0.2 / (0.2 + 0.3 * sqrt(2)));
for p = 1:size(quantities, 1)
    [adb, vat_values, exact, component, label] = quantities{p, :};
    nexttile;
    hold on;
    for k = 1:num_meshes
        plot(pitch__deg, adb(:, component, k), '-', 'Color', mesh_colors(k, :), ...
             'DisplayName', sprintf('ADBSat L%d (%d tri)', levels(k), num_triangles(k)));
    end
    plot(pitch__deg, vat_values(:, component, end), '--', 'Color', colors(3, :), 'LineWidth', 1.5, ...
         'DisplayName', sprintf('vat L%d', levels(end)));
    plot(pitch__deg, exact(:, component), 'k-', 'LineWidth', 2, 'DisplayName', 'analytic');
    xline([-alpha_star__deg, alpha_star__deg], ':', 'HandleVisibility', 'off');
    grid on;
    xlabel('Pitch [deg]');
    ylabel(label);
    if p == 1
        legend('Location', 'best', 'FontSize', 7);
    end
end

%% Local functions
function [error_force, error_torque] = relative_errors(force__N, torque__Nm, force_ref__N, torque_ref__Nm)
    % Force normalised per angle, torque by its largest reference magnitude over the sweep.
    error_force = reshape(vecnorm(force__N - force_ref__N, 2, 2) ./ vecnorm(force_ref__N, 2, 2), ...
                          size(force__N, 1), []);
    error_torque = reshape(vecnorm(torque__Nm - torque_ref__Nm, 2, 2) ./ max(vecnorm(torque_ref__Nm, 2, 2)), ...
                           size(torque__Nm, 1), []);
end
