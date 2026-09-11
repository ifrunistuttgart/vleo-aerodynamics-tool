%% Attitude dependence of the achievable torque set
% Precomputes the achievable-torque surface over a grid of attitudes, then
% opens an interactive explorer with two linked views:
%
%   left   the torque surface for the current attitude, inside a fixed grey
%          envelope of everything reachable at any attitude
%   right  the satellite itself, in a flow-fixed ("wind tunnel") view: the
%          oncoming stream always arrives from +x, drawn as fixed blue arrows,
%          and the satellite tilts by alpha/beta against it. Faces are run
%          through the real shading pipeline and coloured in three states:
%          orange = windward and exposed (carries load), dark blue = windward
%          but occluded by another part of the satellite (carries none),
%          grey = leeward.
%
% Dragging either slider updates both views continuously. Hovering over the
% torque surface deflects the satellite's panels to the exact eta1/eta2 of the
% point under the cursor and reports its torque; the last hovered
% configuration stays on screen when the cursor leaves, so the view can be
% rotated without losing it.
%
% Companion to torque_surface.m, which covers a single attitude and documents
% the eta sign convention.
%
% The eta -> torque map is deliberately not smooth. At eta1 = 0 the panel that
% deploys swaps from the up wing to the bottom wing, so the surface is creased
% along both centre lines and is really four patches (up/bottom x left/right)
% glued together. Those two creases are drawn as black lines, and the stowed
% configuration where they meet as a red star.
%
% The sweep is cached to a .mat next to this script and reused whenever the
% parameters below are unchanged. Delete that file to force a recompute.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

%% Parameters
% Grid sizes are chosen together: 31 x 31 x 9 x 9 is 77 841 load evaluations,
% a few minutes. Both attitude counts are odd so alpha = beta = 0 is sampled,
% and both step in round numbers (6 deg in eta, 7.5 deg in attitude).
cfg.alpha__deg   = linspace(-30, 30, 9);   % angle of attack samples
cfg.beta__deg    = linspace(-30, 30, 9);   % sideslip samples
cfg.eta_max__deg = 90;                     % panel travel
cfg.n_eta        = 31;                     % odd, so eta = 0 is sampled exactly
cfg.num_pixel    = 1000;                   % shading resolution
cfg.obj          = 'shuttlecock_15k.obj';

% 0 = Binary, 1 = CoP. Shared by the sweep and the live view on purpose, so
% the picture on the right can never disagree with the numbers on the left.
cfg.shading_algorithm = 0;

cfg.v__m_per_s        = 7800;
cfg.surface_temp__K   = 300.0;
cfg.rho__kg_per_m3    = 1.2482e-11;
cfg.T_atmospheric__K  = 934.0;
cfg.particle_mass__kg = 16 * 1.6605390689252e-27;

% Separate low-poly mesh for the live view. Every shuttlecock_*.obj shares the
% same five objects and the same bounding box, so the hinge ids and origins
% below apply unchanged, and 240 triangles redraw on every mouse move.
cfg.view_obj = 'shuttlecock_240.obj';

assert(mod(cfg.n_eta, 2) == 1, 'n_eta must be odd so the creases at eta = 0 are sampled.');

here       = fileparts(mfilename('fullpath'));
cache_file = fullfile(here, 'torque_surface_attitude.mat');

%% Compute or load
% T is (eta1, eta2, alpha, beta, xyz) in Nm.
cached = struct();
if isfile(cache_file)
    cached = load(cache_file);
end

if isfield(cached, 'cfg') && isequal(cached.cfg, cfg)
    T = cached.T;
    fprintf('Loaded cached sweep from %s\n', cache_file);
else
    T = sweep(cfg, here);
    save(cache_file, 'T', 'cfg');
    fprintf('Cached sweep to %s\n', cache_file);
end

