%% remeshing: shading and accuracy
% What remeshing changes for the shading pass, and for the loads.
%
% A triangle is lit or shadowed as a whole. Along a shadow edge the result
% is therefore only as fine as the triangles there: a sliver lying across
% the edge counts fully lit or fully dark. This script shows the shadow on
% the imported and on the remeshed shuttlecock, then measures how far the
% force is from a fine reference for each, over num_pixel and both shading
% algorithms.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_15k.obj');
original = vat.geometry.RotatableMeshGeometry(obj_file);
[remeshed, report] = original.remesh(TriangleCount=20000);

conditions = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
gsi = vat.gsi_models.Sentman(1, 0.9);
COP = 1; BINARY = 0;

%% 1. The shadow, before and after
% The body shadows the wings behind it. Seen from the side -- along the
% flow every visible triangle would be lit -- and zoomed in on the wings.
% Each triangle is lit or shadowed as a whole, so the shadow edge is resolved
% in steps of one triangle: on the remeshed geometry in even steps of one
% size, on the import in whatever the export produced, from the wide
% triangles of the large faces to the 0.19 mm slivers of the rims. The next
% section measures what that does to the force.
v_rel__m_per_s = [5000 5000 3000];
visibility_before = vat.shading.ShadingPipeline(original, COP, 3000).shade(v_rel__m_per_s);
visibility_after = vat.shading.ShadingPipeline(remeshed, COP).shade(v_rel__m_per_s);

figure('Name', 'Shading before and after remeshing', 'Position', [100 100 1300 560]);
tiledlayout(1, 2, 'TileSpacing', 'compact');
plot_shading(nexttile, original, visibility_before, v_rel__m_per_s);
title(sprintf('imported, %d triangles', report.before.num_triangles));
plot_shading(nexttile, remeshed, visibility_after, v_rel__m_per_s);
title(sprintf('remeshed, %d triangles', report.after.num_triangles));
for ax = findobj(gcf, 'Type', 'axes')'
    view(ax, [-1 1 1.2]);
    xlim(ax, [-0.36 -0.12]); ylim(ax, [-0.06 0.06]); zlim(ax, [-0.06 0.06]);
end

%% 2. Accuracy against num_pixel
% Reference: the same shuttlecock remeshed to 80k triangles, CoP at 6400
% pixels. Error: the worst force difference over four flow directions,
% relative to the largest reference force.
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

% And what the pipeline picks when num_pixel is left out.
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
    'DisplayName', 'remeshed, num\_pixel left out');
set(gca, 'YScale', 'log'); grid on; ylim([0.05 10]);
xlabel('num\_pixel'); ylabel('worst force error [%]'); legend('Location', 'northeast');
title('accuracy');

nexttile; hold on;
for c = 1:2
    plot(num_pixels, squeeze(ms(c, 1, :)), '-o', 'LineWidth', 1.5, ...
        'DisplayName', sprintf('%s, CoP', cases{c, 2}));
end
grid on; xlabel('num\_pixel'); ylabel('ms per load evaluation'); legend('Location', 'northwest');
title('cost');

% What to take away: remeshed, CoP levels off at about 2000 pixels, and
% leaving num_pixel out lands there. The import needs 6000 and more to come
% close, because its 0.19 mm slivers must be resolved too -- at about the
% same cost per pixel. Binary counts every partly shadowed triangle as lit,
% so its error is set by the triangle size and more pixels do not help: on
% a remeshed geometry, use CoP.

%% 3. The same in the interactive viewer
% Each window blocks until closed.
vat.visualization.show_shading(original, visibility_before, v_rel__m_per_s);
vat.visualization.show_shading(remeshed, visibility_after, v_rel__m_per_s);

%% Helpers
function [F, ms_per_eval] = force_per_flow(geometry, algorithm, num_pixel, flows, gsi, conditions)
    % Force for each flow direction; num_pixel = [] lets the pipeline choose.
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
    % Lit, shadowed and leeward triangles.
    V = reshape(double(geometry.get_vertices()), 3, [])';
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
