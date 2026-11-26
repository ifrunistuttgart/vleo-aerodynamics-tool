%% Convergence study: ADBSat vs vat pixel over a pitch sweep
% ADBSat shades a triangle as a whole by testing its centroid; the vat pixel
% calculator shades per pixel and counts partly shaded triangles with their
% exposed part only. With shading off both codes agree to ~1e-6, so the
% difference between them is the shading discretisation of ADBSat, which
% should shrink as the triangles get smaller (error ~ edge length ~ N^-1/2).
%
% This script sweeps the pitch for every mesh in remeshed/ (see
% remesh_levels.m), compares ADBSat on each mesh against one vat pixel
% reference, and plots
%   1. the RMS and max relative force/torque error over the sweep against the
%      triangle count (convergence),
%   2. Fx, Fz and My against pitch for every ADBSat mesh and the reference,
%   3. the force error over pitch and mesh as a heatmap.
%
% The vat reference runs once, on the coarsest mesh at num_pixel = 2000 with
% MSAA 8; its own pixel error (a few 1e-4, see verify_pixel_convergence.m) is
% the floor ADBSat's error converges to.
% ADBSat results are cached per mesh in tempdir, so rerunning with a larger
% max_level only computes the new meshes.
%
% Requires ADBSat on the MATLAB path (run ADBSat_dynpath) and the Aerospace
% Toolbox. Frames, normalisation and torque reference as in
% verify_shuttlecock_pitch.m: everything in the mesh frame, torques about the
% mesh origin.

clear; close all;
vat.setLogLevel("warn");

assert(exist('ADBSatFcn', 'file') == 2 && exist('environment', 'file') == 2, ...
       'ADBSat is not on the MATLAB path; run ADBSat_dynpath first.');
assert(exist('atmosnrlmsise00', 'file') == 2, 'atmosnrlmsise00 (Aerospace Toolbox) is required.');

%% Inputs
pitch__deg = -30:1:30;      % ADBSat angles of attack [deg]
sideslip__deg = 0;          % ADBSat angle of sideslip [deg]
num_pixel = 2000;           % raster resolution of the vat reference
num_msaa_samples = 8;       % MSAA samples of the vat reference
max_level = 5;              % highest remesh level L<n> to include (the source is always included)
vat_pixel_floor = 4e-4;     % max force change 1000 -> 2000 px at MSAA 8 (verify_pixel_convergence.m)

% Environment and GSI parameters, as in ADBSat's example_GSI.m
param_eq = environment(struct(), 300e3, 0, 0, 1, 0, 140, 140, ones([1, 7]) * 15, 1);
param_eq.gsi_model = 'sentman';
param_eq.alpha = 1;
param_eq.Tw = 300;
vat_setup = adbsat_to_vat(param_eq, 'coeff_sentman');
q__Pa = vat_setup.dynamic_pressure__Pa;

%% Meshes: the unremeshed source plus L1..L<max_level>, sorted by triangle count
folder = fileparts(mfilename('fullpath'));
files = dir(fullfile(folder, 'remeshed', '*.obj'));
levels = zeros(numel(files), 1); % 0 = source
for k = 1:numel(files)
    token = regexp(files(k).name, '_L(\d+)_', 'tokens', 'once');
    if ~isempty(token)
        levels(k) = str2double(token{1});
    end
end
keep = levels <= max_level;
files = files(keep);
levels = levels(keep);
assert(any(levels == 0), 'Unremeshed source mesh not found in remeshed/.');

cache_folder = fullfile(tempdir, 'vat_adbsat_verification');
if ~isfolder(cache_folder)
    mkdir(cache_folder);
end

%% Frame matrices per angle, identical to ADBSat's calc_coeff
num_angles = numel(pitch__deg);
L_gb = [1 0 0; 0 -1 0; 0 0 -1];                 % body to geometric (mesh)
L_gw = zeros(3, 3, num_angles);                 % wind to geometric
for a = 1:num_angles
    aoa = deg2rad(pitch__deg(a));
    aos = deg2rad(sideslip__deg);
    L_wb = [cos(aos) * cos(aoa), sin(aos), sin(aoa) * cos(aos); ...
            -sin(aos) * cos(aoa), cos(aos), -sin(aoa) * sin(aos); ...
            -sin(aoa), 0, cos(aoa)];            % body to wind
    L_gw(:, :, a) = L_gb * L_wb';
end
% vat's v_rel is the geometry velocity relative to the gas: -V times the gas flow direction
v_rel__m_per_s = zeros(num_angles, 3);
for a = 1:num_angles
    v_rel__m_per_s(a, :) = -vat_setup.speed__m_per_s * (L_gw(:, :, a) * [-1; 0; 0])';
end

