%% Benchmark: accuracy against an analytic result
% Two two-sided flat plates face +x, with the upstream plate partly
% shadowing the +x face of the downstream plate. The leeward faces are
% never shadowed. The reference is assembled from direct GSI evaluations,
% using the visible area and centroid of every surface.
% Compares the hybrid calculators (Binary without and with 8x MSAA, and CoP
% shading) and the
% per-pixel calculator.
%   1. over num_pixel at a fixed flow angle, and
%   2. over the flow angle, which moves the shadow across the back plate,
% each with the second plate meshed coarsely and finely. Timing and
% cost-versus-accuracy plots are included as well.

clear; close all;
vat.setLogLevel("warn");

rho       = 1.2482e-11;
aero_cond = vat.AeroConditions(rho, 934.0, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Sentman(1, 0.9);
speed     = 7800;
T_wall    = 300;

folder  = fullfile(fileparts(mfilename('fullpath')), '..', 'geometries');

meshes  = {'two_two_sided_plates.obj', '2 triangles per face'; ...
           'two_two_sided_plates_fine.obj', 'fine two-sided plates'};
methods = {'hybrid, Binary no MSAA', 'hybrid, Binary 8x MSAA','hybrid, CoP', 'per pixel', 'per pixel 8x MSAA'};

%% 1. Error over num_pixel, flow at 12 deg
% At 12 deg the shadow edge on the back plate cuts through the cells of the
% fine mesh; at angles where it falls exactly on a cell boundary, the
% hybrid calculator would be exact by coincidence.
num_pixels  = [250 500 1000 2000];
num_warmup = 20;   % untimed calls per calculator before measuring
% Per-call time statistics [ms]: median and 25th/75th percentile.
time_ms    = zeros(numel(meshes), numel(num_pixels), numel(methods));
time_lo_ms = zeros(size(time_ms));
time_hi_ms = zeros(size(time_ms));

%% 3. Error over the flow angle, multiple num_pixel values
% 0.1 deg gives ~1800 angles; the run time is (angles + num_warmup) calls for
% each of meshes x num_pixels x methods, so raise step_size for a quick run.
step_size = 0.1;
alphas = (-90+step_size):step_size:(90-step_size);   % [deg]; the shadow slides across the whole back plate
% The analytic solution is only valid for |alpha| < 90 deg (flow not parallel to the plates).
assert(all(abs(alphas) < 90), ...
    'Flow angles must lie strictly inside (-90, 90) deg for the analytic solution.');
error_alpha = zeros(numel(num_pixels), numel(alphas), ...
    numel(methods), size(meshes, 1));   % relative force error
error_abs_alpha = zeros(size(error_alpha));   % absolute force error [N]
torque_error_alpha = zeros(size(error_alpha));       % relative torque error
torque_error_abs_alpha = zeros(size(error_alpha));   % absolute torque error [N m]
total_runs = size(meshes, 1) * numel(num_pixels) * numel(methods);
run_count  = 0;
t_total    = tic;
fprintf('Starting benchmark: %d runs, %d flow angles each.\n', ...
    total_runs, numel(alphas));
for g = 1:size(meshes, 1)
    fprintf('\n[Mesh %d/%d] %s\n', g, size(meshes, 1), meshes{g, 2});
    geometry = vat.geometry.RotatableMeshGeometry(fullfile(folder, meshes{g, 1}));
    for p  = 1:numel(num_pixels)
        fprintf('  num_pixel = %d (%d/%d)...', num_pixels(p), p, numel(num_pixels));
        calculators = make_calculators(geometry, gsi_model, num_pixels(p));
        fprintf('instanciated calculator \n')
        for m = 1:numel(calculators)
            run_count = run_count + 1;
            fprintf('    [%d/%d] %s ... ', run_count, total_runs, methods{m});
            t_run = tic;
            F = zeros(numel(alphas), 3);
            T = zeros(numel(alphas), 3);

            % Warm-up (untimed): first calls allocate buffers and the GPU
            % clocks are still ramping up.
            for i = 1:num_warmup
                calculators{m}.calc_aero_load( ...
                    flow(alphas(mod(i - 1, numel(alphas)) + 1), speed), ...
                    T_wall, aero_cond);
            end

            call_ms = zeros(1, numel(alphas));
            for i = 1:numel(alphas)
                start = tic;
                [force, torque] = calculators{m}.calc_aero_load( ...
                    flow(alphas(i), speed), T_wall, aero_cond);
                call_ms(i) = 1e3 * toc(start);
                F(i,:) = force.';   % returned as columns
                T(i,:) = torque.';
            end
            [time_ms(g, p, m), time_lo_ms(g, p, m), time_hi_ms(g, p, m)] = ...
                summarize_times(call_ms);

            for i = 1:numel(alphas)
                [F_ref, T_ref] = exact_solution_shadowing( ...
                    alphas(i), gsi_model, speed, T_wall, aero_cond);
                error_abs_alpha(p, i, m, g) = norm(F(i, :) - F_ref);
                error_alpha(p, i, m, g) = error_abs_alpha(p, i, m, g) / norm(F_ref);
                torque_error_abs_alpha(p, i, m, g) = norm(T(i, :) - T_ref);
                torque_error_alpha(p, i, m, g) = torque_error_abs_alpha(p, i, m, g) / norm(T_ref);
            end
            fprintf('done in %.1f s (median %.2f ms/eval, IQR %.2f-%.2f)\n', ...
                toc(t_run), time_ms(g, p, m), time_lo_ms(g, p, m), time_hi_ms(g, p, m));
        end
    end
end
fprintf('\nBenchmark finished in %.1f s. Plotting results...\n', toc(t_total));

%% Results
% Every figure is made four times: for the relative and the absolute error,
% each of the force and of the torque (about the body origin).
colors = lines(numel(methods));
styles = {'-o', '--s'};
cost_time_ms = squeeze(time_ms(2, :, :));
cost_lo_ms   = squeeze(time_lo_ms(2, :, :));
cost_hi_ms   = squeeze(time_hi_ms(2, :, :));
shown_p = [1, numel(num_pixels)];   % columns of the flow angle figure

kinds = struct( ...
    'quantity', {'force', 'force', 'torque', 'torque'}, ...
    'name',  {'relative', 'absolute', 'relative', 'absolute'}, ...
    'data',  {error_alpha, error_abs_alpha, torque_error_alpha, torque_error_abs_alpha}, ...
    'scale', {100, 1, 100, 1}, ...
    'label', {'force error [%]', 'absolute force error [N]', ...
              'torque error [%]', 'absolute torque error [N m]'});
for k = 1:numel(kinds)
    kind = kinds(k);
    avg_error = kind.scale * mean(kind.data, 2);
    std_error = kind.scale * std(kind.data, 1, 2);
    plot_error_figures(kind, avg_error, std_error, kind.scale * kind.data, ...
        meshes, methods, num_pixels, alphas, colors, styles, shown_p, ...
        cost_time_ms, cost_lo_ms, cost_hi_ms);
end

%% Local functions
function plot_error_figures(kind, avg_error, std_error, error_alpha, ...
        meshes, methods, num_pixels, alphas, colors, styles, shown_p, ...
        cost_time_ms, cost_lo_ms, cost_hi_ms)
figure('Name', sprintf('%s error over num_pixel (%s)', kind.quantity, kind.name));

hold on;

for g = 1:size(meshes, 1)
    for m = 1:numel(methods)
        y_val = squeeze(avg_error(:, :, m, g));
        y_err = squeeze(std_error(:, :, m, g));

        offset_factor = (m - (numel(methods) + 1) / 2) * 15;
        x_shifted = num_pixels + offset_factor;

        errorbar(x_shifted, y_val, y_err, styles{g}, ...
            'Color', colors(m, :), 'LineWidth', 1.5, ...
            'DisplayName', sprintf('%s, %s', methods{m}, meshes{g, 2}));
    end
end

grid on;
xticks(num_pixels);
xlabel('num\_pixel');
ylabel(kind.label);
legend('Location', 'southwest');
title(sprintf('Two plates, average %s %s error', kind.name, kind.quantity));

figure('Name', sprintf('Cost versus %s accuracy (%s)', kind.quantity, kind.name));
cost_error = squeeze(avg_error(:, 1, :, 2));
hold on;
for m = 1:numel(methods)
    % Horizontal bars span the 25th to 75th percentile of the per-call time.
    errorbar(cost_time_ms(:, m), cost_error(:, m), [], [], ...
        cost_time_ms(:, m) - cost_lo_ms(:, m), ...
        cost_hi_ms(:, m) - cost_time_ms(:, m), ...
        '-o', 'Color', colors(m, :), 'LineWidth', 0.5);
end
set(gca, 'XScale', 'log', 'YScale', 'log');
for i = 1:numel(num_pixels)
    text(cost_time_ms(i, 1), cost_error(i, 1), sprintf('  %d px', num_pixels(i)), ...
        'FontSize', 8);
end
grid on;
xlabel('median time per evaluation [ms] (bars: interquartile range)');
ylabel(kind.label);
legend(methods, 'Location', 'southwest');
title('Lower left is better');

figure('Name', sprintf('%s error over flow angle (%s)', kind.quantity, kind.name));
% One row per mesh; columns: lowest and highest num_pixel only.
tl =tiledlayout(size(meshes, 1), numel(shown_p), 'TileSpacing', 'compact');
axs = gobjects(size(meshes, 1), numel(shown_p));
for g = 1:size(meshes, 1)
    for c = 1:numel(shown_p)
        p = shown_p(c);
        axs(g, c) = nexttile;
        for m = 1:numel(methods)
            semilogy(alphas, error_alpha(p, :, m, g), '-', ...
                'Color', colors(m, :), 'LineWidth', 1, ...
                'DisplayName', methods{m});
            hold on;
        end
        grid on;
        if g == 1
            title(sprintf('num\\_pixel = %d', num_pixels(p)));
        end
        if c == 1
            ylabel({meshes{g, 2}, kind.label});
        end
        if g == 1 && c == 1
            legend('Location', 'northwest');
        end
    end
end
linkaxes(axs(:), 'xy');   % same scale so all panels are comparable
xlabel(tl, 'flow angle in the x-y plane [deg]');
title(tl, sprintf('Two plates, %s %s error over flow angle', kind.name, kind.quantity));
end

function [med, lo, hi] = summarize_times(t)
    % Median and 25th/75th percentile without the Statistics Toolbox.
    t  = sort(t(:));
    n  = numel(t);
    med = median(t);
    lo  = t(max(1, round(0.25 * n)));
    hi  = t(min(n, round(0.75 * n)));
end

function v = flow(alpha__deg, speed)
    v = speed * [cosd(alpha__deg), sind(alpha__deg), 0];
end

function calculators = make_calculators(geometry, gsi_model, num_pixel)
    % Same order as `methods`: hybrid with Binary (no anti-aliasing and
    % 8 samples per pixel) and CoP shading, then per pixel without and with 8 samples.
    calculators = {
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 0, num_pixel, 1), gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 0, num_pixel, 8), gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, vat.shading.ShadingPipeline(geometry, 1, num_pixel), gsi_model)
        vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, num_pixel)
        vat.loads.PixelForceTorqueCalculator(geometry, gsi_model, num_pixel, NumMsaaSamples=8)
    };
end
