%% Computation time vs mesh fidelity, for several render resolutions
%
% Times one full force/torque evaluation on the shuttlecock model at increasing
% triangle counts, once per render resolution, and plots the result.
%
% Requires the MEX bindings to be built for the current source:
%   pixi run build-matlab

clear; clc;

script_dir = fileparts(mfilename('fullpath'));
addpath(fullfile(script_dir, '..'));
addpath(fullfile(script_dir, '..', 'bin'));

% Per-call logging goes through the MATLAB engine and would dominate the timings.
setLogLevel("error");

mesh_names  = ["shuttlecock_96.obj", "shuttlecock_240.obj", "shuttlecock_960.obj", ...
               "shuttlecock_3840.obj", "shuttlecock_15360.obj", "shuttlecock_61440.obj"];
resolutions = [1000, 2000, 4000, 8000];
num_repeats = 100;

% Atomic oxygen at roughly 300 km, flow along +x in the body frame.
v_rel__m_per_s  = [7800, 0, 0];
surface_temp__K = 300.0;
aero_conditions = AeroConditions(1.2482e-11, 934.0, 16 * 1.6605390689252e-27);
gsi_model       = Sentman(1,0.9);

num_triangles = zeros(numel(mesh_names), 1);
times__ms     = nan(numel(mesh_names), numel(resolutions));

for m = 1:numel(mesh_names)
    satellite = RotatableMeshSatellite(fullfile(script_dir, "geometries", mesh_names(m)));
    num_triangles(m) = satellite.get_num_triangles();

    for r = 1:numel(resolutions)
        try
            pipeline   = ShadingPipeline(satellite, 1, resolutions(r));  % 1 = CoP shader
            calculator = HybridAeroLoadCalculator(satellite, pipeline, gsi_model);

            % Warm-up: the first call fills the geometry cache and the GPU state.
            calculator.calc_aero_load(v_rel__m_per_s, surface_temp__K, aero_conditions);

            samples__ms = zeros(num_repeats, 1);
            for k = 1:num_repeats
                tic;
                calculator.calc_aero_load(v_rel__m_per_s, surface_temp__K, aero_conditions);
                samples__ms(k) = toc * 1000;
            end
            times__ms(m, r) = median(samples__ms);

            delete(calculator);  % before the pipeline it refers to
            delete(pipeline);
        catch err
            fprintf("  %5d px failed: %s\n", resolutions(r), err.message);
        end
        fprintf("%6d triangles, %5d px: %8.2f ms\n", ...
            num_triangles(m), resolutions(r), times__ms(m, r));
    end

    delete(satellite);
end

%% Plot
figure;
loglog(num_triangles, times__ms, "-o", "LineWidth", 1.5, "MarkerSize", 5);
grid on;
xlabel("Number of triangles");
ylabel("Time per force/torque evaluation [ms]");
title("AeroSat computation time vs mesh fidelity");
legend(compose("%d x %d px", resolutions', resolutions'), "Location", "northwest");