%% vat warm-up: the first OpenGL context and driver start-up must not count against one mesh
source_index = find(levels == 0, 1);
warmup_geometry = vat.geometry.RotatableMeshGeometry(fullfile(files(source_index).folder, ...
                                                              files(source_index).name));
warmup_calculator = vat.loads.PixelForceTorqueCalculator(warmup_geometry, vat_setup.gsi_model, num_pixel, ...
                                                         NumMsaaSamples = num_msaa_samples);
warmup_calculator.calc_aero_load(v_rel__m_per_s(1, :), vat_setup.T_wall__K, vat_setup.aero_cond);
clear warmup_calculator warmup_geometry;

%% ADBSat and vat on every mesh
num_meshes = numel(files);
num_triangles = zeros(num_meshes, 1);
runtime__s = nan(num_meshes, 1);
vat_setup_time__s = zeros(num_meshes, 1);
vat_sweep_time__s = zeros(num_meshes, 1);
force_adb__N = zeros(num_angles, 3, num_meshes);
torque_adb__Nm = zeros(num_angles, 3, num_meshes);
force_vat__N = zeros(num_angles, 3, num_meshes);
torque_vat__Nm = zeros(num_angles, 3, num_meshes);
for k = 1:num_meshes
    mesh_file = fullfile(files(k).folder, files(k).name);
    [~, mesh_name] = fileparts(mesh_file);
    cache_file = fullfile(cache_folder, ['sweep_' mesh_name '.mat']);

    % vat: setup (geometry load, upload, shaders) and sweep timed separately
    timer = tic;
    geometry = vat.geometry.RotatableMeshGeometry(mesh_file);
    calculator = vat.loads.PixelForceTorqueCalculator(geometry, vat_setup.gsi_model, num_pixel, ...
                                                      NumMsaaSamples = num_msaa_samples);
    vat_setup_time__s(k) = toc(timer);
    timer = tic;
    for a = 1:num_angles
        [force_vat__N(a, :, k), torque_vat__Nm(a, :, k)] = calculator.calc_aero_load( ...
            v_rel__m_per_s(a, :), vat_setup.T_wall__K, vat_setup.aero_cond);
    end
    vat_sweep_time__s(k) = toc(timer);
    clear calculator geometry;

    cached = false;
    if isfile(cache_file)
        cache = load(cache_file);
        cached = isequal(cache.pitch__deg, pitch__deg) && cache.sideslip__deg == sideslip__deg;
    end
    if ~cached
        fprintf('ADBSat: %s ...\n', mesh_name);
        timer = tic;
        mat_file = ADBSatImport(mesh_file, cache_folder, 0);
        result_file = ADBSatFcn(mat_file, fullfile(cache_folder, mesh_name), param_eq, ...
                                pitch__deg, sideslip__deg, 1, 0, 0, 1, 0);
        cache = struct();
        cache.aedb = load(result_file, 'aedb').aedb;
        cache.num_triangles = numel(load(mat_file, 'meshdata').meshdata.Areas);
        cache.runtime__s = toc(timer);
        cache.pitch__deg = pitch__deg;
        cache.sideslip__deg = sideslip__deg;
        save(cache_file, '-struct', 'cache');
    end
    num_triangles(k) = cache.num_triangles;
    runtime__s(k) = cache.runtime__s;

    % aedb rows follow the sorted unique angles of attack
    aedb = cache.aedb;
    assert(isequal(round(rad2deg(aedb.aoa(:, 1)'), 6), round(pitch__deg, 6)), 'Unexpected aedb angle order.');
    for a = 1:num_angles
        Cf_w = [aedb.aero.Cf_wX(a, 1); aedb.aero.Cf_wY(a, 1); aedb.aero.Cf_wZ(a, 1)];
        Cm_B = [aedb.aero.Cm_BX(a, 1); aedb.aero.Cm_BY(a, 1); aedb.aero.Cm_BZ(a, 1)];
        force_adb__N(a, :, k) = q__Pa * aedb.AreaRef * (L_gw(:, :, a) * Cf_w);
        torque_adb__Nm(a, :, k) = q__Pa * aedb.AreaRef * aedb.LenRef * (L_gb * Cm_B);
    end
end

[num_triangles, order] = sort(num_triangles);
levels = levels(order);
runtime__s = runtime__s(order);
vat_setup_time__s = vat_setup_time__s(order);
vat_sweep_time__s = vat_sweep_time__s(order);
vat_total_time__s = vat_setup_time__s + vat_sweep_time__s;
files = files(order);
force_adb__N = force_adb__N(:, :, order);
torque_adb__Nm = torque_adb__Nm(:, :, order);
force_vat__N = force_vat__N(:, :, order);
torque_vat__Nm = torque_vat__Nm(:, :, order);

%% vat reference: pixel calculator on the coarsest (unremeshed) mesh
force_ref__N = force_vat__N(:, :, 1);
torque_ref__Nm = torque_vat__Nm(:, :, 1);

%% Relative errors (vector norms, so near-zero components do not dominate)
% The force never vanishes, so it is normalised per angle. The torque passes
% through zero at 0 deg pitch (symmetric geometry), so it is normalised by its
% largest magnitude over the sweep instead.
[error_force, error_torque] = relative_errors(force_adb__N, torque_adb__Nm, force_ref__N, torque_ref__Nm);
[error_force_vat, error_torque_vat] = relative_errors(force_vat__N, torque_vat__Nm, force_ref__N, torque_ref__Nm);
rms_force = sqrt(mean(error_force.^2, 1))';
max_force = max(error_force, [], 1)';
rms_torque = sqrt(mean(error_torque.^2, 1))';
max_torque = max(error_torque, [], 1)';
rms_force_vat = sqrt(mean(error_force_vat.^2, 1))';
max_force_vat = max(error_force_vat, [], 1)';
rms_torque_vat = sqrt(mean(error_torque_vat.^2, 1))';
max_torque_vat = max(error_torque_vat, [], 1)';
speedup = runtime__s ./ vat_total_time__s;

fprintf('\nSentman, pitch %g..%g deg (%d angles), vat reference: pixel %d px, MSAA %d on %s\n', ...
        pitch__deg(1), pitch__deg(end), num_angles, num_pixel, num_msaa_samples, files(1).name);
fprintf('Errors against the reference [%%]\n');
fprintf('%6s %10s | %9s %9s %9s %9s | %9s %9s %9s %9s\n', 'level', 'triangles', ...
        'ADB rmsF', 'ADB maxF', 'ADB rmsM', 'ADB maxM', 'vat rmsF', 'vat maxF', 'vat rmsM', 'vat maxM');
for k = 1:num_meshes
    fprintf('%6s %10d | %9.4f %9.4f %9.4f %9.4f | %9.4f %9.4f %9.4f %9.4f\n', level_label(levels(k)), ...
            num_triangles(k), 100 * [rms_force(k), max_force(k), rms_torque(k), max_torque(k), ...
                                     rms_force_vat(k), max_force_vat(k), rms_torque_vat(k), max_torque_vat(k)]);
end
fprintf('\nRuntime for the whole sweep [s]\n');
fprintf('%6s %10s | %10s %10s %10s | %10s | %9s\n', 'level', 'triangles', 'vat setup', 'vat sweep', ...
        'vat total', 'ADBSat', 'ADB / vat');
for k = 1:num_meshes
    fprintf('%6s %10d | %10.2f %10.2f %10.2f | %10.1f | %9.0f\n', level_label(levels(k)), num_triangles(k), ...
            vat_setup_time__s(k), vat_sweep_time__s(k), vat_total_time__s(k), runtime__s(k), speedup(k));
end

%% Figure 1: convergence
figure('Name', 'ADBSat convergence to vat', 'Position', [100 100 800 550]);
colors = lines(2);
loglog(num_triangles, rms_force, 'o-', 'Color', colors(1, :), 'LineWidth', 1.5, ...
       'DisplayName', 'force, RMS over sweep');
hold on;
loglog(num_triangles, max_force, 'o--', 'Color', colors(1, :), ...
       'DisplayName', 'force, max over sweep');
loglog(num_triangles, rms_torque, 's-', 'Color', colors(2, :), 'LineWidth', 1.5, ...
       'DisplayName', 'torque, RMS over sweep');
loglog(num_triangles, max_torque, 's--', 'Color', colors(2, :), ...
       'DisplayName', 'torque, max over sweep');
% vat on the other meshes against the vat reference (the reference itself has zero error)
% others = 2:num_meshes;
% vat_colors = lines(4);
% loglog(num_triangles(others), rms_force_vat(others), 'd-', 'Color', vat_colors(3, :), 'LineWidth', 1.5, ...
%        'DisplayName', 'vat on mesh, force RMS');
% loglog(num_triangles(others), max_force_vat(others), 'd--', 'Color', vat_colors(3, :), ...
%        'DisplayName', 'vat on mesh, force max');
% loglog(num_triangles(others), rms_torque_vat(others), '^-', 'Color', vat_colors(4, :), 'LineWidth', 1.5, ...
%        'DisplayName', 'vat on mesh, torque RMS');
% loglog(num_triangles(others), max_torque_vat(others), '^--', 'Color', vat_colors(4, :), ...
%        'DisplayName', 'vat on mesh, torque max');
remeshed_counts = num_triangles();
% yline(vat_pixel_floor, '-.', sprintf('vat pixel error (~%d px, MSAA %d)', num_pixel, num_msaa_samples), 'Color', [0.5 0.5 0.5], ...
%       'HandleVisibility', 'off');
grid on;
xlabel('Triangle count');
ylabel('|F_{ADBSat} - F_{vat}| / |F_{vat}|,   |M_{ADBSat} - M_{vat}| / max |M_{vat}|');
title(sprintf('Sentman, pitch %g..%g deg: ADBSat vs vat pixel', pitch__deg(1), pitch__deg(end)));
legend('Location', 'southwest');

%% Figure 2: sweep curves
figure('Name', 'Pitch sweep', 'Position', [120 120 1300 420]);
layout = tiledlayout(1, 3, 'TileSpacing', 'compact');
title(layout, 'Pitch sweep: ADBSat meshes (light = coarse, dark = fine) and vat reference');
mesh_colors = flipud(parula(num_meshes + 2));
mesh_colors = mesh_colors(2:end - 1, :);
quantities = {force_adb__N, force_ref__N, 1, 'F_x [N]'; ...
              force_adb__N, force_ref__N, 3, 'F_z [N]'; ...
              torque_adb__Nm, torque_ref__Nm, 2, 'M_y [Nm]'};
for p = 1:size(quantities, 1)
    nexttile;
    hold on;
    for k = 1:num_meshes
        plot(pitch__deg, quantities{p, 1}(:, quantities{p, 3}, k), '-', 'Color', mesh_colors(k, :), ...
             'DisplayName', sprintf('ADBSat %s (%d tri)', level_label(levels(k)), num_triangles(k)));
    end
    plot(pitch__deg, quantities{p, 2}(:, quantities{p, 3}), 'k-', 'LineWidth', 2, 'DisplayName', 'vat pixel');
    grid on;
    xlabel('Pitch [deg]');
    ylabel(quantities{p, 4});
    if p == 1
        legend('Location', 'best', 'FontSize', 7);
    end
end

%% Figure 3: error map
figure('Name', 'Force error map', 'Position', [140 140 900 450]);
imagesc(pitch__deg, 1:num_meshes, log10(max(error_force', eps)));
set(gca, 'YDir', 'normal', 'YTick', 1:num_meshes, 'YTickLabel', ...
    arrayfun(@(k) sprintf('%s (%d)', level_label(levels(k)), num_triangles(k)), 1:num_meshes, ...
             'UniformOutput', false));
color_bar = colorbar;
color_bar.Label.String = 'log_{10}( |F_{ADBSat} - F_{vat}| / |F_{vat}| )';
xlabel('Pitch [deg]');
ylabel('Mesh (triangles)');
title('Relative force error of ADBSat against vat pixel');

%% Figure 4: runtime comparison
mesh_labels = arrayfun(@(k) sprintf('%s (%d tri)', level_label(levels(k)), num_triangles(k)), 1:num_meshes, ...
                       'UniformOutput', false);
mesh_categories = reordercats(categorical(mesh_labels), mesh_labels);
figure('Name', 'Runtime', 'Position', [160 160 1000 500]);
bars = bar(mesh_categories, [vat_total_time__s, runtime__s], 'grouped');
bars(1).DisplayName = sprintf('vat pixel (%d px, MSAA %d, GPU)', num_pixel, num_msaa_samples);
bars(2).DisplayName = 'ADBSat (CPU, MATLAB)';
%set(gca, 'YScale', 'log');
for k = 1:num_meshes
    text(bars(2).XEndPoints(k), runtime__s(k) * 1.3, sprintf('%.0fx', speedup(k)), ...
         'HorizontalAlignment', 'center', 'VerticalAlignment', 'bottom');
end
ylim([min([vat_total_time__s; runtime__s]) / 2, max(runtime__s) * 1.1]);
grid on;
ylabel('Runtime for the whole sweep [s]');
title(sprintf('Runtime for %d angles (vat: setup + sweep after warm-up; ADBSat: import + sweep)', num_angles));
legend('Location', 'northwest');

%% Local functions
function [error_force, error_torque] = relative_errors(force__N, torque__Nm, force_ref__N, torque_ref__Nm)
    % Force normalised per angle, torque by its largest reference magnitude over the sweep.
    error_force = reshape(vecnorm(force__N - force_ref__N, 2, 2) ./ vecnorm(force_ref__N, 2, 2), ...
                          size(force__N, 1), []);
    error_torque = reshape(vecnorm(torque__Nm - torque_ref__Nm, 2, 2) ./ max(vecnorm(torque_ref__Nm, 2, 2)), ...
                           size(torque__Nm, 1), []);
end

function label = level_label(level)
    if level == 0
        label = 'source';
    else
        label = sprintf('L%d', level);
    end
end
