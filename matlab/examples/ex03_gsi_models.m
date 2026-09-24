%% Example 3: Gas-surface interaction models
% The gas-surface interaction (GSI) model decides how much momentum the gas
% transfers to a surface. VAT has six; they are interchangeable, so the
% same geometry and calculator setup works with every one of them. This
% example compares their drag and lift on the shuttlecock and shows how to
% change a model parameter.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

conditions      = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
surface_temp__K = 300;

obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

%% 1. The six models and their parameters
% alpha_e: energy accommodation coefficient, 0..1
% sigma_n, sigma_t: normal and tangential momentum accommodation, 0..1
% V_w: mean normal velocity of the re-emitted particles [m/s]
models = {
    'Newton',         vat.gsi_models.Newton()
    'Cook',           vat.gsi_models.Cook(0.9)
    'Maxwell',        vat.gsi_models.Maxwell(0.9)
    'Schaaf-Chambre', vat.gsi_models.SchaafChambre(0.9, 0.9)
    'Storch',         vat.gsi_models.Storch(500, 0.9, 0.9)
    'Sentman',        vat.gsi_models.Sentman(1, 0.9)   % 1: exact temperature ratio
};

%% 2. Drag and lift at 20 deg angle of attack
aoa   = 20;   % [deg]
v_rel = 7800 * [cosd(aoa), 0, sind(aoa)];
drag_dir = -v_rel / norm(v_rel);           % drag opposes the motion
lift_dir = [sind(aoa), 0, -cosd(aoa)];     % perpendicular to it in the x-z plane, towards -z

drag = zeros(size(models, 1), 1);
lift = zeros(size(models, 1), 1);
for m = 1:size(models, 1)
    calculator = vat.loads.PixelForceTorqueCalculator(geometry, models{m, 2}, 2000);
    F = calculator.calc_aero_load(v_rel, surface_temp__K, conditions);
    drag(m) = dot(F, drag_dir);
    lift(m) = dot(F, lift_dir);
    fprintf('%-15s drag %.4e N   lift %+.4e N\n', models{m, 1}, drag(m), lift(m));
end

figure('Name', 'GSI models');
bar(categorical(models(:, 1), models(:, 1)), 1e6 * [drag, lift]);
grid on;
ylabel('force [\muN]');
legend('drag', 'lift', 'Location', 'northwest');
title(sprintf('Shuttlecock at %d deg angle of attack', aoa));

%% 3. Changing a model parameter
% Each model has setters for its parameters. A calculator reads them on
% every evaluation, so it does not need to be rebuilt.
sentman    = models{end, 2};
calculator = vat.loads.PixelForceTorqueCalculator(geometry, sentman, 2000);

alpha_e = 0.3:0.1:1;
drag_alpha = zeros(size(alpha_e));
for i = 1:numel(alpha_e)
    sentman.set_alpha_e(alpha_e(i));
    F = calculator.calc_aero_load(v_rel, surface_temp__K, conditions);
    drag_alpha(i) = dot(F, drag_dir);
end

figure('Name', 'Energy accommodation');
plot(alpha_e, 1e6 * drag_alpha, '-o', 'LineWidth', 1.5);
grid on;
xlabel('energy accommodation coefficient \alpha_E');
ylabel('drag [\muN]');
title('Sentman: drag over \alpha_E');
