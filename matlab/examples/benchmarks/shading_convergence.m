%% Benchmark: Shading Convergence and Remeshing Accuracy
% Evaluates shading accuracy and computational cost across raster resolutions
% (num_pixel) for raw imported CAD geometry vs. remeshed geometry, comparing
% Binary (algorithm 0) and Center-of-Pressure (algorithm 1) shading against
% a high-resolution reference solution.
%
% Prerequisites: make sure you have built the matlab binaries with:
%   pixi run build-matlab
% And that you have loaded the Matlab project from Vat.prj

clear; close all;
vat.setLogLevel("warn");

folder   = fullfile(fileparts(mfilename('fullpath')), '..', 'geometries');
obj_file = fullfile(folder, 'shuttlecock_15k.obj');
original = vat.geometry.RotatableMeshGeometry(obj_file);
[remeshed, report] = original.remesh(TriangleCount=20000);

conditions = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
gsi = vat.gsi_models.Sentman(1, 0.9);
COP = 1; BINARY = 0;

%% 1. Shadow boundary visualization
v_rel__m_per_s = [5000 5000 3000];
visibility_before = vat.shading.ShadingPipeline(original, COP, 3000).shade(v_rel__m_per_s);
visibility_after  = vat.shading.ShadingPipeline(remeshed, COP).shade(v_rel__m_per_s);

figure('Name', 'Shading before and after remeshing', 'Position', [100 100 1300 560]);
tiledlayout(1, 2, 'TileSpacing', 'compact');
plot_shading(nexttile, original, visibility_before, v_rel__m_per_s);
title(sprintf('Imported CAD (%d triangles)', report.before.num_triangles));
plot_shading(nexttile, remeshed, visibility_after, v_rel__m_per_s);
title(sprintf('Remeshed (%d triangles)', report.after.num_triangles));
for ax = findobj(gcf, 'Type', 'axes')'
    view(ax, [-1 1 1.2]);
    xlim(ax, [-0.36 -0.12]); ylim(ax, [-0.06 0.06]); zlim(ax, [-0.06 0.06]);
end

%% 2. Accuracy and runtime against num_pixel
% Reference: shuttlecock remeshed to 80k triangles, CoP at 6400 pixels
flows = {[7800 0 0], [7000 3000 1500], [-3000 1000 7000], [2000 -7000 3000]};
[fine, ~] = original.remesh(TriangleCount=80000);
reference = force_per_flow(fine, COP, 6400, flows, gsi, conditions);
scale = max(vecnorm(reference, 2, 2));

num_pixels = [500 1000 2000 3000 4500 6000];
cases = {original, 'imported'; remeshed, 'remeshed'};
algorithms = {COP, 'CoP'; BINARY, 'Binary'};
[err, ms] = deal(zeros(2, 2, numel(num_pixels)));
for c = 1:2
    for a = 1:2
        for k = 1:numel(num_pixels)
            [F, t] = force_per_flow(cases{c, 1}, algorithms{a, 1}, num_pixels(k), flows, gsi, conditions);
            err(c, a, k) = 100 * max(vecnorm(F - reference, 2, 2)) / scale;
            ms(c, a, k) = t;
        end
    end
end

% Automatically selected raster resolution
auto_P = [report.suggested_num_pixel.cop, report.suggested_num_pixel.binary];
auto_err = zeros(1, 2);
for a = 1:2
    auto_err(a) = 100 * max(vecnorm(force_per_flow(remeshed, algorithms{a, 1}, [], flows, gsi, conditions) ...
        - reference, 2, 2)) / scale;
    fprintf('remeshed, %-6s num_pixel chosen: %4d   force error %.3f %%\n', ...
        algorithms{a, 2}, auto_P(a), auto_err(a));
end

figure('Name', 'Accuracy and cost', 'Position', [120 120 1300 460]);
tiledlayout(1, 2, 'TileSpacing', 'compact');
styles = {'-o', '--s'};
nexttile; hold on;
for c = 1:2
    for a = 1:2
        semilogy(num_pixels, squeeze(err(c, a, :)), styles{a}, 'LineWidth', 1.5, ...
            'DisplayName', sprintf('%s, %s', cases{c, 2}, algorithms{a, 2}));
    end
end
plot(auto_P, auto_err, 'wp', 'MarkerSize', 14, 'MarkerFaceColor', 'r', ...
    'DisplayName', 'remeshed, num\_pixel auto');
set(gca, 'YScale', 'log'); grid on; ylim([0.05 10]);
xlabel('num\_pixel'); ylabel('worst force error [%]'); legend('Location', 'northeast');
title('Accuracy Convergence');

nexttile; hold on;
for c = 1:2
    plot(num_pixels, squeeze(ms(c, 1, :)), '-o', 'LineWidth', 1.5, ...
        'DisplayName', sprintf('%s, CoP', cases{c, 2}));
end
grid on; xlabel('num\_pixel'); ylabel('ms per load evaluation'); legend('Location', 'northwest');
title('Evaluation Cost');

%% Helpers
function [F, ms_per_eval] = force_per_flow(geometry, algorithm, num_pixel, flows, gsi, conditions)
    if isempty(num_pixel)
        pipeline = vat.shading.ShadingPipeline(geometry, algorithm);
    else
        pipeline = vat.shading.ShadingPipeline(geometry, algorithm, num_pixel);
    end
    calculator = vat.loads.HybridForceTorqueCalculator(geometry, pipeline, gsi);
    F = zeros(numel(flows), 3);
    tic;
    for i = 1:numel(flows)
        F(i, :) = calculator.calc_aero_load(flows{i}, 300, conditions);
    end
    ms_per_eval = 1e3 * toc / numel(flows);
end

function plot_shading(ax, geometry, visibility, v_rel)
    V = reshape(double(geometry.get_transformed_vertices()), 3, [])';
    F = reshape(1:size(V, 1), 3, [])';
    normals = cross(V(F(:, 2), :) - V(F(:, 1), :), V(F(:, 3), :) - V(F(:, 1), :), 2);
    windward = normals * v_rel(:) > 0;
    state = ones(size(F, 1), 1);                % 1 leeward
    state(windward & visibility(:) > 0.5) = 3;  % 3 lit
    state(windward & visibility(:) <= 0.5) = 2; % 2 windward but shadowed
    patch(ax, 'Faces', F, 'Vertices', V, 'FaceVertexCData', state, ...
        'FaceColor', 'flat', 'EdgeColor', [0.1 0.1 0.1], 'EdgeAlpha', 0.25, 'LineWidth', 0.2);
    colormap(ax, [0.55 0.55 0.55; 0.15 0.25 0.60; 0.98 0.80 0.20]);
    clim(ax, [0.5 3.5]);
    axis(ax, 'equal'); axis(ax, 'off');
    cb = colorbar(ax, 'Ticks', 1:3, 'TickLabels', {'leeward', 'shadowed', 'lit'});
    cb.Location = 'southoutside';
end
