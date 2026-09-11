%% Attitude dependence of the achievable torque set
% Precomputes the achievable-torque surface over a grid of attitudes, then
% opens an interactive explorer: two sliders pick the attitude and the surface
% redraws instantly inside a fixed grey envelope of everything reachable at
% any attitude. Companion to torque_surface.m, which covers a single attitude
% and documents the eta sign convention.
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
cfg.alpha__deg   = linspace(-30, 30, 7);   % angle of attack samples
cfg.beta__deg    = linspace(-30, 30, 7);   % sideslip samples
cfg.eta_max__deg = 90;                     % panel travel
cfg.n_eta        = 15;                     % odd, so eta = 0 is sampled exactly
cfg.num_pixel    = 1000;                   % shading resolution
cfg.obj          = 'shuttlecock_15k.obj';

cfg.v__m_per_s        = 7800;
cfg.surface_temp__K   = 300.0;
cfg.rho__kg_per_m3    = 1.2482e-11;
cfg.T_atmospheric__K  = 934.0;
cfg.particle_mass__kg = 16 * 1.6605390689252e-27;

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

%% Explorer
T__uNm = T * 1e6;
X = T__uNm(:,:,:,:,1);
Y = T__uNm(:,:,:,:,2);
Z = T__uNm(:,:,:,:,3);

lim  = [min(X(:)) max(X(:)); min(Y(:)) max(Y(:)); min(Z(:)) max(Z(:))];
lim  = lim + [-1 1] .* (0.08 * diff(lim, 1, 2));
wall = [lim(1,1) lim(2,2) lim(3,1)];   % which wall each projection lands on

fig = figure('Name', 'Achievable torque vs attitude', 'Color', 'w', ...
    'Position', [100 100 900 780]);
ax = axes('Parent', fig, 'Position', [0.10 0.20 0.72 0.72]);
hold(ax, 'on'); grid(ax, 'on'); view(ax, 3); daspect(ax, [1 1 1]);
xlim(ax, lim(1,:)); ylim(ax, lim(2,:)); zlim(ax, lim(3,:));
xlabel(ax, 'T_x [\muNm]'); ylabel(ax, 'T_y [\muNm]'); zlabel(ax, 'T_z [\muNm]');

% Envelope: convex hull of every torque reachable at any sampled attitude.
h_env = gobjects(1);
try
    k_hull = convhull(X(:), Y(:), Z(:), 'Simplify', true);
    h_env  = trisurf(k_hull, X(:), Y(:), Z(:), 'Parent', ax, ...
        'FaceColor', [.4 .4 .4], 'FaceAlpha', 0.07, 'EdgeColor', 'none');
catch err
    warning('Envelope not drawn: %s', err.message);
end

% Projections onto three walls, so the folded sheet stays readable end-on.
h_proj = gobjects(1, 3);
for k = 1:3
    h_proj(k) = surf(ax, nan(cfg.n_eta), nan(cfg.n_eta), nan(cfg.n_eta), ...
        'FaceColor', [.65 .65 .65], 'EdgeColor', 'none', 'FaceAlpha', 0.4);
end

% Current attitude, coloured by eta1.
eta__deg = linspace(-cfg.eta_max__deg, cfg.eta_max__deg, cfg.n_eta);
h_surf = surf(ax, nan(cfg.n_eta), nan(cfg.n_eta), nan(cfg.n_eta), ...
    eta__deg(:) * ones(1, cfg.n_eta), ...
    'EdgeColor', [.3 .3 .3], 'EdgeAlpha', 0.2, 'FaceAlpha', 0.95);
h_crease = [plot3(ax, nan, nan, nan, 'k-', 'LineWidth', 2), ...
            plot3(ax, nan, nan, nan, 'k-', 'LineWidth', 2)];
h_stowed = plot3(ax, nan, nan, nan, 'rp', 'MarkerSize', 15, 'MarkerFaceColor', 'r');

colormap(ax, parula);
cb = colorbar(ax);
cb.Label.String = '\eta_1 [deg]';
legend([h_env h_surf h_crease(1) h_stowed], ...
    {'reachable at any attitude', 'this attitude', ...
     'crease, one panel swaps', 'all panels stowed'}, ...
    'Location', 'northeast', 'AutoUpdate', 'off');

% Sliders. Value is the index into the attitude grid, so it steps discretely.
na = numel(cfg.alpha__deg);
nb = numel(cfg.beta__deg);
s_alpha = uicontrol(fig, 'Style', 'slider', 'Units', 'normalized', ...
    'Position', [0.18 0.095 0.60 0.035], ...
    'Min', 1, 'Max', na, 'Value', ceil(na/2), 'SliderStep', [1 1]/(na-1));
s_beta = uicontrol(fig, 'Style', 'slider', 'Units', 'normalized', ...
    'Position', [0.18 0.045 0.60 0.035], ...
    'Min', 1, 'Max', nb, 'Value', ceil(nb/2), 'SliderStep', [1 1]/(nb-1));
uicontrol(fig, 'Style', 'text', 'Units', 'normalized', 'String', 'alpha', ...
    'Position', [0.07 0.09 0.10 0.03], 'BackgroundColor', 'w');
uicontrol(fig, 'Style', 'text', 'Units', 'normalized', 'String', 'beta', ...
    'Position', [0.07 0.04 0.10 0.03], 'BackgroundColor', 'w');

% The anonymous body runs on every call, so s_alpha.Value is read live.
redraw = @(varargin) update_view(ax, h_surf, h_proj, h_crease, h_stowed, ...
    X, Y, Z, round(s_alpha.Value), round(s_beta.Value), cfg, wall);
s_alpha.Callback = redraw;
s_beta.Callback  = redraw;
redraw();

%% Helpers
function T = sweep(cfg, here)
    geometry = vat.geometry.RotatableMeshGeometry( ...
        fullfile(here, 'geometries', cfg.obj));
    conditions = vat.AeroConditions(cfg.rho__kg_per_m3, ...
        cfg.T_atmospheric__K, cfg.particle_mass__kg);
    pipeline   = vat.shading.ShadingPipeline(geometry, 1, cfg.num_pixel);
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

function update_view(ax, h_surf, h_proj, h_crease, h_stowed, X, Y, Z, ia, ib, cfg, wall)
    x = X(:,:,ia,ib);
    y = Y(:,:,ia,ib);
    z = Z(:,:,ia,ib);
    set(h_surf, 'XData', x, 'YData', y, 'ZData', z);

    set(h_proj(1), 'XData', x,           'YData', y,           'ZData', wall(3)+0*z);
    set(h_proj(2), 'XData', x,           'YData', wall(2)+0*y, 'ZData', z);
    set(h_proj(3), 'XData', wall(1)+0*x, 'YData', y,           'ZData', z);

    k = (cfg.n_eta + 1) / 2;   % index of eta = 0
    set(h_crease(1), 'XData', x(k,:), 'YData', y(k,:), 'ZData', z(k,:));
    set(h_crease(2), 'XData', x(:,k), 'YData', y(:,k), 'ZData', z(:,k));
    set(h_stowed, 'XData', x(k,k), 'YData', y(k,k), 'ZData', z(k,k));

    title(ax, ['\alpha = ' num2str(cfg.alpha__deg(ia)) ' deg,   \beta = ' ...
        num2str(cfg.beta__deg(ib)) ' deg']);
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
