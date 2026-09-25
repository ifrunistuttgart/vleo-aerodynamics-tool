%% Benchmark: how smooth are the loads over the attitude?
% Sweeps the flow from the nose (yaw 0 deg) to the side (yaw 90 deg) in
% 0.25 deg steps. On the way the body and the wings start to shade each
% other, and the pitch torque follows. Every method sees the geometry at
% pixel resolution, so its result moves in small jumps:
%   hybrid:    when a triangle switches between visible and hidden,
%   per pixel: when a shadow edge or an outline crosses a pixel. Surfaces
%              seen almost edge-on cover few pixels with a large wetted
%              area each (pixel area / cos(delta)), so they jump most.
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

obj_file = fullfile(fileparts(mfilename('fullpath')), '..', 'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

yaw        = 0:0.25:90;   % [deg]
num_pixels = [500 1000 2000 4000];
methods    = {'hybrid, Binary', 'hybrid, CoP', 'per pixel'};

%% Reference
reference = sweep(vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, 8000), yaw, T_wall, aero_cond);
scale = max(reference) - min(reference);

%% Every method at every resolution
pitch = zeros(numel(yaw), numel(methods), numel(num_pixels));
for k = 1:numel(num_pixels)
    N = num_pixels(k);
    calculators = {
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 0, N), gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 1, N), gsi_model)
        vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, N)
    };
    for m = 1:numel(calculators)
        pitch(:, m, k) = sweep(calculators{m}, yaw, T_wall, aero_cond);
    end
    fprintf('num_pixel %4d done\n', N);
end

% Largest jump between two neighbouring yaw angles, in % of the torque range.
max_jump = squeeze(max(abs(diff(pitch - reference, 1, 1)), [], 1)) / scale * 100;   % methods x num_pixels
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
figure('Name', 'Pitch torque over yaw', 'Position', [100 100 900 500]);
stairs(yaw, 1e6 * pitch(:, :, shown), 'LineWidth', 1);
hold on;
stairs(yaw, 1e6 * reference, 'k', 'LineWidth', 1.5);
grid on;
xlabel('yaw [deg]');
ylabel('pitch torque T_y [\muNm]');
legend([methods, {'per pixel, 8000 px (reference)'}], 'Location', 'best');
title(sprintf('Flow swinging from the nose to the side, %d x %d pixels', num_pixels(shown), num_pixels(shown)));

figure('Name', 'Jumps over num_pixel');
loglog(num_pixels, max_jump', '-o', 'LineWidth', 1.5);
grid on;
xticks(num_pixels);
xlabel('num\_pixel');
ylabel('largest jump [% of the torque range]');
legend(methods, 'Location', 'southwest');
title('Jumps between 0.25 deg yaw steps');

%% Local functions
function pitch = sweep(calculator, yaw, T_wall, aero_cond)
    pitch = zeros(numel(yaw), 1);
    for i = 1:numel(yaw)
        v_rel = 7800 * [cosd(yaw(i)), sind(yaw(i)), 0.05];
        [~, T] = calculator.calc_aero_load(v_rel, T_wall, aero_cond);
        pitch(i) = T(2);
    end
end
