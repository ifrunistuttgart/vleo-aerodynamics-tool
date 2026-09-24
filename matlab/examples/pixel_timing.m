%% Computation time over the number of pixels
% Times one force/torque evaluation for the hybrid calculator (Binary and
% CoP shading) and the per-pixel calculator at several resolutions. Setting
% up a calculator is done once and not timed; what matters for sweeps over
% attitudes or wing angles is the cost of each further evaluation.
%
% At low resolutions the fixed costs dominate: the call from MATLAB and,
% for the hybrid calculator, the GSI evaluation per triangle on the CPU.
% The per-pixel calculator does its work per pixel, so it gets cheaper than
% hybrid at low resolutions and grows with num_pixel^2 at high ones.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

aero_cond = vat.AeroConditions(1e-9, 934, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Sentman(1, 0.95);
v_rel     = 7800 * [cosd(20), sind(10), 0];
T_wall    = 300;

obj_file = fullfile(fileparts(mfilename('fullpath')), ...
    'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

num_pixels = [250 500 1000 2000 4000];
methods    = {'hybrid, Binary', 'hybrid, CoP', 'per pixel'};
time_ms    = zeros(numel(num_pixels), numel(methods));

for i = 1:numel(num_pixels)
    N = num_pixels(i);

    pipeline_binary = vat.shading.ShadingPipeline(geometry, 0, N);
    pipeline_cop    = vat.shading.ShadingPipeline(geometry, 1, N);
    calculators = {
        vat.loads.HybridForceTorqueCalculator(geometry, pipeline_binary, gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, pipeline_cop, gsi_model)
        vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, N)
    };

    for m = 1:numel(calculators)
        % timeit runs the call repeatedly and returns the median, in seconds.
        time_ms(i, m) = 1e3 * timeit(@() calculators{m}.calc_aero_load(v_rel, T_wall, aero_cond));
    end
    fprintf('%5d x %-5d pixels: %7.2f ms (Binary)  %7.2f ms (CoP)  %7.2f ms (pixel)\n', ...
        N, N, time_ms(i, :));

    clear calculators pipeline_binary pipeline_cop
end

figure('Name', 'Computation time');
loglog(num_pixels, time_ms, '-o', 'LineWidth', 1.5);
grid on;
xticks(num_pixels);
xlabel('num\_pixel (render is num\_pixel x num\_pixel)');
ylabel('time per evaluation [ms]');
legend(methods, 'Location', 'northwest');
title(sprintf('%d triangles, Sentman', geometry.get_num_triangles()));
