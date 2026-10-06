%% Example 5: Where does the load come from?
% The per-pixel calculator can keep the pressure of every pixel, which
% shows where on the geometry the flow pushes, seen from upstream. It also
% reports the areas it integrated over.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

conditions      = vat.AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
surface_temp__K = 300;
gsi_model       = vat.gsi_models.Sentman(1, 0.95);

obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

%% Calculator that keeps the pressure image
calculator = vat.loads.PixelForceTorqueCalculator( ...
    geometry, gsi_model, 2000, KeepPressureImage=true);

aoa   = 4;   % [deg]
v_rel = 7800 * [cosd(aoa), sind(aoa), 0];
[F, T] = calculator.calc_aero_load(v_rel, surface_temp__K, conditions);

%% Areas
% wetted:    surface area the flow reaches
% projected: the same surfaces seen along the flow (the frontal area)
[wetted, projected] = calculator.last_areas();
fprintf('Force  [N]   : %+.4e %+.4e %+.4e\n', F);
fprintf('Torque [Nm]  : %+.4e %+.4e %+.4e\n', T);
fprintf('Wetted area  : %.4f m^2\n', wetted);
fprintf('Frontal area : %.4f m^2\n', projected);

%% Pressure image
% The face the flow hits head-on carries most of the pressure; surfaces
% seen almost edge-on carry a small, mostly thermal pressure. The log scale
% shows both. The raw matrix is calculator.pressure_image().
figure('Name', 'Pressure image');
vat.visualization.show_pressure_image(calculator, geometry, Scale="log");
title(sprintf('Sentman, angle of attack %d deg', aoa));
