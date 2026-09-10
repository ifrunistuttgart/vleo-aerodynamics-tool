%% Example to showcase rotating parts of the satellite

% Minimal logging
vat.setLogLevel("warn")

% First: import the satellite geometry
fp.current_folder  = fileparts(mfilename('fullpath'));
fp.obj_file        = fullfile(fp.current_folder, ...
    "geometries/shuttlecock_15k.obj");

% Define the geometry instance
geometry = vat.geometry.RotatableMeshGeometry(fp.obj_file);

%% Rotating wings around hinge points
% Now we can visualize the geometry and its distinct meshes
% In this example we have four wings (id's from 1 to 4)
vat.visualization.show_meshes(geometry);

% Our goal is to make the four wings rotatable. Let us start by defining a
% matlab struct that holds that information

h(1).mesh_id = 1; % Wing bottom
h(1).origin = [ -0.15 .1 0.05]; h(1).axis = [0 1 0]; 

h(2).mesh_id = 2; % Wing left
h(2).origin = [ -0.15 -0.05 .1]; h(2).axis = [0 0 1]; 
 
h(3).mesh_id = 3; % Wing right
h(3).origin = [ -0.15 0.05 -.1]; h(3).axis = [0 0 -1]; 

h(4).mesh_id = 4; % Wing up
h(4).origin = [ -0.15 -.1 -0.05]; h(4).axis = [0 -1 0]; 

% Now we can check if our hinges are defined correctly:
vat.visualization.show_hinges(geometry, h);

% And finally: we rotate those points
angles = deg2rad([10; 20; 50; -5]);
rotate_shuttlecock_wings(geometry, h, angles);

% Show rotated satellite
vat.visualization.show_meshes(geometry);


%% Drag increase over panel angle
% Now let us rotate all panels from -5 to 90 degrees to see how this
% affects drag

% Aerodynamic Model
alpha_e = 0.95;
aero.model = vat.gsi_models.Sentman(1, alpha_e);

% Environment
T_env   = 934;  % temperature
rho     = 1e-9; % density
ao_mass = 16 * 1.6605390689252e-27;
aero.conditions = vat.AeroConditions(rho, T_env, ao_mass);

shading_pipeline = vat.shading.ShadingPipeline(geometry, 0, 4000);

calculator = vat.loads.HybridForceTorqueCalculator( ...
    geometry, shading_pipeline,aero.model);


% Sweep panel angles
alpha = deg2rad(-5:.6:90);
drag = nan(size(alpha));
v_rel = [7800;0;0];

for i=1:numel(alpha)
    rotate_shuttlecock_wings( ...
        geometry, h, alpha(i).*ones(numel(h),1));

    % calc_aero_load returns [force, torque].
    [F, ~] = calculator.calc_aero_load(v_rel, 300, aero.conditions);

    % drag is negative x-axis force
    drag(i) = -F(1);
end

%% Plotting
panel_angle__deg = rad2deg(alpha);
drag__mN         = drag * 1e3;

figure(1);
plot(panel_angle__deg, drag__mN, 'LineWidth', 2);
grid on;
xlim([panel_angle__deg(1) panel_angle__deg(end)]);
ylim([min(drag__mN), ...
          max(drag__mN)]);
xlabel('Panel angle [deg]');
ylabel('Drag [mN]');
title('Drag over shuttlecock panel angle');

%% Helpers
function rotate_shuttlecock_wings(geometry, hinges, angles)
    % Each turn_mesh_around_axis call is absolute rather than relative, so the
    % sweep can just set the angle it wants without undoing the previous one.
    for i = 1:numel(hinges)
        geometry.turn_mesh_around_axis( ...
            hinges(i).mesh_id, ...
            angles(i), ...
            hinges(i).origin, ...
            hinges(i).axis)
    end
end