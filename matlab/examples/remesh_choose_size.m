%% remeshing: choosing the size
% How many triangles to ask for, and what each choice costs.
%
% remesh takes the size either as TriangleCount or as EdgeLength [m] -- one
% number for the whole geometry. predict_remesh answers "what would I get?"
% without remeshing, so sizes can be tried cheaply. Every load evaluation
% then pays for every triangle, so ask for what the problem needs.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

geometry_folder = fullfile(fileparts(mfilename('fullpath')), 'geometries');
geometry = vat.geometry.RotatableMeshGeometry(fullfile(geometry_folder, 'shuttlecock_15k.obj'));

%% 1. Predicted and actual, over a range of sizes
% The prediction assumes a perfect tiling with equilateral triangles. The
% real count lands somewhat above it: sharp edges and panel rims are kept
% exactly, and they resist coarsening -- the more so, the coarser the target
% relative to the parts.
targets = [2000 5000 10000 20000 50000];
n = numel(targets);
[predicted, actual, edge__mm, memory__MB, seconds, num_pixel_cop] = deal(zeros(1, n));
meshes = cell(1, n);

fprintf('%8s %10s %10s %9s %10s %9s %13s\n', 'target', 'predicted', 'actual', 'edge mm', 'memory MB', 'time s', 'num_pixel CoP');
for i = 1:n
    p = geometry.predict_remesh(TriangleCount=targets(i));
    tic; [meshes{i}, report] = geometry.remesh(TriangleCount=targets(i)); seconds(i) = toc;
    predicted(i) = p.predicted_triangles;
    actual(i) = report.after.num_triangles;
    edge__mm(i) = 1e3 * p.target_edge_length__m;
    memory__MB(i) = p.predicted_memory__MB;
    num_pixel_cop(i) = report.suggested_num_pixel.cop;
    fprintf('%8d %10d %10d %9.2f %10.1f %9.2f %13d\n', targets(i), predicted(i), actual(i), ...
        edge__mm(i), memory__MB(i), seconds(i), num_pixel_cop(i));
end

figure('Name', 'Choosing the size', 'Position', [100 100 1300 420]);
tiledlayout(1, 3, 'TileSpacing', 'compact');
nexttile;
loglog(targets, actual, 'o-', 'LineWidth', 1.5); hold on;
loglog(targets, targets, 'k--');
grid on; xlabel('TriangleCount asked for'); ylabel('triangles produced');
legend('remeshed', 'as asked', 'Location', 'northwest'); title('count');
nexttile;
semilogx(actual, seconds, 'o-', 'LineWidth', 1.5);
grid on; xlabel('triangles'); ylabel('remeshing time [s]'); title('one-off cost');
% num_pixel follows the narrowest triangles. At coarse sizes those are on the
% wings' 3 mm rims -- a triangle cannot be wider than the face it sits on --
% so num_pixel stops falling there.
nexttile;
semilogx(actual, num_pixel_cop, 'o-', 'LineWidth', 1.5);
grid on; xlabel('triangles'); ylabel('num\_pixel chosen for CoP'); title('matching raster');

%% 2. Three of them side by side
% The same wing face as in remesh_before_after.m, coarse to fine.
figure('Name', 'Three sizes', 'Position', [120 120 1400 330]);
layout = tiledlayout(1, 3, 'TileSpacing', 'compact');
show = [1 3 5];
for k = 1:3
    ax = nexttile;
    V = reshape(double(meshes{show(k)}.get_vertices()), 3, [])';
    patch(ax, 'Faces', reshape(1:size(V, 1), 3, [])', 'Vertices', V, ...
        'FaceColor', [0.30 0.55 0.85], 'EdgeColor', [0.1 0.1 0.1], 'LineWidth', 0.3);
    axis(ax, 'equal'); axis(ax, 'off'); view(ax, [0 -1 0]);
    xlim(ax, [-0.36 -0.10]); zlim(ax, [-0.06 0.06]);
    title(ax, sprintf('%d triangles, edge %.1f mm', actual(show(k)), edge__mm(show(k))));
end

%% 3. Size as an edge length
% The same edge length gives very different counts on different models:
% the count follows the surface area.
soar = vat.geometry.RotatableMeshGeometry(fullfile(geometry_folder, 'soar_satellite.obj'));
for g = {{'shuttlecock', geometry}, {'soar_satellite', soar}}
    p = g{1}{2}.predict_remesh(EdgeLength=5e-3);
    fprintf('%-15s EdgeLength = 5 mm -> about %6d triangles (surface %.3f m^2)\n', ...
        g{1}{1}, p.predicted_triangles, p.total_area__m2);
end

%% 4. The limits
% Above 250k triangles remesh warns; above 1M it refuses, before doing any
% work. predict_remesh reports such a size instead of refusing it.
p = geometry.predict_remesh(EdgeLength=0.2e-3);
fprintf('\nEdgeLength = 0.2 mm -> about %d triangles, %.0f MB; exceeds the limit: %d\n', ...
    p.predicted_triangles, p.predicted_memory__MB, p.exceeds_limit);
try
    geometry.remesh(EdgeLength=0.2e-3);
catch err
    fprintf('remesh refused: %s\n', err.message);
end

%% 5. Look at the coarsest one interactively
% Blocks until the window is closed.
vat.visualization.show_meshes(meshes{1});
