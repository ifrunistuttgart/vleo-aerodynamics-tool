%% Benchmark: angle of attack of a two-sided flat plate
% Compares both shading algorithms with the analytical expressions in
% docs/gsi_flat_plate_aoa.md for every MATLAB GSI model.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

rho__kg_per_m3    = 1.2482e-11;
T_atmospheric__K  = 934.0;
particle_mass__kg = 16 * 1.6605390689252e-27;
T_wall__K         = 300.0;
speed__m_per_s    = 7800.0;
area__m2          = 1.0;
alpha_e           = 0.9;
sigma_n           = 0.9;
sigma_t           = 0.9;
V_w__m_per_s      = 100.0;
num_pixel         = 1000;
alphas__deg       = 0:1:180;

aero_cond = vat.AeroConditions(rho__kg_per_m3, T_atmospheric__K, particle_mass__kg);
folder = fileparts(mfilename('fullpath'));
mesh_file = fullfile(folder, '..', 'geometries', 'two_sided_plate.obj');
geometry = vat.geometry.RotatableMeshGeometry(mesh_file);

model_names = {'Newton', 'Maxwell', 'Cook', 'Schaaf-Chambre', 'Storch', 'Sentman'};
shading_names = {'Binary', 'CoP'};
num_models = numel(model_names);
num_shading = numel(shading_names);
num_angles = numel(alphas__deg);

analytical_drag__N = zeros(num_angles, num_models);
analytical_lift__N = zeros(num_angles, num_models);
numerical_drag__N = zeros(num_angles, num_models, num_shading);
numerical_lift__N = zeros(num_angles, num_models, num_shading);

for model_index = 1:num_models
    gsi_model = make_model(model_names{model_index}, alpha_e, sigma_n, sigma_t, V_w__m_per_s);
    calculators = {
        vat.loads.HybridForceTorqueCalculator(geometry, ...
            vat.shading.ShadingPipeline(geometry, 0, num_pixel), gsi_model)
        vat.loads.HybridForceTorqueCalculator(geometry, ...
            vat.shading.ShadingPipeline(geometry, 1, num_pixel), gsi_model)
    };

    for angle_index = 1:num_angles
        alpha__rad = deg2rad(alphas__deg(angle_index));
        [analytical_drag__N(angle_index, model_index), ...
            analytical_lift__N(angle_index, model_index)] = analytical_load( ...
            model_names{model_index}, alpha__rad, rho__kg_per_m3, ...
            speed__m_per_s, T_atmospheric__K, T_wall__K, particle_mass__kg, ...
            alpha_e, sigma_n, sigma_t, V_w__m_per_s, area__m2);

        % The fixed mesh is expressed in the plate frame. Rotating the
        % incoming relative velocity is equivalent to rotating the plate
        % about +y while keeping the global flow along +x.
        v_rel_body__m_per_s = speed__m_per_s * ...
            [cos(alpha__rad), 0, sin(alpha__rad)];
        rotation_body_to_global = [cos(alpha__rad), 0, sin(alpha__rad); ...
                                   0,                1, 0; ...
                                  -sin(alpha__rad), 0, cos(alpha__rad)];
        for shading_index = 1:num_shading
            force_body__N = calculators{shading_index}.calc_aero_load( ...
                v_rel_body__m_per_s, T_wall__K, aero_cond);
            force_global__N = rotation_body_to_global * force_body__N(:);
            numerical_drag__N(angle_index, model_index, shading_index) = -force_global__N(1);
            numerical_lift__N(angle_index, model_index, shading_index) = -force_global__N(3);
        end
    end
end

relative_error_drag = abs(numerical_drag__N - analytical_drag__N) ./ ...
    max(abs(analytical_drag__N), eps);
relative_error_lift = abs(numerical_lift__N - analytical_lift__N) ./ ...
    max(abs(analytical_lift__N), eps);

plot_loads(alphas__deg, model_names, shading_names, analytical_drag__N, ...
    analytical_lift__N, numerical_drag__N, numerical_lift__N);
plot_errors(alphas__deg, model_names, shading_names, relative_error_drag, ...
    relative_error_lift);

