%% remeshing: a satellite with rotating wings
% Remeshing in a real workflow: remesh once, keep the file, and sweep the
% wing angle on the result -- the same study as rotate_wings.m.
%
% Remeshing keeps every mesh under its mesh_id, so hinge definitions made
% for the imported file apply to the remeshed one unchanged.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

%% 1. Remesh once, then load the saved file
% Remeshing takes seconds, loading milliseconds. Here the file goes to
% tempdir; in your own work, keep it next to the original.
obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_61440.obj');
remeshed_file = fullfile(tempdir, 'shuttlecock_remeshed_20k.obj');

original = vat.geometry.RotatableMeshGeometry(obj_file);
if ~isfile(remeshed_file)
    [remeshed, report] = original.remesh(TriangleCount=20000);
    remeshed.export_obj(remeshed_file);
    fprintf('remeshed %d -> %d triangles, saved to %s\n', ...
        report.before.num_triangles, report.after.num_triangles, remeshed_file);
end
remeshed = vat.geometry.RotatableMeshGeometry(remeshed_file);
fprintf('imported %d triangles, remeshed %d\n', original.get_num_triangles(), remeshed.get_num_triangles());

%% 2. The hinges of rotate_wings.m
% Mesh ids 1 to 4 are the four wings, in both files.
h(1).mesh_id = 1; h(1).origin = [-0.15  0.10  0.05]; h(1).axis = [0  1  0]; % bottom
h(2).mesh_id = 2; h(2).origin = [-0.15 -0.05  0.10]; h(2).axis = [0  0  1]; % left
h(3).mesh_id = 3; h(3).origin = [-0.15  0.05 -0.10]; h(3).axis = [0  0 -1]; % right
h(4).mesh_id = 4; h(4).origin = [-0.15 -0.10 -0.05]; h(4).axis = [0 -1  0]; % up

% Blocks until the window is closed.
vat.visualization.show_hinges(remeshed, h);

%% 3. Both geometries with the wings at 45 degrees
turn_wings(original, h, deg2rad(45));
turn_wings(remeshed, h, deg2rad(45));

figure('Name', 'Wings at 45 degrees, before and after remeshing', 'Position', [100 100 1300 520]);
tiledlayout(1, 2, 'TileSpacing', 'compact');
geometries = {original, remeshed};
names = {'imported', 'remeshed'};
for k = 1:2
    ax = nexttile;
    V = reshape(double(geometries{k}.get_vertices()), 3, [])';
    patch(ax, 'Faces', reshape(1:size(V, 1), 3, [])', 'Vertices', V, ...
        'FaceColor', [0.30 0.55 0.85], 'EdgeColor', [0.1 0.1 0.1], 'EdgeAlpha', 0.4, 'LineWidth', 0.2);
    axis(ax, 'equal'); axis(ax, 'off'); view(ax, [-1 0.8 0.6]);
    xlim(ax, [-0.40 -0.10]);
    title(ax, sprintf('%s, %d triangles, wings at 45 deg', names{k}, geometries{k}.get_num_triangles()));
end

% Blocks until the window is closed.
vat.visualization.show_meshes(remeshed);

%% 4. Build the pipeline in the most extended pose
% num_pixel is fixed when the pipeline is built, but the frustum follows
% the current bounding radius: a pose reaching further out spreads the same
% pixels wider. So build it where the radius is largest over the sweep.
alpha = deg2rad(-5:1:90);
radius = zeros(size(alpha));
for i = 1:numel(alpha)
    turn_wings(remeshed, h, alpha(i));
    q = remeshed.get_mesh_quality();
    radius(i) = q.bounding_sphere_radius__m;
end
[~, widest] = max(radius);
fprintf('bounding radius %.3f to %.3f m over the sweep, largest at %.0f deg\n', ...
    min(radius), max(radius), rad2deg(alpha(widest)));

turn_wings(remeshed, h, alpha(widest));
pipeline = vat.shading.ShadingPipeline(remeshed, 1);       % CoP, num_pixel chosen
fprintf('remeshed: CoP, num_pixel chosen %d\n', pipeline.get_num_pixel());

% As rotate_wings.m does it: the imported mesh, Binary, num_pixel 4000.
turn_wings(original, h, alpha(widest));
pipeline_original = vat.shading.ShadingPipeline(original, 0, 4000);

%% 5. Drag over wing angle
conditions = vat.AeroConditions(1e-9, 934, 16 * 1.6605390689252e-27);
gsi = vat.gsi_models.Sentman(1, 0.95);
v_rel__m_per_s = 7800 * [cosd(5), sind(5), 0];

sweeps = {original, pipeline_original, 'imported, Binary, num\_pixel 4000';
          remeshed, pipeline, sprintf('remeshed, CoP, num\\_pixel %d', pipeline.get_num_pixel())};
drag__mN = zeros(2, numel(alpha));
seconds = zeros(1, 2);
for s = 1:2
    calculator = vat.loads.HybridForceTorqueCalculator(sweeps{s, 1}, sweeps{s, 2}, gsi);
    tic;
    for i = 1:numel(alpha)
        turn_wings(sweeps{s, 1}, h, alpha(i));
        [F, ~] = calculator.calc_aero_load(v_rel__m_per_s, 300, conditions);
        drag__mN(s, i) = -1e3 * F(1);
    end
    seconds(s) = toc;
    fprintf('%-38s %.2f s for %d angles\n', strrep(sweeps{s, 3}, '\', ''), seconds(s), numel(alpha));
end

figure('Name', 'Drag over wing angle', 'Position', [120 120 900 450]);
plot(rad2deg(alpha), drag__mN(1, :), 'LineWidth', 1.5); hold on;
plot(rad2deg(alpha), drag__mN(2, :), 'LineWidth', 1.5);
grid on; xlabel('wing angle [deg]'); ylabel('drag [mN]');
legend(sprintf('%s: %.1f s', sweeps{1, 3}, seconds(1)), ...
       sprintf('%s: %.1f s', sweeps{2, 3}, seconds(2)), 'Location', 'northwest');
title(sprintf('Drag over wing angle, %d angles', numel(alpha)));

% The curves agree: drag comes mostly from the large faces, which the import
% already resolves well at 4000 pixels. The remeshed geometry gets the same
% answer in well under half the time, and with CoP rather than Binary, which
% matters where shadow edges cross the parts -- see remesh_shading.m.

%% Helpers
function turn_wings(geometry, hinges, angle__rad)
    % turn_mesh_around_axis is absolute, not relative: it sets the angle.
    for i = 1:numel(hinges)
        geometry.turn_mesh_around_axis(hinges(i).mesh_id, angle__rad, hinges(i).origin, hinges(i).axis);
    end
end
