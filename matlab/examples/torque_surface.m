%% Achievable torque set of a shuttlecock satellite
% Sweeps two differential panel commands over their full range at one fixed
% attitude and plots the resulting torque vectors in 3D. The result is the
% set of torques the satellite can command without changing attitude.
%
% eta1 drives the up/bottom panel pair, eta2 the left/right pair. Opposing
% panels do not move together: a positive eta deflects one panel of the pair
% outward while its opposite stays stowed, a negative eta swaps their roles.
%
%   eta1 > 0 -> up panel out     eta1 < 0 -> bottom panel out
%   eta2 > 0 -> left panel out   eta2 < 0 -> right panel out
%
% Two commands sweep a 2D surface in 3D torque space, so the reachable set
% is a warped sheet, not a solid volume.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

%% Parameters
alpha__deg   = 45;    % angle of attack
beta__deg    = 25;     % sideslip angle
eta_max__deg = 90;    % panel travel
n_grid       = 20;    % samples per eta, so n_grid^2 load evaluations
num_pixel    = 3000;  % shading resolution

v__m_per_s        = 7800;
surface_temp__K   = 300.0;
rho__kg_per_m3    = 1.2482e-11;
T_atmospheric__K  = 934.0;
particle_mass__kg = 16 * 1.6605390689252e-27;

%% Model
obj_file = fullfile(fileparts(mfilename('fullpath')), ...
    'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

conditions = vat.AeroConditions(rho__kg_per_m3, T_atmospheric__K, particle_mass__kg);
pipeline   = vat.shading.ShadingPipeline(geometry, 1, num_pixel);
calculator = vat.loads.HybridForceTorqueCalculator( ...
    geometry, pipeline, vat.gsi_models.Sentman(1, 0.9));

% Wing hinges, taken from rotate_wings.m. Body frame is x forward, y right,
% z down, so the up wing hinges below the x axis and the left wing left of it.
wings.up     = struct('mesh_id', 4, 'origin', [-0.15 -0.10 -0.05], 'axis', [0 -1  0]);
wings.bottom = struct('mesh_id', 1, 'origin', [-0.15  0.10  0.05], 'axis', [0  1  0]);
wings.left   = struct('mesh_id', 2, 'origin', [-0.15 -0.05  0.10], 'axis', [0  0  1]);
wings.right  = struct('mesh_id', 3, 'origin', [-0.15  0.05 -0.10], 'axis', [0  0 -1]);

%% Flow direction
% Standard aerodynamic definition of angle of attack and sideslip.
a = deg2rad(alpha__deg);
b = deg2rad(beta__deg);
v_rel__m_per_s = v__m_per_s * [cos(a)*cos(b), sin(b), sin(a)*cos(b)];

%% Sweep both commands
eta__rad = deg2rad(linspace(-eta_max__deg, eta_max__deg, n_grid));
Tx = nan(n_grid); Ty = nan(n_grid); Tz = nan(n_grid);

for i = 1:n_grid
    for j = 1:n_grid
        set_panels(geometry, wings, eta__rad(i), eta__rad(j));
        [~, torque__Nm] = calculator.calc_aero_load( ...
            v_rel__m_per_s, surface_temp__K, conditions);
        Tx(i,j) = torque__Nm(1);
        Ty(i,j) = torque__Nm(2);
        Tz(i,j) = torque__Nm(3);
    end
    fprintf('eta1 = %+6.1f deg   (%d/%d)\n', rad2deg(eta__rad(i)), i, n_grid);
end

%% Plotting
Tx__uNm = Tx * 1e6; Ty__uNm = Ty * 1e6; Tz__uNm = Tz * 1e6;
eta1__deg = rad2deg(eta__rad(:)) * ones(1, n_grid);   % colour by eta1

figure(1); clf;
surf(Tx__uNm, Ty__uNm, Tz__uNm, eta1__deg, 'EdgeColor', 'none', 'FaceAlpha', 0.8);
hold on;
scatter3(Tx__uNm(:), Ty__uNm(:), Tz__uNm(:), 6, 'k', 'filled');

% The all-stowed configuration, eta1 = eta2 = 0
k = (n_grid + 1) / 2;
if mod(n_grid, 2) == 1
    plot3(Tx__uNm(k,k), Ty__uNm(k,k), Tz__uNm(k,k), 'rp', ...
        'MarkerSize', 14, 'MarkerFaceColor', 'r');
end

axis equal; grid on; view(3); colormap(parula);
c = colorbar; c.Label.String = 'eta_1 [deg]';
xlabel('T_x [\muNm]'); ylabel('T_y [\muNm]'); zlabel('T_z [\muNm]');
title(['Achievable torques at \alpha = ' num2str(alpha__deg) ' deg, \beta = ' num2str(beta__deg) ' deg']);

%% Helpers
function set_panels(geometry, wings, eta1__rad, eta2__rad)
    % A positive eta deflects one panel of the pair, a negative eta its
    % opposite. turn_mesh_around_axis is absolute, so every panel is
    % commanded on every call and no state carries over between samples.
    turn(geometry, wings.up,     max( eta1__rad, 0));
    turn(geometry, wings.bottom, max(-eta1__rad, 0));
    turn(geometry, wings.left,   max( eta2__rad, 0));
    turn(geometry, wings.right,  max(-eta2__rad, 0));
end

function turn(geometry, wing, angle__rad)
    geometry.turn_mesh_around_axis( ...
        wing.mesh_id, angle__rad, wing.origin, wing.axis);
end
