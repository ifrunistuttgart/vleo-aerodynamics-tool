%% Pressure images of deflected wings
% Turns the upper and, separately, the lower wing of the shuttlecock out
% into the flow and shows where the flow pushes on the geometry, together
% with the resulting pitch torque. The calculator is built once; turning a
% wing only changes a model matrix, so each evaluation is cheap.
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

% Hinges of the two opposite wings, as in rotate_wings.m. A positive angle
% turns either wing out into the flow.
hinges(1).name    = 'upper wing';
hinges(1).mesh_id = 4;
hinges(1).origin  = [-0.15, -0.1, -0.05];
hinges(1).axis    = [0, -1, 0];

hinges(2).name    = 'lower wing';
hinges(2).mesh_id = 1;
hinges(2).origin  = [-0.15, 0.1, 0.05];
hinges(2).axis    = [0, 1, 0];

calculator = vat.loads.PixelForceTorqueCalculator( ...
    geometry, gsi_model, num_pixel, KeepPressureImage=true);

%% Pitch torque over the deflection angle
% One wing at a time; the other one stays at 0 deg. The two wings push the
% nose in opposite directions, so the curves mirror each other.
angles = 0:5:40;   % [deg]
pitch  = zeros(numel(angles), numel(hinges));
for h = 1:numel(hinges)
    for i = 1:numel(angles)
        deflect(geometry, hinges(h), angles(i));
        [~, T] = calculator.calc_aero_load(v_rel, T_wall, aero_cond);
        pitch(i, h) = T(2);
    end
    deflect(geometry, hinges(h), 0);
end

figure('Name', 'Torque over deflection');
plot(angles, pitch * 1e6, '-o', 'LineWidth', 1.5);
grid on;
xlabel('wing deflection [deg]');
ylabel('pitch torque T_y [\muNm]');
legend({hinges.name}, 'Location', 'best');
title('One wing turned out into the flow along +x');

%% Pressure images: no deflection, upper wing out, lower wing out
% All three share one colour scale, so the images compare directly.
cases  = {'no deflection', 0, 0; ...
          'upper wing 30 deg', 1, 30; ...
          'lower wing 30 deg', 2, 30};
images = cell(size(cases, 1), 1);
for c = 1:size(cases, 1)
    if cases{c, 2} > 0
        deflect(geometry, hinges(cases{c, 2}), cases{c, 3});
    end
    calculator.calc_aero_load(v_rel, T_wall, aero_cond);
    % pressure_image returns the top row first; flip it so y points up.
    images{c} = flipud(calculator.pressure_image());
    if cases{c, 2} > 0
        deflect(geometry, hinges(cases{c, 2}), 0);
    end
end

p_max = max(cellfun(@(p) max(p(:)), images));
figure('Name', 'Pressure images', 'Position', [100 100 1200 400]);
layout = tiledlayout(1, numel(images), 'TileSpacing', 'compact');
for c = 1:numel(images)
    nexttile;
    p = images{c};
    imagesc(p, 'AlphaData', p > 0);
    set(gca, 'YDir', 'normal', 'Color', [0.95 0.95 0.95], 'XTick', [], 'YTick', []);
    axis image;
    clim([0, p_max]);
    title(cases{c, 1});
    zoom_to_surface(images);
end
cb = colorbar;
cb.Layout.Tile = 'east';
cb.Label.String = 'pressure [N/m^2]';
title(layout, 'Seen from upstream, looking along the flow');

%% Local functions
function deflect(geometry, hinge, angle_deg)
    geometry.turn_mesh_around_axis(hinge.mesh_id, deg2rad(angle_deg), hinge.origin, hinge.axis);
end

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
