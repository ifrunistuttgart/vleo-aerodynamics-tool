%% Example 6: Loads over the attitude
% Sweeps the angle of attack and plots drag, lift and pitch torque. The
% calculator is built once; each further attitude costs one render.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

conditions      = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
surface_temp__K = 300;
gsi_model       = vat.gsi_models.Sentman(1, 0.9);

obj_file   = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_15k.obj');
geometry   = vat.geometry.RotatableMeshGeometry(obj_file);
calculator = vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, 1000);

%% Sweep the angle of attack
% The flow turns in the body x-z plane: 0 deg is nose first.
aoa   = -60:1:60;   % [deg]
drag  = zeros(size(aoa));
lift  = zeros(size(aoa));
pitch = zeros(size(aoa));

tic;
for i = 1:numel(aoa)
    v_rel = 7800 * [cosd(aoa(i)), 0, sind(aoa(i))];
    [F, T] = calculator.calc_aero_load(v_rel, surface_temp__K, conditions);

    drag(i)  = dot(F, -v_rel / norm(v_rel));
    lift(i)  = dot(F, [sind(aoa(i)), 0, -cosd(aoa(i))]);   % towards -z
    pitch(i) = T(2);
end
fprintf('%d attitudes in %.2f s\n', numel(aoa), toc);

%% Plot
figure('Name', 'Attitude sweep', 'Position', [100 100 700 600]);
tiledlayout(2, 1, 'TileSpacing', 'compact');

nexttile;
plot(aoa, 1e6 * [drag; lift], 'LineWidth', 1.5);
grid on;
ylabel('force [\muN]');
legend('drag', 'lift', 'Location', 'best');
title('Shuttlecock, flow in the body x-z plane');

nexttile;
plot(aoa, 1e6 * pitch, 'LineWidth', 1.5);
grid on;
xlabel('angle of attack [deg]');
ylabel('pitch torque T_y [\muNm]');
