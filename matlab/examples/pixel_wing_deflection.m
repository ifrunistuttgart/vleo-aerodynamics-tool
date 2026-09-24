%% Pressure images of a deflected wing
% Turns the upper wing of the shuttlecock about its hinge and shows, for a
% few deflections, where the flow pushes on the geometry, together with the
% resulting pitch torque. The calculator is built once; turning a wing
% only changes a model matrix, so each evaluation is cheap.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

aero_cond = vat.AeroConditions(1e-9, 934, 16 * 1.6605390689252e-27);
gsi_model = vat.gsi_models.Sentman(1, 0.95);
v_rel     = [7800, 0, 0];
T_wall    = 300;
num_pixel = 1000;

obj_file = fullfile(fileparts(mfilename('fullpath')), ...
    'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

% Hinge of the upper wing (mesh 4), as in rotate_wings.m
wing.mesh_id = 4;
wing.origin  = [-0.15, -0.1, -0.05];
wing.axis    = [0, -1, 0];

calculator = vat.loads.PixelForceTorqueCalculator( ...
    geometry, gsi_model, num_pixel, KeepPressureImage=true);

%% Torque over the deflection angle
angles = -40:5:40;   % [deg]
torque = zeros(numel(angles), 3);
for i = 1:numel(angles)
    geometry.turn_mesh_around_axis(wing.mesh_id, deg2rad(angles(i)), wing.origin, wing.axis);
    [~, torque(i, :)] = calculator.calc_aero_load(v_rel, T_wall, aero_cond);
end

% Deflected one way, the wing turns into the flow and the torque grows.
% Deflected the other way, the body hides most of it from the flow, so the
% torque hardly changes until the wing tip comes out of the body's shadow.
figure('Name', 'Torque over deflection');
plot(angles, torque * 1e6, '-o', 'LineWidth', 1.5);
grid on;
xlabel('wing deflection [deg]');
ylabel('torque [\muNm]');
legend('T_x', 'T_y', 'T_z', 'Location', 'best');
title('Upper wing deflected, flow along +x');

%% Pressure images for three deflections
% All three share one colour scale, so the images compare directly.
shown  = [-30, 0, 30];   % [deg]
images = cell(size(shown));
for i = 1:numel(shown)
    geometry.turn_mesh_around_axis(wing.mesh_id, deg2rad(shown(i)), wing.origin, wing.axis);
    calculator.calc_aero_load(v_rel, T_wall, aero_cond);
    % pressure_image returns the top row first; flip it so y points up.
    images{i} = flipud(calculator.pressure_image());
end
geometry.turn_mesh_around_axis(wing.mesh_id, 0, wing.origin, wing.axis);

p_max = max(cellfun(@(p) max(p(:)), images));
figure('Name', 'Pressure images', 'Position', [100 100 1200 400]);
layout = tiledlayout(1, numel(shown), 'TileSpacing', 'compact');
for i = 1:numel(shown)
    nexttile;
    p = images{i};
    imagesc(p, 'AlphaData', p > 0);
    set(gca, 'YDir', 'normal', 'Color', [0.95 0.95 0.95], 'XTick', [], 'YTick', []);
    axis image;
    clim([0, p_max]);
    title(sprintf('%+d deg', shown(i)));
    zoom_to_surface(images);
end
cb = colorbar;
cb.Layout.Tile = 'east';
cb.Label.String = 'pressure [N/m^2]';
title(layout, 'Seen from upstream, looking along the flow');

%% Local functions
function zoom_to_surface(images)
    % One common window around every pixel that carries a load in any image.
    covered = false(size(images{1}));
    for k = 1:numel(images)
        covered = covered | images{k} > 0;
    end
    cols = find(any(covered, 1));
    rows = find(any(covered, 2));
    margin = round(0.05 * size(covered, 1));
    xlim([cols(1) - margin, cols(end) + margin]);
    ylim([rows(1) - margin, rows(end) + margin]);
end