%% Local functions
function model = make_model(name, alpha_e, sigma_n, sigma_t, V_w__m_per_s)
    switch name
        case 'Newton'
            model = vat.gsi_models.Newton();
        case 'Maxwell'
            model = vat.gsi_models.Maxwell(single(alpha_e));
        case 'Cook'
            model = vat.gsi_models.Cook(single(alpha_e));
        case 'Schaaf-Chambre'
            model = vat.gsi_models.SchaafChambre(single(sigma_n), single(sigma_t));
        case 'Storch'
            model = vat.gsi_models.Storch(single(V_w__m_per_s), single(sigma_n), single(sigma_t));
        case 'Sentman'
            model = vat.gsi_models.Sentman(1, single(alpha_e));
        otherwise
            error('Unknown GSI model: %s', name);
    end
end

function [drag__N, lift__N] = analytical_load(name, alpha__rad, rho, speed, ...
        T_i, T_w, particle_mass, alpha_e, sigma_n, sigma_t, V_w, area)
    q = 0.5 * rho * speed^2;
    s = sin(alpha__rad);
    c = cos(alpha__rad);
    switch name
        case 'Newton'
            cd = 2 * s^3;
            cl = 2 * s^2 * c;
        case 'Maxwell'
            speed_ratio = speed / sqrt(2 * 1.380649e-23 * T_i / particle_mass);
            epsilon = 1 - alpha_e;
            erf_term = erf(speed_ratio * s);
            cd = 2 * (1 - epsilon * cos(2 * alpha__rad)) / (sqrt(pi) * speed_ratio) * ...
                exp(-speed_ratio^2 * s^2) + s / speed_ratio^2 * ...
                (1 + 2 * speed_ratio^2 + epsilon * (1 - 2 * speed_ratio^2 * cos(2 * alpha__rad))) * erf_term + ...
                (1 - epsilon) * sqrt(pi) / speed_ratio * s^2 * sqrt(T_w / T_i);
            cl = 4 * epsilon / (sqrt(pi) * speed_ratio) * s * c * ...
                exp(-speed_ratio^2 * s^2) + c / speed_ratio^2 * ...
                (1 + epsilon * (1 + 4 * speed_ratio^2 * s^2)) * erf_term + ...
                (1 - epsilon) * sqrt(pi) / speed_ratio * s * c * sqrt(T_w / T_i);
        case 'Cook'
            temperature_ratio = sqrt(1 + alpha_e * (T_w / T_i - 1));
            cd = 2 * s * (1 + 2 / 3 * s * temperature_ratio);
            cl = 4 / 3 * c * s * temperature_ratio;
        case 'Schaaf-Chambre'
            speed_ratio = speed / sqrt(2 * 1.380649e-23 * T_i / particle_mass);
            thermal_ratio = sqrt(T_w / T_i);
            exp_term = exp(-speed_ratio^2 * s^2);
            erf_term = erf(speed_ratio * s);
            cp = ((2 - sigma_n) / sqrt(pi) * speed_ratio * s + sigma_n / 2 * thermal_ratio) * exp_term;
            cp = cp + ((2 - sigma_n) * (speed_ratio^2 * s^2 + 0.5) + ...
                sigma_n / 2 * sqrt(pi) * speed_ratio * s * thermal_ratio) * (1 + erf_term);
            cp = cp / speed_ratio^2;
            ctau = sigma_t * c / (speed_ratio * sqrt(pi)) * ...
                (exp_term + speed_ratio * sqrt(pi) * s * (1 + erf_term));
            cd = cp * s + ctau * c;
            cl = cp * c - ctau * s;
        case 'Storch'
            cp = 2 * s * (sigma_n * V_w / speed + (2 - sigma_n) * s);
            ctau = 2 * sigma_t * c * s;
            cd = cp * s + ctau * c;
            cl = cp * c - ctau * s;
        case 'Sentman'
            [drag__N, lift__N] = analytical_sentman(alpha__rad, rho, speed, ...
                T_i, T_w, particle_mass, alpha_e, area);
            return;
        otherwise
            error('Unknown GSI model: %s', name);
    end
    drag__N = q * area * cd;
    lift__N = q * area * cl;
end

