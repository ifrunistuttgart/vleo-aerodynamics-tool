import vleo_aerodynamics_core.*
clear;

%% Import bodies
% Create inputs to import function
% Get absolute path of test folder
[test_folder,~,~] = fileparts(mfilename("fullpath"));
% Get absolute paths of all obj files
filepaths = string(fullfile(test_folder, 'obj_files', ...
                                {'MainBody.obj', ...
                                'WingRight.obj', ...
                                'WingTop.obj', ...
                                'WingLeft.obj', ...
                                'WingBottom.obj' ...
                                }));

% filepaths = string(fullfile(test_folder, 'obj_files', ...
%                                 {'full_satellite.m'
%                                 }));

rotation_hinge_points_CAD = [ 0.00,  0.05, 0.00, -0.05,  0.00;
                              0.00,  0.00, 0.05,  0.00, -0.05;
                              0.00,  0.00, 0.00,  0.00,  0.00];

rotation_directions_CAD = [ 1,  0,  1,  0,  -1;
                            0,  1,  0,  1,  0;
                            0,  0,  0, 0,  0];

surface_temperatures__K = num2cell([300, 300, 300, 300, 300]);

surface_energy_accommodation_coefficients = num2cell(0.9*ones(1,5));

DCM_B_from_CAD = [0, -1, 0;...
                    -1, 0, 0; ...
                    0, 0, -1];

CoM_CAD = [0; 0; 0];

bodies = importMultipleBodies(filepaths, ...
    rotation_hinge_points_CAD, ...
    rotation_directions_CAD, ...
    surface_temperatures__K, ...
    surface_energy_accommodation_coefficients, ...
    DCM_B_from_CAD, ...
    CoM_CAD);

%% Show imported bodies
% Calculate based on actual face counts:
scalar_vals = cell(1, length(bodies));
for i = 1:length(bodies)
    num_faces = size(bodies{i}.vertices_B, 3);
    scalar_vals{i} = 1:num_faces;
end

showBodies(bodies);


%% Calculate the aerodynamic forces and torques for different control surface rotations

% Constants
altitude__m = 3e5;
gravitational_parameter__m3_per_s2 = 3.986e14;
radius__m = 6.378e6;

rotational_velocity_BI_B__rad_per_s = 0;
velocity_I_I__m_per_s = sqrt(gravitational_parameter__m3_per_s2 ...
                             / (radius__m + altitude__m)) * [1;0;0];
wind_velocity_I_I__m_per_s = zeros(3,1);
[T, R] = atmosnrlmsise00(altitude__m, 0, 0, 2024, 150, 0);
density__kg_per_m3 = R(6);
temperature__K = T(2);
particles_mass__kg = 16 * 1.6605390689252e-27;
temperature_ratio_method = 1;

% Loops
num_angles = 101;
control_surface_angles__rad = linspace(0, pi, num_angles);
aerodynamic_force_B__N = nan(3, num_angles, 4, 2);
aerodynamic_torque_B_B__Nm = aerodynamic_force_B__N;
for k = 1:2
    dir = [0; 1; 0];
    attitude_angle = (k-1) * (-pi/4);
    attitude_quaternion_BI = [cos(attitude_angle/2); sin(attitude_angle/2) * dir];
    % Loop over control surfaces
    for i = 1:4
        % Loop over angles
        for j = 1:length(control_surface_angles__rad)
            current_angle = control_surface_angles__rad(j);
            bodies_rotation_angles__rad = zeros(1,5);
            bodies_rotation_angles__rad(1+i) = current_angle;
        
            [aerodynamic_force_B__N(:,j,i,k), ...
                aerodynamic_torque_B_B__Nm(:,j,i,k)] = ...
                vleoAerodynamics(attitude_quaternion_BI, ...
                                            rotational_velocity_BI_B__rad_per_s, ...
                                            velocity_I_I__m_per_s, ...
                                            wind_velocity_I_I__m_per_s, ...
                                            density__kg_per_m3, ...
                                            temperature__K, ... 
                                            particles_mass__kg, ...
                                            bodies, ...                                                       
                                            bodies_rotation_angles__rad, ...
                                            temperature_ratio_method);
        end
    end
end

%% Plot forces and torques
for k = 1:2
    figure;
    tl = tiledlayout('flow');
    if k == 1
        figure_title = 'Nominal Attitude';
    else
        figure_title = 'Pitched up by 45°';
    end
    title(tl, figure_title);
    ax1 = nexttile;
    grid on;
    hold on;
    xlabel('x');
    ylabel('y');
    zlabel('z');
    title('Individual Force Envelopes');
    ax1.DataAspectRatio = [1 1 1];
    legend;
    ax2 = nexttile;
    grid on;
    hold on;
    xlabel('x');
    ylabel('y');
    zlabel('z');
    title('Individual Torque Envelopes');
    ax2.DataAspectRatio = [1 1 1];
    legend;
    
    for i = 1:4
        plot3(ax1, aerodynamic_force_B__N(1,:,i,k), ...
                    aerodynamic_force_B__N(2,:,i,k), ...
                    aerodynamic_force_B__N(3,:,i), ...
                    'DisplayName',['Surface ', num2str(i)]);
        plot3(ax2, aerodynamic_torque_B_B__Nm(1,:,i,k), ...
                    aerodynamic_torque_B_B__Nm(2,:,i,k), ...
                    aerodynamic_torque_B_B__Nm(3,:,i,k), ...
                    'DisplayName',['Surface ', num2str(i)]);
    end
end