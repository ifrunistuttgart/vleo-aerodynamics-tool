%% remeshing: before and after
% What remeshing does to a mesh, shown side by side.
%
% The shuttlecock below is a typical CAD export: its triangles are long thin
% slivers. A triangle is lit or shadowed as a whole, so a sliver lying across
% a shadow edge is counted fully lit or fully dark, and one narrower than a
% pixel can fall between pixels entirely. Remeshing replaces them with
% near-equilateral triangles of one size.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

%% 1. Import and remesh
obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_15k.obj');
original = vat.geometry.RotatableMeshGeometry(obj_file);
[remeshed, report] = original.remesh(TriangleCount=20000);

fprintf('%-9s %9s %14s %14s %16s\n', '', 'triangles', 'aspect median', 'aspect p95', 'width p05 [mm]');
print_quality('before', report.before);
print_quality('after', report.after);
fprintf('area change %+.3f %%   (flat faces keep their area exactly)\n', report.area_change__percent);

%% 2. Side by side, coloured by aspect ratio
% Aspect ratio is 1 for an equilateral triangle. The export gave every face
% of the model 512 triangles whatever its size. On a wing's large face that
% is harmless (top row). On its 3 mm rim it makes 512 slivers 0.19 mm wide
% (bottom row, seen from above) -- and the rims face the flow.
[aspect_before, width_before] = triangle_metrics(original.get_vertices());
[aspect_after, width_after] = triangle_metrics(remeshed.get_vertices());

figure('Name', 'Before and after remeshing, coloured by aspect ratio', 'Position', [100 100 1400 700]);
layout = tiledlayout(2, 2, 'TileSpacing', 'compact');
geometries = {original, remeshed};
aspects = {aspect_before, aspect_after};
labels = {sprintf('imported (%d triangles)', report.before.num_triangles), ...
          sprintf('remeshed (%d triangles)', report.after.num_triangles)};
for k = 1:2                                    % top row: a wing's large face and the body end
    ax = nexttile(k); plot_mesh(ax, geometries{k}, log10(aspects{k}));
    view(ax, [0 -1 0]);                        % from -y: x to the right, z up
    xlim(ax, [-0.36 -0.10]); zlim(ax, [-0.06 0.06]);
    title(ax, [labels{k} ': large wing face']);
end
for k = 1:2                                    % bottom row: 14 mm of one wing's 3 mm rim
    ax = nexttile(k + 2); plot_mesh(ax, geometries{k}, log10(aspects{k}));
    view(ax, [0 0 1]);                         % from above: x to the right, y up
    xlim(ax, [-0.262 -0.248]); ylim(ax, [0.0455 0.0515]);
    title(ax, [labels{k} ': 3 mm wing rim, from above']);
end
for ax = findobj(layout, 'Type', 'axes')'
    clim(ax, [0 log10(40)]);
end
cb = colorbar(nexttile(4));
cb.Ticks = log10([1 2 5 10 20 40]); cb.TickLabels = string([1 2 5 10 20 40]);
cb.Label.String = 'aspect ratio (1 = equilateral)';

%% 3. Distributions
% The narrowest width, not the area, decides whether the shading raster
% resolves a triangle: it must span a few pixels. The dashed line marks one
% pixel at num_pixel = 1000 (the frustum is 2R across).
pixel__mm = 1e3 * 2 * report.before.bounding_sphere_radius__m / 1000;

figure('Name', 'Triangle shape and size', 'Position', [120 120 1200 420]);
tiledlayout(1, 2, 'TileSpacing', 'compact');
nexttile; hold on;
edges = logspace(0, log10(60), 50);
histogram(aspect_before, edges, 'Normalization', 'probability');
histogram(aspect_after, edges, 'Normalization', 'probability');
set(gca, 'XScale', 'log'); grid on;
xlabel('aspect ratio'); ylabel('fraction of triangles');
legend('imported', 'remeshed'); title('shape');

nexttile; hold on;
edges = logspace(log10(0.05), log10(20), 50);
histogram(1e3 * width_before, edges, 'Normalization', 'probability');
histogram(1e3 * width_after, edges, 'Normalization', 'probability');
xline(pixel__mm, '--', 'one pixel at P = 1000', 'LabelOrientation', 'horizontal');
set(gca, 'XScale', 'log'); grid on;
xlabel('narrowest width [mm]'); ylabel('fraction of triangles');
legend('imported', 'remeshed'); title('size');

%% 4. The same in the interactive viewer
% Each window blocks until closed. T toggles the triangle edges; they fade
% out as you zoom out, so zoom in on a wing.
vat.visualization.show_meshes(original);
vat.visualization.show_meshes(remeshed);

%% Helpers
function print_quality(label, q)
    fprintf('%-9s %9d %14.2f %14.2f %16.3f\n', label, q.num_triangles, ...
        q.aspect_ratio.median, q.aspect_ratio.p95, 1e3 * q.min_altitude__m.p05);
end

function [aspect, width] = triangle_metrics(vertices)
    % Per triangle: aspect ratio (longest edge / (2 sqrt(3) inradius)) and
    % narrowest width (smallest altitude). vertices holds x/y/z of three
    % corners per triangle, as get_vertices returns them.
    V = reshape(double(vertices), 3, 3, []);   % xyz x corner x triangle
    a = squeeze(vecnorm(V(:, 2, :) - V(:, 3, :)));
    b = squeeze(vecnorm(V(:, 3, :) - V(:, 1, :)));
    c = squeeze(vecnorm(V(:, 1, :) - V(:, 2, :)));
    area = 0.5 * vecnorm(cross(squeeze(V(:, 2, :) - V(:, 1, :)), squeeze(V(:, 3, :) - V(:, 1, :))))';
    longest = max([a b c], [], 2);
    width = 2 * area ./ longest;
    aspect = longest ./ (2 * sqrt(3) * area ./ (0.5 * (a + b + c)));
end

function plot_mesh(ax, geometry, face_values)
    % Draws every triangle with its edges, coloured per face.
    V = reshape(double(geometry.get_vertices()), 3, [])';
    F = reshape(1:size(V, 1), 3, [])';
    patch(ax, 'Faces', F, 'Vertices', V, 'FaceVertexCData', face_values(:), ...
        'FaceColor', 'flat', 'EdgeColor', [0.15 0.15 0.15], 'LineWidth', 0.3);
    axis(ax, 'equal'); axis(ax, 'off');
    colormap(ax, turbo);
end
