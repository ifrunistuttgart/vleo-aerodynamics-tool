%% Benchmark: angle of attack of a two-sided flat plate
% Compares both shading algorithms with direct GSI model evaluations for
% every MATLAB GSI model.

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
num_pixel         = 500;
alphas__deg       = 0:1:360;

aero_cond = vat.AeroConditions(rho__kg_per_m3, T_atmospheric__K, particle_mass__kg);
folder = fileparts(mfilename('fullpath'));
mesh_file = fullfile(folder, '..', 'geometries', 'two_sided_plate.obj');
geometry = vat.geometry.RotatableMeshGeometry(mesh_file);

model_names = {'Newton', 'Maxwell', 'Cook', 'Schaaf-Chambre', 'Storch', 'Sentman'};
shading_names = {'Binary', 'CoP'};
num_models = numel(model_names);
num_shading = numel(shading_names);
num_angles = numel(alphas__deg);

reference_drag__N = zeros(num_angles, num_models);
reference_lift__N = zeros(num_angles, num_models);
numerical_drag__N = zeros(num_angles, num_models, num_shading);
numerical_lift__N = zeros(num_angles, num_models, num_shading);
%%
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
        normal_front = [sin(alpha__rad), 0, cos(alpha__rad)];
        normal_back = -normal_front;
        v_rel_global__m_per_s = [speed__m_per_s, 0, 0];

        [force_front__N, ~] = gsi_model.calc_aero_force_torque( ...
            area__m2, normal_front, [0, 0, 0], v_rel_global__m_per_s, ...
            T_wall__K, aero_cond);
        [force_back__N, ~] = gsi_model.calc_aero_force_torque( ...
            area__m2, normal_back, [0, 0, 0], v_rel_global__m_per_s, ...
            T_wall__K, aero_cond);
        force_two_sided__N = force_front__N(:) + force_back__N(:);
        reference_drag__N(angle_index, model_index) = -force_two_sided__N(1);
        reference_lift__N(angle_index, model_index) = -force_two_sided__N(3);

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

relative_error_drag = abs(numerical_drag__N - reference_drag__N) ./ ...
    max(abs(reference_drag__N), eps);
relative_error_lift = abs(numerical_lift__N - reference_lift__N) ./ ...
    max(abs(reference_lift__N), eps);

plot_loads(alphas__deg, model_names, shading_names, reference_drag__N, ...
    reference_lift__N, numerical_drag__N, numerical_lift__N);
plot_errors(alphas__deg, model_names, shading_names, relative_error_drag, ...
    relative_error_lift);
%%
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

function plot_loads(alphas, model_names, shading_names, reference_drag, ...
        reference_lift, numerical_drag, numerical_lift)
    figure('Name', 'Two-sided plate: direct GSI and numerical loads', ...
        'Position', [100 100 1200 800]);
    tiledlayout(3, 4, 'TileSpacing', 'compact');
    colors = lines(numel(shading_names));
    for i = 1:numel(model_names)
        nexttile(2 * i - 1);
        plot(alphas, reference_drag(:, i), 'k-', 'LineWidth', 1.5, 'DisplayName', 'Direct GSI');
        hold on;
        for j = 1:numel(shading_names)
            plot(alphas, numerical_drag(:, i, j), '-', 'Color', colors(j, :), ...
                'DisplayName', shading_names{j});
        end
        grid on; title([model_names{i} ' drag']); ylabel('drag [N]');
        if i > 4, xlabel('AOA [deg]'); end
        legend('Location', 'best');
        nexttile(2 * i);
        plot(alphas, reference_lift(:, i), 'k-', 'LineWidth', 1.5, 'DisplayName', 'Direct GSI');
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
