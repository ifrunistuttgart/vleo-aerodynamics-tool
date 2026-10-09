%% Example 6: Deflecting wings
% Uses the hinges from example 2 to turn the shuttlecock's wings and
% computes what that does to drag and pitch torque. Turning a wing only
% changes a model matrix, so the calculator is built once and each angle
% costs one render.
%
% Prerequisites: make sure you have build the matlab binaries with:
% pixi run build-matlab
% And that you have loaded the Matlab project from Vat.prj

clear; close all;
vat.setLogLevel("warn");

conditions      = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
surface_temp__K = 300;
gsi_model       = vat.gsi_models.Sentman(1, 0.95);

obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

% Hinges from example 2; a positive angle turns a wing out into the flow.
hinges(1).mesh_id = 1;  hinges(1).origin = [-0.15,  0.10,  0.05];  hinges(1).axis = [0,  1,  0];  % bottom
hinges(2).mesh_id = 2;  hinges(2).origin = [-0.15, -0.05,  0.10];  hinges(2).axis = [0,  0,  1];  % left
hinges(3).mesh_id = 3;  hinges(3).origin = [-0.15,  0.05, -0.10];  hinges(3).axis = [0,  0, -1];  % right
hinges(4).mesh_id = 4;  hinges(4).origin = [-0.15, -0.10, -0.05];  hinges(4).axis = [0, -1,  0];  % up

pipeline   = vat.shading.ShadingPipeline(geometry, 1, 1000);
calculator = vat.loads.HybridForceTorqueCalculator(geometry, pipeline, gsi_model);

%% 1. All four wings together: drag over the wing angle
v_rel  = [7800, 0, 0];
angles = -5:1:90;   % [deg]
drag   = zeros(size(angles));
for i = 1:numel(angles)
    deflect(geometry, hinges, angles(i) * [1 1 1 1]);
    F = calculator.calc_aero_load(v_rel, surface_temp__K, conditions);
    drag(i) = -F(1);
end
deflect(geometry, hinges, [0 0 0 0]);

figure('Name', 'Drag over wing angle');
plot(angles, 1e6 * drag, 'LineWidth', 1.5);
grid on;
xlabel('angle of all four wings [deg]');
ylabel('drag [\muN]');
title('Flow along +x');

%% 2. One wing at a time: pitch torque
% Upper or lower wing turned out, the other wings at 0 deg. The flow comes
% in at 20 deg angle of attack, from the lower side: the lower wing turns
% into it, while the upper wing stays in the shadow of the body and hardly
% changes the torque. The shading view below shows why.
v_rel   = 7800 * [cosd(20), 0, sind(20)];
angles  = 0:0.5:40;   % [deg]
upper   = [0 0 0 1];  % which wing turns
lower   = [1 0 0 0];
pitch   = zeros(numel(angles), 2);
for i = 1:numel(angles)
    deflect(geometry, hinges, angles(i) * upper);
    [~, T] = calculator.calc_aero_load(v_rel, surface_temp__K, conditions);
    pitch(i, 1) = T(2);

    deflect(geometry, hinges, angles(i) * lower);
    [~, T] = calculator.calc_aero_load(v_rel, surface_temp__K, conditions);
    pitch(i, 2) = T(2);
end
deflect(geometry, hinges, [0 0 0 0]);

figure('Name', 'Pitch torque over wing angle');
plot(angles, 1e6 * pitch, 'LineWidth', 1.5);
grid on;
xlabel('wing angle [deg]');
ylabel('pitch torque T_y [\muNm]');
legend('upper wing turned out', 'lower wing turned out', 'Location', 'best');
title('Flow at 20 deg angle of attack');

%% 3. Why the upper wing hardly matters
% With the upper wing turned out by 30 deg, the shading view shows it in
% the shadow of the body. show_shading opens an interactive 3D window and
% waits until it is closed.
deflect(geometry, hinges, 30 * upper);
vat.visualization.show_shading(geometry, pipeline.shade(v_rel), v_rel);
deflect(geometry, hinges, [0 0 0 0]);

%% Local functions
function deflect(geometry, hinges, angles__deg)
    % Sets every wing to its angle; angles are absolute, not added up.
    for i = 1:numel(hinges)
        geometry.turn_mesh_around_axis(hinges(i).mesh_id, deg2rad(angles__deg(i)), ...
            hinges(i).origin, hinges(i).axis);
    end
end