%% Figure
S       = struct();
S.cfg   = cfg;
S.T__uNm = T * 1e6;
S.X = S.T__uNm(:,:,:,:,1);
S.Y = S.T__uNm(:,:,:,:,2);
S.Z = S.T__uNm(:,:,:,:,3);
S.eta__deg = linspace(-cfg.eta_max__deg, cfg.eta_max__deg, cfg.n_eta);
S.k_zero   = (cfg.n_eta + 1) / 2;          % index of eta = 0

lim   = [min(S.X(:)) max(S.X(:)); min(S.Y(:)) max(S.Y(:)); min(S.Z(:)) max(S.Z(:))];
lim   = lim + [-1 1] .* (0.08 * diff(lim, 1, 2));
S.lim  = lim;
S.wall = [lim(1,1) lim(2,2) lim(3,1)];     % wall each projection lands on
S.tol  = 0.06 * max(diff(lim, 1, 2));      % hover pick radius, data units

% Colours are left to the MATLAB theme, so the window is readable in both the
% light and the dark desktop theme.
fig = figure('Name', 'Achievable torque vs attitude', ...
    'Position', [80 80 1400 820]);
S.fig = fig;

% Left: torque surface
ax = axes('Parent', fig, 'Position', [0.05 0.20 0.44 0.72]);
hold(ax, 'on'); grid(ax, 'on'); view(ax, 3); daspect(ax, [1 1 1]);
xlim(ax, lim(1,:)); ylim(ax, lim(2,:)); zlim(ax, lim(3,:));
xlabel(ax, 'T_x [\muNm]'); ylabel(ax, 'T_y [\muNm]'); zlabel(ax, 'T_z [\muNm]');
S.ax = ax;

h_env = gobjects(1);
try
    k_hull = convhull(S.X(:), S.Y(:), S.Z(:), 'Simplify', true);
    h_env  = trisurf(k_hull, S.X(:), S.Y(:), S.Z(:), 'Parent', ax, ...
        'FaceColor', [.5 .5 .5], 'FaceAlpha', 0.10, 'EdgeColor', 'none', ...
        'PickableParts', 'none');
catch err
    warning('Envelope not drawn: %s', err.message);
end

S.h_proj = gobjects(1, 3);
for k = 1:3
    S.h_proj(k) = surf(ax, nan(cfg.n_eta), nan(cfg.n_eta), nan(cfg.n_eta), ...
        'FaceColor', [.65 .65 .65], 'EdgeColor', 'none', 'FaceAlpha', 0.4, ...
        'PickableParts', 'none');
end

S.h_surf = surf(ax, nan(cfg.n_eta), nan(cfg.n_eta), nan(cfg.n_eta), ...
    S.eta__deg(:) * ones(1, cfg.n_eta), ...
    'EdgeColor', [.3 .3 .3], 'EdgeAlpha', 0.2, 'FaceAlpha', 0.95);
S.h_crease = [plot3(ax, nan, nan, nan, 'k-', 'LineWidth', 2), ...
              plot3(ax, nan, nan, nan, 'k-', 'LineWidth', 2)];
S.h_stowed = plot3(ax, nan, nan, nan, 'rp', 'MarkerSize', 15, 'MarkerFaceColor', 'r');
S.h_pick   = plot3(ax, nan, nan, nan, 'o', 'MarkerSize', 11, 'LineWidth', 2, ...
    'MarkerEdgeColor', [0 0 0], 'MarkerFaceColor', [1 1 0]);

colormap(ax, parula);
cb = colorbar(ax, 'Position', [0.505 0.20 0.011 0.72]);
cb.Label.String = '\eta_1 [deg]';
legend([h_env S.h_surf S.h_crease(1) S.h_stowed S.h_pick], ...
    {'reachable at any attitude', 'this attitude', ...
     'crease, one panel swaps', 'all panels stowed', 'hovered point'}, ...
    'Location', 'northeast', 'AutoUpdate', 'off');

