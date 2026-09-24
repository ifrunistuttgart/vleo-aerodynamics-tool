%% Per-pixel force and torque
% Computes force and torque with the per-pixel calculator and shows where
% on the geometry the load comes from: the pressure image, seen from
% upstream along the flow.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

%% Atmosphere and gas-surface interaction model
T_env   = 934;                         % atmospheric temperature [K]
rho     = 1e-9;                        % density [kg/m^3]
ao_mass = 16 * 1.6605390689252e-27;    % atomic oxygen [kg]
alpha_e = 0.95;                        % energy accommodation coefficient
T_wall  = 300;                         % surface temperature [K]

aero_cond = vat.AeroConditions(rho, T_env, ao_mass);
gsi_model = vat.gsi_models.Sentman(1, alpha_e);

%% Geometry
obj_file = fullfile(fileparts(mfilename('fullpath')), ...
    'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

%% Per-pixel calculator
% num_pixel is the resolution of the render along the flow. No shading
% pipeline is needed: the calculator renders the geometry itself.
% KeepPressureImage makes it keep the pressure of every pixel for plotting.
num_pixel  = 2000;
calculator = vat.loads.PixelForceTorqueCalculator( ...
    geometry, gsi_model, num_pixel, KeepPressureImage=true);

%% Force and torque at 4 deg angle of attack
aoa   = deg2rad(4);
v_rel = 7800 * [cos(aoa), sin(aoa), 0];   % body frame, [x y z]

[F, T] = calculator.calc_aero_load(v_rel, T_wall, aero_cond);
[wetted, projected] = calculator.last_areas();

fprintf('Force  [N]    : %+.4e %+.4e %+.4e\n', F);
fprintf('Torque [Nm]   : %+.4e %+.4e %+.4e\n', T);
fprintf('Wetted area   : %.4f m^2\n', wetted);
fprintf('Frontal area  : %.4f m^2\n', projected);

%% Pressure image
% The image spans the bounding sphere, [-R, R] in both directions, seen
% from upstream. Pixels without a surface facing the flow are zero; they
% are left transparent here.
% pressure_image returns the top row first; flip it so that row 1 is the
% bottom and the y axis can point up.
p = flipud(calculator.pressure_image());
R = bounding_radius(geometry);
x = linspace(-R, R, num_pixel);

% The face the flow hits head-on dominates; surfaces seen almost edge-on
% carry a small, mostly thermal pressure. A log scale shows both.
figure('Name', 'Pressure image');
imagesc(x, x, p, 'AlphaData', p > 0);
set(gca, 'YDir', 'normal', 'Color', [0.95 0.95 0.95], 'ColorScale', 'log');
clim([max(p(:)) / 1e3, max(p(:))]);
axis image;
cb = colorbar;
cb.Label.String = 'pressure [N/m^2]';
xlabel('across the flow [m]');
ylabel('across the flow [m]');
title(sprintf('Sentman, %d x %d pixels, AoA %.0f deg', num_pixel, num_pixel, rad2deg(aoa)));
zoom_to_surface(x, p);

%% Local functions
function R = bounding_radius(geometry)
    % Largest distance of any vertex from the body origin: the radius the
    % calculator's render spans.
    v = reshape(geometry.get_vertices(), 3, []);
    R = double(max(vecnorm(v)));
end

function zoom_to_surface(x, p)
    % Limit the axes to the pixels that carry a load, plus a small margin.
    cols = find(any(p > 0, 1));
    rows = find(any(p > 0, 2));
    if isempty(cols), return; end
    margin = 0.05 * (x(end) - x(1));
    xlim([x(cols(1)) - margin, x(cols(end)) + margin]);
    ylim([x(rows(1)) - margin, x(rows(end)) + margin]);
end
