%% How smooth are per-pixel loads over the attitude?
% Sweeps the flow from the nose (yaw 0 deg) to the side (yaw 90 deg) in
% small steps. On the way the body and the wings start to shade each other,
% and the pitch torque follows. The per-pixel calculator sees the geometry
% only at pixel resolution, so its result moves in small jumps when a shadow
% edge or a surface outline crosses a pixel. This shows how large those
% jumps are for several values of num_pixel, measured against a run at a
% much higher resolution.
%
% The largest jumps come from surfaces seen almost edge-on. Such a surface
% covers only a few pixels, but each pixel stands for a large wetted area
% (pixel area / cos(delta)), so gaining or losing a few pixels moves the
% load noticeably. last_areas() shows it: at 250 pixels the wetted area
% changes by a fifth between two neighbouring yaw angles while the
% projected area barely moves.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

aero_cond = vat.AeroConditions(1e-9, 934, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Sentman(1, 0.95);
T_wall    = 300;

obj_file = fullfile(fileparts(mfilename('fullpath')), ...
    'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

yaw        = 0:0.25:90;                   % [deg]
num_pixels = [250 500 1000 2000 4000];
N_ref      = 8000;                         % reference resolution

%% Pitch torque over yaw for every resolution
all_N = [num_pixels, N_ref];
pitch = zeros(numel(yaw), numel(all_N));
for k = 1:numel(all_N)
    calculator = vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, all_N(k));
    for i = 1:numel(yaw)
        v_rel = 7800 * [cosd(yaw(i)), sind(yaw(i)), 0.05];
        [~, T] = calculator.calc_aero_load(v_rel, T_wall, aero_cond);
        pitch(i, k) = T(2);
    end
    fprintf('num_pixel %4d done\n', all_N(k));
end
reference = pitch(:, end);
pitch     = pitch(:, 1:end-1);

%% How large are the jumps?
% error: distance from the reference curve.
% jump:  how much one 0.25 deg step differs from the reference's step, i.e.
%        the part of a step that is pixel noise, not physics.
% Both relative to the range the pitch torque covers over the sweep.
scale = max(reference) - min(reference);
error = pitch - reference;
jump  = diff(error);

max_error = max(abs(error)) / scale;
max_jump  = max(abs(jump)) / scale;
fprintf('\n num_pixel   max error   max jump   (relative to the torque range)\n');
fprintf(' %9d   %8.2f %%  %8.2f %%\n', [num_pixels; 100 * max_error; 100 * max_jump]);

%% Plots
labels = arrayfun(@(N) sprintf('%d px', N), num_pixels, 'UniformOutput', false);

figure('Name', 'Pitch torque over yaw', 'Position', [100 100 900 700]);
tiledlayout(2, 1, 'TileSpacing', 'compact');

nexttile;
plot(yaw, pitch * 1e6, 'LineWidth', 1);
hold on;
plot(yaw, reference * 1e6, 'k', 'LineWidth', 1.5);
grid on;
ylabel('pitch torque T_y [\muNm]');
legend([labels, {sprintf('%d px (reference)', N_ref)}], 'Location', 'best');
title('Flow swinging from the nose to the side');

nexttile;
plot(yaw, 100 * error / scale, 'LineWidth', 1);
grid on;
xlabel('yaw [deg]');
ylabel('deviation from reference [% of range]');
legend(labels, 'Location', 'best');

figure('Name', 'Jumps over resolution');
loglog(num_pixels, 100 * max_error, '-o', num_pixels, 100 * max_jump, '-s', 'LineWidth', 1.5);
hold on;
% A line falling like 1/num_pixel for comparison: the jumps come from pixels
% along edges, so that is how they should shrink.
loglog(num_pixels, 100 * max_jump(1) * num_pixels(1) ./ num_pixels, 'k--');
grid on;
xticks(num_pixels);
xlabel('num\_pixel');
ylabel('% of the torque range');
legend('largest deviation from reference', 'largest jump between two steps', '\propto 1/num\_pixel', ...
    'Location', 'southwest');
title('Pixel noise in the pitch torque, 0.25 deg yaw steps');