% Right: satellite in the flow-fixed view
ax_sat = axes('Parent', fig, 'Position', [0.57 0.30 0.40 0.62]);
hold(ax_sat, 'on'); daspect(ax_sat, [1 1 1]); axis(ax_sat, 'off');
S.ax_sat = ax_sat;

S.view_geometry = vat.geometry.RotatableMeshGeometry( ...
    fullfile(here, 'geometries', cfg.view_obj));
% The live view runs the real shading pipeline rather than colouring by
% dot(normal, v_rel) alone. Without it a panel sitting in the body's shadow
% still reads as loaded, because its normal does face the flow -- occlusion is
% the whole thing the per-triangle test cannot see.
S.view_pipeline = vat.shading.ShadingPipeline( ...
    S.view_geometry, cfg.shading_algorithm, 1000);
S.wings   = shuttlecock_wings();
n_tri_view = S.view_geometry.get_num_triangles();
S.h_sat = patch('Parent', ax_sat, ...
    'Faces', reshape(1:3*n_tri_view, 3, [])', ...
    'Vertices', nan(3*n_tri_view, 3), ...
    'FaceColor', 'flat', 'FaceVertexCData', zeros(n_tri_view, 3), ...
    'EdgeColor', [.25 .25 .25], 'LineWidth', 0.25);

% Fixed limits so the satellite neither jumps nor rescales as panels deploy.
% The body spans x = -0.353..0.15 and a panel at 90 deg reaches about 0.25 out;
% the upper x bound leaves room for the flow arrows upstream of the nose.
xlim(ax_sat, [-0.40 0.45]); ylim(ax_sat, [-0.30 0.30]); zlim(ax_sat, [-0.30 0.30]);

% Camera sits upwind so the windward faces face the viewer, but well off the
% flow axis: looking straight down +x would foreshorten the flow arrows to
% almost nothing. camup = -z because body z points down (nadir).
camtarget(ax_sat, [0.02 0 0]);
campos(ax_sat, [0.02 0 0] + [0.62 -0.95 -0.45]);
camup(ax_sat, [0 0 -1]);

% Oncoming flow. The view is flow-fixed, so these arrows never move and the
% satellite tilts against them. The stream runs from +x towards -x, which is
% exactly why the windward faces are the ones whose normals point back along
% +x: they are the faces the stream runs into.
z_arrow = [-0.17 0 0.17];
quiver3(ax_sat, repmat(0.42, size(z_arrow)), zeros(size(z_arrow)), z_arrow, ...
    repmat(-0.21, size(z_arrow)), zeros(size(z_arrow)), zeros(size(z_arrow)), ...
    0, 'Color', [0.10 0.55 0.90], 'LineWidth', 2.5, 'MaxHeadSize', 0.5);
text(ax_sat, 0.32, 0, -0.25, 'oncoming flow', 'Color', [0.10 0.55 0.90], ...
    'HorizontalAlignment', 'center', 'FontWeight', 'bold');

S.h_readout = uicontrol(fig, 'Style', 'text', 'Units', 'normalized', ...
    'Position', [0.57 0.15 0.40 0.13], ...
    'HorizontalAlignment', 'left', 'FontName', 'Consolas', 'FontSize', 10);

% Sliders. Value is the index into the attitude grid, so it steps discretely.
na = numel(cfg.alpha__deg);
nb = numel(cfg.beta__deg);
S.s_alpha = uicontrol(fig, 'Style', 'slider', 'Units', 'normalized', ...
    'Position', [0.12 0.095 0.36 0.032], ...
    'Min', 1, 'Max', na, 'Value', ceil(na/2), 'SliderStep', [1 1]/(na-1));
S.s_beta = uicontrol(fig, 'Style', 'slider', 'Units', 'normalized', ...
    'Position', [0.12 0.045 0.36 0.032], ...
    'Min', 1, 'Max', nb, 'Value', ceil(nb/2), 'SliderStep', [1 1]/(nb-1));
uicontrol(fig, 'Style', 'text', 'Units', 'normalized', 'String', 'alpha', ...
    'Position', [0.05 0.09 0.06 0.028]);
