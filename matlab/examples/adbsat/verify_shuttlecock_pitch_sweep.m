%% Convergence study: ADBSat vs vat pixel over a pitch sweep
% ADBSat shades a triangle as a whole by testing its centroid; the vat pixel
% calculator shades per pixel and counts partly shaded triangles with their
% exposed part only. With shading off both codes agree to ~1e-6, so the
% difference between them is the shading discretisation of ADBSat, which
% should shrink as the triangles get smaller (error ~ edge length ~ N^-1/2).
%
% This script sweeps the pitch for every remeshed level L1..L<max_level> in
% remeshed/ (see remesh_levels.m; the unremeshed source is not used), compares
% ADBSat on each mesh against one vat pixel reference, and plots
%   1. the RMS and max relative force/torque error over the sweep against the
%      triangle count (convergence),
%   2. Fx, Fz and My against pitch for every ADBSat mesh and the reference,
%   3. the force error over pitch and mesh as a heatmap.
%
% The vat reference is vat on the coarsest level (L1) at num_pixel = 2000 with
% MSAA 8; its own pixel error (a few 1e-4, see verify_pixel_convergence.m) is
% the floor ADBSat's error converges to.
% ADBSat runs on every mesh in every run; its intermediate files go to a fresh
% temporary folder that is deleted afterwards.
%
% Requires ADBSat on the MATLAB path (run ADBSat_dynpath) and the Aerospace
% Toolbox. Frames, normalisation and torque reference as in
% verify_shuttlecock_pitch.m: everything in the mesh frame, torques about the
% mesh origin.

clear; close all;
vat.setLogLevel("debug");

assert(exist('ADBSatFcn', 'file') == 2 && exist('environment', 'file') == 2, ...
       'ADBSat is not on the MATLAB path; run ADBSat_dynpath first.');
assert(exist('atmosnrlmsise00', 'file') == 2, 'atmosnrlmsise00 (Aerospace Toolbox) is required.');

%% Inputs
pitch__deg = -30:1:30;      % ADBSat angles of attack [deg]
sideslip__deg = 0;          % ADBSat angle of sideslip [deg]
num_pixel = 2000;           % raster resolution of the vat reference
num_msaa_samples = 8;       % MSAA samples of the vat reference
max_level = 9;              % highest remesh level L<n> to include
vat_pixel_floor = 4e-4;     % max force change 1000 -> 2000 px at MSAA 8 (verify_pixel_convergence.m)

% Environment and GSI parameters, as in ADBSat's example_GSI.m
param_eq = environment(struct(), 300e3, 0, 0, 1, 0, 140, 140, ones([1, 7]) * 15, 1);
param_eq.gsi_model = 'sentman';
param_eq.alpha = 1;
param_eq.Tw = 300;
vat_setup = adbsat_to_vat(param_eq, 'coeff_sentman');
q__Pa = vat_setup.dynamic_pressure__Pa;

%% Meshes: remeshed levels L1..L<max_level>, sorted by triangle count
folder = fileparts(mfilename('fullpath'));
files = dir(fullfile(folder, 'remeshed', '*_L*_*tri.obj'));
levels = zeros(numel(files), 1);
for k = 1:numel(files)
    levels(k) = str2double(regexp(files(k).name, '_L(\d+)_', 'tokens', 'once'));
end
keep = levels <= max_level;
files = files(keep);
levels = levels(keep);
assert(~isempty(files), 'No remeshed levels in remeshed/; run remesh_levels.m first.');

