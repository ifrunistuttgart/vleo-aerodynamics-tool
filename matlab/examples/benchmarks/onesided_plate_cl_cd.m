%% Lift and drag coefficients of a one-sided flat plate
% Calculates C_L and C_D with every MATLAB GSI model for a unit-area plate
% centered at the origin over a complete 0-360 degree angle sweep.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')
%
% The relative velocity is fixed along global +x. The plate normal is
% rotated in the global x-z plane, so alpha = 90 degrees is face-on and
% alpha = 0 degrees is edge-on. The plate is one-sided: loads are set to
% zero when the flow reaches the back face.

clear; close all;
vat.setLogLevel("warn");

%% Atmosphere, plate, and GSI parameters
rho__kg_per_m3    = 1.2482e-11;
T_atmospheric__K  = 934.0;
particle_mass__kg = 16 * 1.6605390689252e-27;
surface_temp__K   = 300.0;
speed__m_per_s    = 7800.0;
area__m2          = 1.0;
v_rel__m_per_s    = [speed__m_per_s, 0, 0];
centroid__m       = [0, 0, 0];

alpha_e      = single(0.9);
sigma_n      = single(0.9);
sigma_t      = single(0.9);
V_w__m_per_s = single(100.0);

conditions = vat.AeroConditions( ...
    rho__kg_per_m3, T_atmospheric__K, particle_mass__kg);

model_names = {'Newton', 'Maxwell', 'Cook', 'Schaaf-Chambre', ...
    'Storch', 'Sentman'};
num_models = numel(model_names);
models = cell(num_models, 1);
for model_index = 1:num_models
    models{model_index} = make_model(model_names{model_index}, ...
        alpha_e, sigma_n, sigma_t, V_w__m_per_s);
end

%% Angle sweep and aerodynamic loads
alphas__deg = (0:1:360).';
alphas__rad = deg2rad(alphas__deg);
num_angles = numel(alphas__deg);

drag__N = zeros(num_angles, num_models);
lift__N = zeros(num_angles, num_models);
dynamic_pressure__Pa = 0.5 * rho__kg_per_m3 * speed__m_per_s^2;

for model_index = 1:num_models
    for angle_index = 1:num_angles
        alpha__rad = alphas__rad(angle_index);
        normal = [sin(alpha__rad), 0, cos(alpha__rad)];

        [force__N, ~] = models{model_index}.calc_aero_force_torque( ...
            area__m2, normal, centroid__m, v_rel__m_per_s, ...
            surface_temp__K, conditions);

        drag__N(angle_index, model_index) = -force__N(1);
        lift__N(angle_index, model_index) = -force__N(3);
    end
end

cd = drag__N / (dynamic_pressure__Pa * area__m2);
cl = lift__N / (dynamic_pressure__Pa * area__m2);

%% Plot coefficients
figure('Name', 'One-sided plate: aerodynamic coefficients');
tiledlayout(2, 1);

nexttile;
plot(alphas__deg, cd, 'LineWidth', 1.2);
grid on;
xlim([0, 360]);
yline(0, 'k-');
xlabel('\alpha [deg]');
ylabel('C_D');
title('One-sided flat plate: drag coefficient');
legend(model_names, 'Location', 'best');

nexttile;
plot(alphas__deg, cl, 'LineWidth', 1.2);
grid on;
xlim([0, 360]);
yline(0, 'k-');
xlabel('\alpha [deg]');
ylabel('C_L');
title('One-sided flat plate: lift coefficient');
legend(model_names, 'Location', 'best');

function model = make_model(name, alpha_e, sigma_n, sigma_t, V_w__m_per_s)
    switch name
        case 'Newton'
            model = vat.gsi_models.Newton();
        case 'Maxwell'
            model = vat.gsi_models.Maxwell(alpha_e);
        case 'Cook'
            model = vat.gsi_models.Cook(alpha_e);
        case 'Schaaf-Chambre'
            model = vat.gsi_models.SchaafChambre(sigma_n, sigma_t);
        case 'Storch'
            model = vat.gsi_models.Storch(V_w__m_per_s, sigma_n, sigma_t);
        case 'Sentman'
            model = vat.gsi_models.Sentman(1, alpha_e);
        otherwise
            error('Unknown GSI model: %s', name);
    end
end