uicontrol(fig, 'Style', 'text', 'Units', 'normalized', 'String', 'beta', ...
    'Position', [0.05 0.04 0.06 0.028]);

% Open on a deployed, deliberately asymmetric configuration at the middle
% attitude. Stowed would be the obvious default, but all four panels lie flush
% with the body there, so the opening view would say nothing about the mechanism.
[~, i0] = min(abs(S.eta__deg - 45));
[~, j0] = min(abs(S.eta__deg + 45));
S.hov = [i0 j0];
S.dragging = false;
fig.UserData = S;

% ContinuousValueChange fires throughout the drag; the plain Callback would
% only fire on release.
addlistener(S.s_alpha, 'ContinuousValueChange', @(~,~) on_attitude(fig));
addlistener(S.s_beta,  'ContinuousValueChange', @(~,~) on_attitude(fig));
S.s_alpha.Callback = @(~,~) on_attitude(fig);
S.s_beta.Callback  = @(~,~) on_attitude(fig);

% Suppress hover picking while a drag-rotate is in progress, so the highlight
% does not chase the cursor as the camera swings.
fig.WindowButtonDownFcn   = @(~,~) set_dragging(fig, true);
fig.WindowButtonUpFcn     = @(~,~) set_dragging(fig, false);
fig.WindowButtonMotionFcn = @(~,~) on_hover(fig);

on_attitude(fig);

%% Callbacks
function on_attitude(fig)
    % Slider moved: swap the torque surface to the new attitude slice.
    S  = fig.UserData;
    ia = round(S.s_alpha.Value);
    ib = round(S.s_beta.Value);
    x  = S.X(:,:,ia,ib); y = S.Y(:,:,ia,ib); z = S.Z(:,:,ia,ib);

    set(S.h_surf, 'XData', x, 'YData', y, 'ZData', z);
    set(S.h_proj(1), 'XData', x, 'YData', y, 'ZData', S.wall(3)+0*z);
    set(S.h_proj(2), 'XData', x, 'YData', S.wall(2)+0*y, 'ZData', z);
    set(S.h_proj(3), 'XData', S.wall(1)+0*x, 'YData', y, 'ZData', z);

    k = S.k_zero;
    set(S.h_crease(1), 'XData', x(k,:), 'YData', y(k,:), 'ZData', z(k,:));
    set(S.h_crease(2), 'XData', x(:,k), 'YData', y(:,k), 'ZData', z(:,k));
    set(S.h_stowed, 'XData', x(k,k), 'YData', y(k,k), 'ZData', z(k,k));

    title(S.ax, ['\alpha = ' num2str(S.cfg.alpha__deg(ia)) ' deg,   \beta = ' ...
        num2str(S.cfg.beta__deg(ib)) ' deg']);

    S.att = [ia ib];
    fig.UserData = S;
    refresh_satellite(fig);
end

function on_hover(fig)
    % Cursor moved: pick the nearest surface point along the view ray and
    % deflect the satellite's panels to match. Leaving the surface keeps the
    % last pick, so the view can be rotated without losing it.
    S = fig.UserData;
    if S.dragging || ~in_axes(S.ax)
        return
    end
    ia = S.att(1); ib = S.att(2);
    [i, j, d] = nearest_on_surface(S.ax.CurrentPoint, ...
        S.X(:,:,ia,ib), S.Y(:,:,ia,ib), S.Z(:,:,ia,ib));
    if d > S.tol || isequal([i j], S.hov)
        return
    end
    S.hov = [i j];
    fig.UserData = S;
    refresh_satellite(fig);
end

