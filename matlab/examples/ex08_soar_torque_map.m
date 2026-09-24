%% Example 8: Torque map over all flow directions
% A complete application: the SOAR satellite with one panel turned, and
% its aerodynamic torque for every relative-wind direction. Hundreds of
% evaluations take a few seconds, which is what makes such maps, and
% attitude simulations, practical without precomputed databases.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

conditions      = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
surface_temp__K = 300;
gsi_model       = vat.gsi_models.Sentman(1, 0.95);

%% Geometry: SOAR with its upper panel turned by 45 deg
obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'soar_satellite.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);
geometry.turn_mesh_around_axis(0, deg2rad(0), [-0.15, 0, -0.05], [0, 0, -1]);

calculator = vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, 4000);

%% Sweep over the wind directions
% Angle of attack alpha: rotation of the nominal +x flow about the body y
% axis; sideslip beta: rotation about the body z axis after that.
alpha = 0:5:90;     % [deg]
beta  = 0:5:180;    % [deg]
[ALPHA, BETA] = meshgrid(alpha, beta);
torque = zeros([size(ALPHA), 3]);

tic;
for k = 1:numel(ALPHA)
    v_rel = 7800 * wind_direction(ALPHA(k), BETA(k));
    [~, T] = calculator.calc_aero_load(v_rel, surface_temp__K, conditions);
    [i, j] = ind2sub(size(ALPHA), k);
    torque(i, j, :) = T;
end
elapsed = toc;
fprintf('%d wind directions in %.2f s (%.2f ms each)\n', numel(ALPHA), elapsed, 1e3 * elapsed / numel(ALPHA));

%% Plot
figure('Name', 'Torque map', 'Position', [100 100 1200 400]);
tiledlayout(1, 3, 'TileSpacing', 'compact');
names = {'T_x', 'T_y', 'T_z'};
for axis_index = 1:3
    nexttile;
    surf(ALPHA, BETA, 1e6 * torque(:, :, axis_index));
    shading interp;
    xlabel('\alpha [deg]');
    ylabel('\beta [deg]');
    zlabel([names{axis_index} ' [\muNm]']);
    title(names{axis_index});
end

%% Local functions
function direction = wind_direction(alpha__deg, beta__deg)
    % Nominal +x flow, pitched by alpha about y, then yawed by beta about z.
    direction = [cosd(beta__deg) * cosd(alpha__deg), ...
                 sind(beta__deg) * cosd(alpha__deg), ...
                 sind(alpha__deg)];
end