% ADBSat reads and writes .mat files; give this run its own folder
work_folder = tempname;
mkdir(work_folder);

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
warmup_geometry = vat.geometry.RotatableMeshGeometry(fullfile(files(1).folder, files(1).name));
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

    fprintf('ADBSat: %s ...\n', mesh_name);
    timer = tic;
    mat_file = ADBSatImport(mesh_file, work_folder, 0);
    result_file = ADBSatFcn(mat_file, fullfile(work_folder, mesh_name), param_eq, ...
                            pitch__deg, sideslip__deg, 1, 0, 0, 1, 0);
    runtime__s(k) = toc(timer);
    num_triangles(k) = numel(load(mat_file, 'meshdata').meshdata.Areas);

    % aedb rows follow the sorted unique angles of attack
    aedb = load(result_file, 'aedb').aedb;
    assert(isequal(round(rad2deg(aedb.aoa(:, 1)'), 6), round(pitch__deg, 6)), 'Unexpected aedb angle order.');
    for a = 1:num_angles
        Cf_w = [aedb.aero.Cf_wX(a, 1); aedb.aero.Cf_wY(a, 1); aedb.aero.Cf_wZ(a, 1)];
        Cm_B = [aedb.aero.Cm_BX(a, 1); aedb.aero.Cm_BY(a, 1); aedb.aero.Cm_BZ(a, 1)];
        force_adb__N(a, :, k) = q__Pa * aedb.AreaRef * (L_gw(:, :, a) * Cf_w);
        torque_adb__Nm(a, :, k) = q__Pa * aedb.AreaRef * aedb.LenRef * (L_gb * Cm_B);
    end
end
rmdir(work_folder, 's');

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

%% vat reference: pixel calculator on the coarsest remeshed level
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
semilogx(num_triangles, rms_force, 'o-', 'Color', colors(1, :), 'LineWidth', 1.5, ...
       'DisplayName', 'force, RMS over sweep');
hold on;
semilogx(num_triangles, max_force, 'o--', 'Color', colors(1, :), ...
       'DisplayName', 'force, max over sweep');
semilogx(num_triangles, rms_torque, 's-', 'Color', colors(2, :), 'LineWidth', 1.5, ...
       'DisplayName', 'torque, RMS over sweep');
semilogx(num_triangles, max_torque, 's--', 'Color', colors(2, :), ...
       'DisplayName', 'torque, max over sweep');
remeshed_counts = num_triangles();
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

%% Figure 4: runtime comparison
mesh_labels = arrayfun(@(k) sprintf('%d tri', num_triangles(k)), 1:num_meshes, ...
                       'UniformOutput', false);
mesh_categories = reordercats(categorical(mesh_labels), mesh_labels);
figure('Name', 'Runtime', 'Position', [160 160 1000 500]);
bars = bar(mesh_categories, [vat_total_time__s, runtime__s], 'grouped');
bars(1).DisplayName = sprintf('vat pixel (%d px, MSAA %d, GPU)', num_pixel, num_msaa_samples);
bars(2).DisplayName = 'ADBSat (CPU, MATLAB)';
%set(gca, 'YScale', 'log');

ylim([min([vat_total_time__s; runtime__s]) / 2, max(runtime__s) * 1.1]);
grid on;
ylabel('Runtime for the whole sweep [s]');
title(sprintf('Runtime for %d angles (vat: setup + sweep after warm-up; ADBSat: import + sweep)', num_angles));
legend('Location', 'northwest');

%% Export the figures as TikZ/pgfplots
% Written straight from the data arrays into tikz/: one tikzpicture per file,
% to be \input into a document whose preamble has
%   \usepackage{pgfplots} \usepgfplotslibrary{groupplots} \pgfplotsset{compat=1.18}
tikz_folder = fullfile(folder, 'tikz');
if ~isfolder(tikz_folder)
    mkdir(tikz_folder);
end
sweep_title = sprintf('Sentman, pitch $%g\\ldots%g^\\circ$', pitch__deg(1), pitch__deg(end));

% Figure 1: convergence
lines_tex = [tikz_header(), ...
             {'\begin{semilogxaxis}[', ...
              '  width=0.8\textwidth, height=0.55\textwidth, grid=both,', ...
              ['  xtick={' strjoin(compose('%d', num_triangles'), ',') '}, log ticks with fixed point,'], ...
              '  xticklabel style={rotate=45, anchor=north east, font=\small},', ...
              '  xlabel={Triangle count}, ymin=0,', ...
              '  ylabel={Relative difference ADBSat vs.\ vat [\%]},', ...
              ['  title={' sweep_title ': ADBSat vs.\ vat pixel},'], ...
              '  legend pos=north east, legend cell align=left]'}, ...
             tikz_plot('color=blue, thick, mark=o', num_triangles, 100 * rms_force, ...
                       'force, RMS over sweep'), ...
             tikz_plot('color=blue, dashed, mark=o', num_triangles, 100 * max_force, ...
                       'force, max over sweep'), ...
             tikz_plot('color=red, thick, mark=square', num_triangles, 100 * rms_torque, ...
                       'torque, RMS over sweep'), ...
             tikz_plot('color=red, dashed, mark=square', num_triangles, 100 * max_torque, ...
                       'torque, max over sweep'), ...
             {'\end{semilogxaxis}', '\end{tikzpicture}'}];
writelines(lines_tex, fullfile(tikz_folder, 'pitch_sweep_convergence.tex'));

% Figure 2: sweep curves, one groupplot with three panels
lines_tex = tikz_header();
for k = 1:num_meshes
    lines_tex{end + 1} = sprintf('\\definecolor{mesh%d}{rgb}{%.4f,%.4f,%.4f}', k, mesh_colors(k, :)); %#ok<SAGROW>
end
lines_tex = [lines_tex, ...
             {'\begin{groupplot}[', ...
              '  group style={group size=3 by 1, horizontal sep=1.6cm},', ...
              '  width=0.31\textwidth, height=0.3\textwidth, grid=major,', ...
              '  xlabel={Pitch [$^\circ$]}, legend style={font=\tiny}, legend cell align=left]'}];
axis_labels = {'$F_x$ [N]', '$F_z$ [N]', '$M_y$ [N\,m]'};
for p = 1:size(quantities, 1)
    lines_tex{end + 1} = sprintf('\\nextgroupplot[ylabel={%s}%s]', axis_labels{p}, ...
                                 repmat(', legend to name=sweeplegend', 1, p == 1)); %#ok<SAGROW>
    for k = 1:num_meshes
        lines_tex = [lines_tex, tikz_plot(sprintf('color=mesh%d', k), pitch__deg, ...
                                          quantities{p, 1}(:, quantities{p, 3}, k), ...
                                          sprintf('ADBSat %s (%d tri)', level_label(levels(k)), ...
                                                  num_triangles(k)), p == 1)]; %#ok<AGROW>
    end
    lines_tex = [lines_tex, tikz_plot('color=black, very thick', pitch__deg, ...
                                      quantities{p, 2}(:, quantities{p, 3}), 'vat pixel', p == 1)]; %#ok<AGROW>
end
lines_tex = [lines_tex, {'\end{groupplot}', ...
                         '\node[anchor=north] at ($(group c2r1.south)+(0,-1.2cm)$) {\pgfplotslegendfromname{sweeplegend}};', ...
                         '\end{tikzpicture}'}];
writelines(lines_tex, fullfile(tikz_folder, 'pitch_sweep_curves.tex'));

% Figure 4: runtime bars
level_names = arrayfun(@(k) level_label(levels(k)), 1:num_meshes, 'UniformOutput', false);
lines_tex = [tikz_header(), ...
             {'\begin{axis}[', ...
              '  ybar, bar width=6pt, width=0.8\textwidth, height=0.5\textwidth,', ...
              ['  symbolic x coords={' strjoin(level_names, ',') '}, xtick=data,'], ...
              ['  xticklabels={' strjoin(compose('%d', num_triangles'), ',') '},'], ...
              '  enlarge x limits=0.08, ymin=0, enlarge y limits={upper, value=0.25},', ...
              '  xlabel={Triangle count}, ylabel={Runtime for the whole sweep [s]}, ymajorgrids,', ...
              sprintf('  title={Runtime for %d angles}, legend pos=north west, legend cell align=left]', ...
                      num_angles)}, ...
             tikz_bars('fill=blue!60', level_names, vat_total_time__s, ...
                       sprintf('vat pixel (%d px, MSAA %d, GPU)', num_pixel, num_msaa_samples)), ...
             tikz_bars('fill=orange!80', level_names, runtime__s, 'ADBSat (CPU, MATLAB)'), ...
             {'\end{axis}', '\end{tikzpicture}'}];
writelines(lines_tex, fullfile(tikz_folder, 'pitch_sweep_runtime.tex'));
fprintf('\nTikZ figures written to %s\n', tikz_folder);

%% Local functions
function lines_tex = tikz_header()
    lines_tex = {sprintf('%% Generated by verify_shuttlecock_pitch_sweep.m on %s', ...
                         char(datetime('now', 'Format', 'yyyy-MM-dd HH:mm'))), ...
                 '% Preamble: \usepackage{pgfplots} \usepgfplotslibrary{groupplots} \usetikzlibrary{calc} \pgfplotsset{compat=1.18}', ...
                 '\begin{tikzpicture}'};
end

function lines_tex = tikz_plot(style, x, y, legend_entry, with_legend)
    % One \addplot with inline coordinates; the legend entry only if requested.
    if nargin < 5
        with_legend = true;
    end
    points = compose('(%.6g,%.6g)', [x(:), y(:)]);
    lines_tex = {sprintf('\\addplot[%s] coordinates {%s};', style, strjoin(points', ' '))};
    if with_legend
        lines_tex{end + 1} = sprintf('\\addlegendentry{%s}', legend_entry);
    end
end

function lines_tex = tikz_bars(style, categories, values, legend_entry)
    points = cellfun(@(c, v) sprintf('(%s,%.6g)', c, v), categories(:), num2cell(values(:)), ...
                     'UniformOutput', false);
    lines_tex = {sprintf('\\addplot[%s] coordinates {%s};', style, strjoin(points', ' ')), ...
                 sprintf('\\addlegendentry{%s}', legend_entry)};
end
function [error_force, error_torque] = relative_errors(force__N, torque__Nm, force_ref__N, torque_ref__Nm)
    % Force normalised per angle, torque by its largest reference magnitude over the sweep.
    error_force = reshape(vecnorm(force__N - force_ref__N, 2, 2) ./ vecnorm(force_ref__N, 2, 2), ...
                          size(force__N, 1), []);
    error_torque = reshape(vecnorm(torque__Nm - torque_ref__Nm, 2, 2) ./ max(vecnorm(torque_ref__Nm, 2, 2)), ...
                           size(torque__Nm, 1), []);
end

function label = level_label(level)
    label = sprintf('L%d', level);
end
