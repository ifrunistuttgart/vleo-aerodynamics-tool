%% Verification: shuttlecock at 20 deg pitch, vat pixel calculator vs ADBSat
% Computes the total aerodynamic force and torque on every remeshed
% shuttlecock in remeshed/ (see remesh_levels.m) with the Sentman model, once
% with vat.loads.PixelForceTorqueCalculator and once with ADBSat, and plots
% each force and torque component against the triangle count.
%
% Requires ADBSat on the MATLAB path (run ADBSat_dynpath) and the Aerospace
% Toolbox (atmosnrlmsise00, used by ADBSat's environment()).
%
% Frames and conventions:
%   - Both codes get the same mesh. ADBSat calls the mesh frame "geometric"
%     (G); its body frame is L_gb = diag(1, -1, -1) away from it. All results
%     are compared in the mesh frame G.
%   - Pitch = ADBSat angle of attack, sideslip 0. The flow direction in G is
%     ADBSat's vdir = L_gw * [-1; 0; 0], so vat's v_rel (geometry relative to
%     the atmosphere) is -V * vdir.
%   - Torques are about the mesh origin in both codes.
%   - ADBSat evaluates Sentman on every panel, leeward ones included, and
%     shades windward panels by their centroid. vat in its default
%     (non-hyperthermal) regime also evaluates leeward triangles without
%     shading, but shades windward ones per pixel, so partly shaded triangles
%     count with only their exposed part.

clear; close all;
vat.setLogLevel("warn");

assert(exist('ADBSatFcn', 'file') == 2 && exist('environment', 'file') == 2, ...
       'ADBSat is not on the MATLAB path; run ADBSat_dynpath first.');
assert(exist('atmosnrlmsise00', 'file') == 2, 'atmosnrlmsise00 (Aerospace Toolbox) is required.');

%% Inputs
pitch__deg = 10;   % ADBSat angle of attack [deg]
sideslip__deg = 10; % ADBSat angle of sideslip [deg]
num_pixel = 2000;  % raster resolution of the vat pixel calculator

% Environment and GSI parameters, as in ADBSat's example_GSI.m
h = 300e3;
lat = 0;
lon = 0;
dayOfYear = 1;
UTseconds = 0;
f107Average = 140;
f107Daily = 140;
magneticIndex = ones([1, 7]) * 15;
AnO = 1;

param_eq = environment(struct(), h, lat, lon, dayOfYear, UTseconds, f107Average, f107Daily, magneticIndex, AnO);
param_eq.gsi_model = 'sentman';
param_eq.alpha = 1;
param_eq.Tw = 300;

folder = fileparts(mfilename('fullpath'));
mesh_files = dir(fullfile(folder, 'remeshed', '*.obj'));
assert(~isempty(mesh_files), 'No meshes in remeshed/; run remesh_levels.m first.');
adbsat_folder = fullfile(tempdir, 'vat_adbsat_verification');
if ~isfolder(adbsat_folder)
    mkdir(adbsat_folder);
end

%% Flow direction, identical to ADBSat's calc_coeff
aoa = deg2rad(pitch__deg);
aos = deg2rad(sideslip__deg);
L_wb = [cos(aos) * cos(aoa), sin(aos), sin(aoa) * cos(aos); ...
        -sin(aos) * cos(aoa), cos(aos), -sin(aoa) * sin(aos); ...
        -sin(aoa), 0, cos(aoa)];       % body to wind
L_gb = [1 0 0; 0 -1 0; 0 0 -1];        % body to geometric (mesh)
L_gw = L_gb * L_wb';                   % wind to geometric
flow_dir = L_gw * [-1; 0; 0];          % gas velocity direction in the mesh frame

vat_setup = adbsat_to_vat(param_eq, 'coeff_sentman');
v_rel__m_per_s = -vat_setup.speed__m_per_s * flow_dir';

%% Evaluate every mesh with both codes
num_meshes = numel(mesh_files);
num_triangles = zeros(num_meshes, 1);
force_vat__N = zeros(num_meshes, 3);
torque_vat__Nm = zeros(num_meshes, 3);
force_adb__N = zeros(num_meshes, 3);
torque_adb__Nm = zeros(num_meshes, 3);

for k = 1:num_meshes
    mesh_file = fullfile(mesh_files(k).folder, mesh_files(k).name);
    [~, mesh_name] = fileparts(mesh_file);

    % vat
    geometry = vat.geometry.RotatableMeshGeometry(mesh_file);
    quality = geometry.get_mesh_quality();
    num_triangles(k) = quality.num_triangles;
    disp("compute pixel")
    calculator = vat.loads.PixelForceTorqueCalculator(geometry, vat_setup.gsi_model, num_pixel, NumMsaaSamples = 8);
    [force__N, torque__Nm] = calculator.calc_aero_load(v_rel__m_per_s, vat_setup.T_wall__K, vat_setup.aero_cond);
    disp("Done")
    force_vat__N(k, :) = force__N;
    torque_vat__Nm(k, :) = torque__Nm;

    % ADBSat: coefficients are normalised by q * AreaRef (forces) and
    % q * AreaRef * LenRef (moments); Cf_w is in wind axes, Cm_B in body axes.
    disp("compute adbsat")
    mat_file = ADBSatImport(mesh_file, adbsat_folder, 0);
    result_file = ADBSatFcn(mat_file, fullfile(adbsat_folder, [mesh_name '_result']), param_eq, ...
                            pitch__deg, sideslip__deg, 1, 0, 0, 0, 0);
    result = load(result_file, 'Cf_w', 'Cm_B', 'AreaRef', 'LenRef');
    q__Pa = vat_setup.dynamic_pressure__Pa;
    force_adb__N(k, :) = q__Pa * result.AreaRef * (L_gw * result.Cf_w);
    torque_adb__Nm(k, :) = q__Pa * result.AreaRef * result.LenRef * (L_gb * result.Cm_B);
    disp("Done")
    meshdata = load(mat_file, 'meshdata').meshdata;
    assert(numel(meshdata.Areas) == num_triangles(k), 'ADBSat and vat read different triangle counts.');
end

[num_triangles, order] = sort(num_triangles);
force_vat__N = force_vat__N(order, :);
torque_vat__Nm = torque_vat__Nm(order, :);
force_adb__N = force_adb__N(order, :);
torque_adb__Nm = torque_adb__Nm(order, :);

%% Report
fprintf('Sentman, pitch %g deg, sideslip %g deg, mesh frame, torque about the origin\n', ...
        pitch__deg, sideslip__deg);
fprintf('%9s | %-34s | %-34s\n', '', 'force vat - ADBSat [% of |F|]', 'torque vat - ADBSat [% of |M|]');
fprintf('%9s | %10s %10s %10s | %10s %10s %10s\n', 'triangles', 'x', 'y', 'z', 'x', 'y', 'z');
for k = 1:num_meshes
    fprintf('%9d | %10.3f %10.3f %10.3f | %10.3f %10.3f %10.3f\n', num_triangles(k), ...
            100 * (force_vat__N(k, :) - force_adb__N(k, :)) / norm(force_adb__N(k, :)), ...
            100 * (torque_vat__Nm(k, :) - torque_adb__Nm(k, :)) / norm(torque_adb__Nm(k, :)));
end

%% Plot
component_names = {'F_x [N]', 'F_y [N]', 'F_z [N]', 'M_x [Nm]', 'M_y [Nm]', 'M_z [Nm]'};
values_vat = [force_vat__N, torque_vat__Nm];
values_adb = [force_adb__N, torque_adb__Nm];
figure('Name', sprintf('Shuttlecock, Sentman, pitch %g deg', pitch__deg), 'Position', [100 100 1200 650]);
layout = tiledlayout(2, 3, 'TileSpacing', 'compact');
title(layout, sprintf('Shuttlecock, Sentman, pitch %g deg (mesh frame, torque about origin)', pitch__deg));
for c = 1:6
    nexttile;
    semilogx(num_triangles, values_vat(:, c), 'o-', 'LineWidth', 1.25, 'DisplayName', 'vat pixel');
    hold on;
    semilogx(num_triangles, values_adb(:, c), 's--', 'LineWidth', 1.25, 'DisplayName', 'ADBSat');
    grid on;
    ylabel(component_names{c});
    if c > 3
        xlabel('Triangle count');
    end
    if c == 1
        legend('Location', 'best');
    end
end