function [drag__N, lift__N] = analytical_sentman(alpha, rho, speed, T_i, T_w, ...
        particle_mass, alpha_e, area)
    k_B = 1.380649e-23;
    v_th = sqrt(2 * k_B * T_i / particle_mass);
    speed_ratio = speed / v_th;
    s = sin(alpha);
    c = cos(alpha);
    s_cos_delta = speed_ratio * s;
    exp_term = exp(-s_cos_delta^2);
    erfc_term = erfc(-s_cos_delta);
    g1 = s_cos_delta / sqrt(pi) * exp_term + (0.5 + s_cos_delta^2) * erfc_term;
    g2 = exp_term / sqrt(pi) + s_cos_delta * erfc_term;
    denominator = exp_term / sqrt(pi) + s_cos_delta * erfc_term;
    theta = alpha_e * (2 * k_B * T_w / (particle_mass * speed^2)) * speed_ratio^2 + ...
        (1 - alpha_e) * (1 + speed_ratio^2 / 2 + 0.25 * s_cos_delta * erfc_term / denominator);
    pressure_scale = rho / 2 * v_th^2;
    term_1 = -(g1 + sqrt(pi) / 2 * sqrt(theta) * g2);
    term_2 = speed_ratio * g2;
    % The gas velocity in the GSI implementation is -v_rel. Projecting the
    % document's vector expression onto drag and lift directions gives:
    drag_coeff = -(term_1 * s - term_2 * (1 - s^2));
    lift_coeff = -(term_1 * c + term_2 * s * c);
    drag__N = pressure_scale * area * drag_coeff;
    lift__N = pressure_scale * area * lift_coeff;
end

function plot_loads(alphas, model_names, shading_names, analytic_drag, ...
        analytic_lift, numerical_drag, numerical_lift)
    figure('Name', 'Two-sided plate: analytical and numerical loads', ...
        'Position', [100 100 1200 800]);
    tiledlayout(3, 4, 'TileSpacing', 'compact');
    colors = lines(numel(shading_names));
    for i = 1:numel(model_names)
        nexttile(2 * i - 1);
        plot(alphas, analytic_drag(:, i), 'k-', 'LineWidth', 1.5, 'DisplayName', 'Analytical');
        hold on;
        for j = 1:numel(shading_names)
            plot(alphas, numerical_drag(:, i, j), '-', 'Color', colors(j, :), ...
                'DisplayName', shading_names{j});
        end
        grid on; title([model_names{i} ' drag']); ylabel('drag [N]');
        if i > 4, xlabel('AOA [deg]'); end
        legend('Location', 'best');
        nexttile(2 * i);
        plot(alphas, analytic_lift(:, i), 'k-', 'LineWidth', 1.5, 'DisplayName', 'Analytical');
        hold on;
        for j = 1:numel(shading_names)
            plot(alphas, numerical_lift(:, i, j), '-', 'Color', colors(j, :), ...
                'DisplayName', shading_names{j});
        end
        grid on; title([model_names{i} ' lift']); ylabel('lift [N]');
        if i > 4, xlabel('AOA [deg]'); end
    end
end

function plot_errors(alphas, model_names, shading_names, drag_error, lift_error)
    figure('Name', 'Two-sided plate: relative error', 'Position', [120 120 1200 800]);
    tiledlayout(3, 4, 'TileSpacing', 'compact');
    colors = lines(numel(shading_names));
    for i = 1:numel(model_names)
        nexttile(2 * i - 1);
        semilogy(alphas, 100 * drag_error(:, i, 1), '-', 'Color', colors(1, :), 'LineWidth', 1);
        hold on; plot(alphas, 100 * drag_error(:, i, 2), '-', 'Color', colors(2, :));
        grid on; title([model_names{i} ' drag error']); ylabel('relative error [%]');
        if i > 4, xlabel('AOA [deg]'); end
        legend(shading_names, 'Location', 'best');
        nexttile(2 * i);
        semilogy(alphas, 100 * lift_error(:, i, 1), '-', 'Color', colors(1, :));
        hold on; plot(alphas, 100 * lift_error(:, i, 2), '-', 'Color', colors(2, :));
        grid on; title([model_names{i} ' lift error']); ylabel('relative error [%]');
        if i > 4, xlabel('AOA [deg]'); end
    end
end
