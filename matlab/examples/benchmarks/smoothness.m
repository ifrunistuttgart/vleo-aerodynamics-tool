%% Benchmark: how smooth are the loads over the attitude?
% Sweeps the flow with pitch alpha from 0 deg to 90 deg in
% 0.25 deg steps. On the way the body and the wings start to shade each
% other, and the pitch torque follows. Every method sees the geometry at
% pixel resolution, so its result moves in small jumps:
%   hybrid:     when a triangle switches between visible and hidden,
% A jump is the part of a 0.25 deg step that differs from the step of a
% reference curve, the per-pixel result at 8000 pixels.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

aero_cond = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Sentman(1, 0.95);
T_wall    = 300;

obj_file  = fullfile(fileparts(mfilename('fullpath')), '..', 'geometries', 'two_two_sided_plates_fine.obj');
geometry  = vat.geometry.RotatableMeshGeometry(obj_file);

alpha      = 0:0.25:90;   % [deg]
num_pixels = [500 1000 2000 4000];
methods    = {'hybrid, Binary', 'hybrid, CoP'};
speed_val  = 7800;

%% Reference Solution
T_ref = zeros(numel(alpha), 1);
for i = 1:numel(alpha)
    [~, T] = exact_solution_shadowing(alpha(i), gsi_model, speed_val, T_wall, aero_cond);
    T_ref(i) = T(2);
end

%% Every Method at Every Resolution
pitch = zeros(numel(alpha), numel(methods), numel(num_pixels));
for k = 1:numel(num_pixels)
    N = num_pixels(k);
    calculators = {
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 0, N), gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 1, N), gsi_model)
    };
    for m = 1:numel(calculators)
        pitch(:, m, k) = sweep(calculators{m}, alpha, T_wall, aero_cond, speed_val);
    end
    fprintf('num_pixel %4d done\n', N);
end

% Largest jump between two neighbouring alpha angles, in % of the torque range.
torque_range = max(T_ref) - min(T_ref);
max_jump = squeeze(max(abs(diff(pitch - T_ref, 1, 1)), [], 1)) / torque_range * 100;  % methods x num_pixels

fprintf('\n num_pixel');
fprintf('  %16s', methods{:});
fprintf('   (largest jump, %% of the pitch torque range)\n');
for k = 1:numel(num_pixels)
    fprintf(' %9d', num_pixels(k));
    fprintf('  %16.2f', max_jump(:, k));
    fprintf('\n');
end

%% Plots
shown = find(num_pixels == 1000);
figure('Name', 'Pitch torque over alpha', 'Position', [100 100 900 500]);
stairs(alpha, 1e6 * pitch(:, :, shown), 'LineWidth', 1);
hold on;
stairs(alpha, 1e6 * T_ref, 'k', 'LineWidth', 1.5);
grid on;
xlabel('alpha [deg]');
ylabel('pitch torque T_y [\muNm]');
legend([methods, {'analytical (reference)'}], 'Location', 'best');
title(sprintf('Flow pitching from 0 to 90 deg, %d x %d pixels', num_pixels(shown), num_pixels(shown)));

figure('Name', 'Jumps over num_pixel');
loglog(num_pixels, max_jump, '-o', 'LineWidth', 1.5);
grid on;
xticks(num_pixels);
xlabel('num\_pixel');
ylabel('largest jump [% of the torque range]');
legend(methods, 'Location', 'southwest');
title('Jumps between 0.25 deg alpha steps');

%% Local Functions
function pitch_vals = sweep(calculator, alpha_angles, T_wall, aero_cond, speed)
    pitch_vals = zeros(numel(alpha_angles), 1);
    for i = 1:numel(alpha_angles)
        v_rel = flow(alpha_angles(i), speed);
        [~, T] = calculator.calc_aero_load(v_rel, T_wall, aero_cond);
        pitch_vals(i) = T(2);
    end
end

function v = flow(alpha__deg, speed)
    v = speed * [cosd(alpha__deg), sind(alpha__deg), 0];
end