function refresh_satellite(fig)
    % Redraw the satellite for the current attitude and hovered panel angles,
    % and update the marker and readout to match.
    S  = fig.UserData;
    ia = S.att(1); ib = S.att(2);
    i  = S.hov(1); j  = S.hov(2);

    eta1__rad = deg2rad(S.eta__deg(i));
    eta2__rad = deg2rad(S.eta__deg(j));
    set_panels(S.view_geometry, S.wings, eta1__rad, eta2__rad);

    % get_vertices returns a flat 9*N column of model-transformed vertices,
    % three explicit vertices per triangle (the mesh is de-indexed).
    P = reshape(S.view_geometry.get_vertices(), 3, []);

    % Shade in the body frame, which is the frame the pipeline works in.
    a = deg2rad(S.cfg.alpha__deg(ia));
    b = deg2rad(S.cfg.beta__deg(ib));
    v_rel = S.cfg.v__m_per_s * [cos(a)*cos(b), sin(b), sin(a)*cos(b)];
    visible = S.view_pipeline.shade(v_rel) > 0.5;

    % Flow-fixed view: rotate the body so v_rel lands on +x on screen. v_rel is
    % Ry(-alpha)*Rz(beta)*ex, so its inverse Rz(-beta)*Ry(alpha) does it.
    P = rot_z(-b) * rot_y(a) * P;

    % Windward test. In this frame the flow is exactly +x, so the sign of the
    % normal's x component is dot(normal, v_rel). Rotation preserves that sign,
    % so it is the same test the GSI model applies in the body frame.
    v1 = P(:, 1:3:end); v2 = P(:, 2:3:end); v3 = P(:, 3:3:end);
    n  = cross(v2 - v1, v3 - v1);
    windward = n(1,:)' > 0;

    % Three states, because windward and actually-loaded are not the same
    % thing: a windward face the pipeline reports as shadowed carries no load.
    loaded   = windward &  visible;
    shadowed = windward & ~visible;
    colour = repmat([0.72 0.72 0.74], numel(windward), 1);   % leeward
    colour(loaded, :)   = repmat([0.85 0.33 0.10], sum(loaded), 1);
    colour(shadowed, :) = repmat([0.20 0.31 0.52], sum(shadowed), 1);
    set(S.h_sat, 'Vertices', P', 'FaceVertexCData', colour);

    title(S.ax_sat, sprintf('flow-fixed view   (%d windward: %d loaded, %d shadowed)', ...
        sum(windward), sum(loaded), sum(shadowed)));

    set(S.h_pick, 'XData', S.X(i,j,ia,ib), 'YData', S.Y(i,j,ia,ib), ...
        'ZData', S.Z(i,j,ia,ib));
    t = [S.X(i,j,ia,ib) S.Y(i,j,ia,ib) S.Z(i,j,ia,ib)];
    S.h_readout.String = { ...
        sprintf('eta1 %+7.1f deg   (up/bottom)', S.eta__deg(i)), ...
        sprintf('eta2 %+7.1f deg   (left/right)', S.eta__deg(j)), ...
        sprintf('T  = [%+7.3f %+7.3f %+7.3f] uNm', t), ...
        sprintf('|T| =  %7.3f uNm', norm(t))};
end

function set_dragging(fig, tf)
    S = fig.UserData;
    S.dragging = tf;
    fig.UserData = S;
end

%% Helpers
function [i, j, d] = nearest_on_surface(ray, x, y, z)
    % Nearest grid point to the cursor's view ray, and its perpendicular
    % distance in data units. ray is the 2x3 axes CurrentPoint. Kept free of
    % graphics state so it can be tested directly.
    p0  = ray(1,:)';
    dir = ray(2,:)' - p0;
    dir = dir / norm(dir);
    P   = [x(:) y(:) z(:)]' - p0;
    perp = P - dir * (dir' * P);
    [d, idx] = min(vecnorm(perp, 2, 1));
    [i, j] = ind2sub(size(x), idx);
end

function tf = in_axes(ax)
    p  = ax.Parent.CurrentPoint;                      % pixels, figure frame
    sz = ax.Parent.Position(3:4);
    r  = ax.Position .* [sz sz];
    tf = p(1) >= r(1) && p(1) <= r(1)+r(3) && p(2) >= r(2) && p(2) <= r(2)+r(4);
end

function R = rot_y(t)
    R = [cos(t) 0 sin(t); 0 1 0; -sin(t) 0 cos(t)];
end

function R = rot_z(t)
    R = [cos(t) -sin(t) 0; sin(t) cos(t) 0; 0 0 1];
end

function T = sweep(cfg, here)
    geometry = vat.geometry.RotatableMeshGeometry( ...
        fullfile(here, 'geometries', cfg.obj));
    conditions = vat.AeroConditions(cfg.rho__kg_per_m3, ...
        cfg.T_atmospheric__K, cfg.particle_mass__kg);
    pipeline   = vat.shading.ShadingPipeline(geometry, cfg.shading_algorithm, cfg.num_pixel);
    calculator = vat.loads.HybridForceTorqueCalculator( ...
        geometry, pipeline, vat.gsi_models.Sentman(1, 0.9));

    wings = shuttlecock_wings();

    % Flow direction per attitude, standard aerodynamic definition.
    na = numel(cfg.alpha__deg);
    nb = numel(cfg.beta__deg);
    v_rel = nan(na, nb, 3);
    for ia = 1:na
        for ib = 1:nb
            a = deg2rad(cfg.alpha__deg(ia));
            b = deg2rad(cfg.beta__deg(ib));
            v_rel(ia,ib,:) = cfg.v__m_per_s * ...
                [cos(a)*cos(b), sin(b), sin(a)*cos(b)];
        end
    end

    % Panel angles are the outer loop on purpose: turning a mesh invalidates
    % the geometry's transform cache, so setting the panels once and then
    % sweeping every attitude keeps that cost out of the inner loop.
    eta__rad = deg2rad(linspace(-cfg.eta_max__deg, cfg.eta_max__deg, cfg.n_eta));
    T  = nan(cfg.n_eta, cfg.n_eta, na, nb, 3);
    t0 = tic;
    for i = 1:cfg.n_eta
        for j = 1:cfg.n_eta
            set_panels(geometry, wings, eta__rad(i), eta__rad(j));
            for ia = 1:na
                for ib = 1:nb
                    [~, torque__Nm] = calculator.calc_aero_load( ...
                        squeeze(v_rel(ia,ib,:))', cfg.surface_temp__K, conditions);
                    T(i,j,ia,ib,:) = torque__Nm;
                end
            end
        end
        fprintf('eta1 = %+6.1f deg   (%2d/%2d)   %5.0f s elapsed\n', ...
            rad2deg(eta__rad(i)), i, cfg.n_eta, toc(t0));
    end
end

function wings = shuttlecock_wings()
    % Hinges from rotate_wings.m. Body frame is x forward, y right, z down.
    wings.up     = struct('mesh_id', 4, 'origin', [-0.15 -0.10 -0.05], 'axis', [0 -1  0]);
    wings.bottom = struct('mesh_id', 1, 'origin', [-0.15  0.10  0.05], 'axis', [0  1  0]);
    wings.left   = struct('mesh_id', 2, 'origin', [-0.15 -0.05  0.10], 'axis', [0  0  1]);
    wings.right  = struct('mesh_id', 3, 'origin', [-0.15  0.05 -0.10], 'axis', [0  0 -1]);
end

function set_panels(geometry, wings, eta1__rad, eta2__rad)
    % A positive eta deflects one panel of the pair, a negative eta its
    % opposite. turn_mesh_around_axis is absolute, so every panel is
    % commanded on every call and no state carries over between samples.
    turn(geometry, wings.up,     max( eta1__rad, 0));
    turn(geometry, wings.bottom, max(-eta1__rad, 0));
    turn(geometry, wings.left,   max( eta2__rad, 0));
    turn(geometry, wings.right,  max(-eta2__rad, 0));
end

function turn(geometry, wing, angle__rad)
    geometry.turn_mesh_around_axis( ...
        wing.mesh_id, angle__rad, wing.origin, wing.axis);
end
