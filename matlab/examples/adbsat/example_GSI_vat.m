%% ADBSat example_GSI with the toolbox GSI models
% Reproduces ADBSat's examples/example_GSI.m (flat-plate Cd and Cl over the
% incidence angle) with the vat GSI models. Every ADBSat model with a vat
% equivalent (Sentman, Maxwell, Cook, Schaaf-Chambre, Storch, Newton) is
% compared against ADBSat; only Sentman is plotted.
% adbsat_to_vat() maps the ADBSat parameter struct param_eq onto
% vat.AeroConditions and the vat.gsi_models objects.
%
% If ADBSat is on the MATLAB path (run its ADBSat_dynpath first), the
% environment is computed with ADBSat's environment() (needs the Aerospace
% Toolbox for atmosnrlmsise00) and the ADBSat coefficients are overlaid for
% comparison. Otherwise param_eq is built from the manual state below with the
% same formulas ADBSat uses.
%
% Incidence angle delta is ADBSat's: the angle between the surface normal and
% the flow, so delta = 0 is face-on.

clear; close all;
vat.setLogLevel("warn");

%% ADBSat inputs (identical to example_GSI.m)
h = 300e3;                        % Altitude [m]
lat = 0;                          % Geodetic latitude [deg]
lon = 0;                          % Longitude [deg]
dayOfYear = 1;                    % Day of year [1-365]
UTseconds = 0;                    % Seconds of the day [s]
f107Average = 140;                % Solar activity
f107Daily = 140;
magneticIndex = ones([1, 7]) * 15; % Geomagnetic activity
AnO = 1;                          % Anomalous oxygen flag
delta = deg2rad(0:1:90);          % Incidence angle [rad]

% GSI parameters (identical to example_GSI.m)
alpha = 1;                        % Energy accommodation (Sentman, Maxwell, Cook) [-]
Tw = 300;                         % Wall temperature [K]
sigmaN = 1;                       % Normal momentum accommodation (Schaaf-Chambre, Storch) [-]
sigmaT = 1;                       % Tangential momentum accommodation (Schaaf-Chambre, Storch) [-]

%% Environment
adbsat_available = exist('environment', 'file') == 2 && exist('coeff_sentman', 'file') == 2;
if adbsat_available && exist('atmosnrlmsise00', 'file') == 2
    param_eq = environment(struct(), h, lat, lon, dayOfYear, UTseconds, f107Average, f107Daily, ...
                           magneticIndex, AnO);
else
    error('example_GSI_vat:noEnvironment', ...
            'ADBSat environment() not available, using the manual atmospheric state.');
end
param_eq.alpha = alpha;
param_eq.Tw = Tw;
param_eq.sigmaN = sigmaN;
param_eq.sigmaT = sigmaT;

%% Toolbox evaluation for every ADBSat model with a vat equivalent
adbsat_models = {'sentman', 'maxwell', 'cook', 'schaaf', 'storchHyp', 'newton'};
num_models = numel(adbsat_models);
cd_vat = zeros(num_models, numel(delta));
cl_vat = zeros(num_models, numel(delta));
for k = 1:num_models
    [cd_vat(k, :), cl_vat(k, :)] = plate_coefficients(adbsat_to_vat(param_eq, ['coeff_' adbsat_models{k}]), delta);
end

%% Comparison with ADBSat
if adbsat_available
    cd_adb = zeros(num_models, numel(delta));
    cl_adb = zeros(num_models, numel(delta));
    fprintf('%-10s %12s %12s\n', 'model', 'max |dCd|', 'max |dCl|');
    for k = 1:num_models
        [~, ~, cd_adb(k, :), cl_adb(k, :)] = feval(['coeff_' adbsat_models{k}], param_eq, delta);
        fprintf('%-10s %12.3e %12.3e\n', adbsat_models{k}, ...
                max(abs(cd_vat(k, :) - cd_adb(k, :))), max(abs(cl_vat(k, :) - cl_adb(k, :))));
    end
end

%% Post processing: Sentman only
sentman_index = find(strcmp(adbsat_models, 'sentman'));
delta__deg = rad2deg(delta);
figure();
hold on;
plot(delta__deg, cd_vat(sentman_index, :), '--', 'LineWidth', 1.25);
labels = {'C_D Sentman vat'};
if adbsat_available
    plot(delta__deg, cd_adb(sentman_index, :), 'o', 'MarkerSize', 4);
    labels = [labels, {'C_D Sentman ADBSat'}];
end
grid on;
ylabel('Drag Coefficient, C_D');

yyaxis right;
hold on;
plot(delta__deg, cl_vat(sentman_index, :), '--', 'LineWidth', 1.25);
labels = [labels, {'C_L Sentman vat'}];
if adbsat_available
    plot(delta__deg, cl_adb(sentman_index, :), 'o', 'MarkerSize', 4);
    labels = [labels, {'C_L Sentman ADBSat'}];
end
ylabel('Lift Coefficient, C_L');
xlabel('Incidence Angle [deg]');
legend(labels, 'Location', 'best');

%% Local functions
function [cd, cl] = plate_coefficients(vat_setup, delta)
    % Flat plate of unit area, flow along +x, normal tilted by delta in the
    % x-z plane. Drag is along -x and lift along -z (see one_sided_plate_gsi.m).
    area__m2 = 1.0;
    v_rel__m_per_s = [vat_setup.speed__m_per_s, 0, 0];
    q_times_area__N = vat_setup.dynamic_pressure__Pa * area__m2;
    cd = zeros(size(delta));
    cl = zeros(size(delta));
    for i = 1:numel(delta)
        normal = [cos(delta(i)), 0, sin(delta(i))];
        force__N = vat_setup.gsi_model.calc_aero_force_torque(area__m2, normal, [0, 0, 0], v_rel__m_per_s, ...
                                                              vat_setup.T_wall__K, vat_setup.aero_cond);
        cd(i) = -force__N(1) / q_times_area__N;
        cl(i) = -force__N(3) / q_times_area__N;
    end
end

function param_eq = manual_param_eq(h, density__kg_per_m3, T_atmospheric__K, mmean__g_per_mol)
    % Same derived quantities as ADBSat's environment(), with its constants.
    MU_EARTH__M3_PER_S2 = 3.986004418e14;
    R_EARTH__M = 6.37813649e6;
    R_UNIVERSAL__J_PER_MOL_K = 8.31446261815324;
    KB_ADBSAT__J_PER_K = 1.3806503e-23;
    AVOGADRO__PER_MOL = 6.02214076e23;

    param_eq = struct();
    param_eq.rho = density__kg_per_m3;
    param_eq.Tinf = T_atmospheric__K;
    param_eq.mmean = mmean__g_per_mol;
    param_eq.Rmean = R_UNIVERSAL__J_PER_MOL_K / mmean__g_per_mol * 1000;
    param_eq.vinf = sqrt(MU_EARTH__M3_PER_S2 / (R_EARTH__M + h));
    param_eq.vth = sqrt(2 * KB_ADBSAT__J_PER_K * T_atmospheric__K / (mmean__g_per_mol / AVOGADRO__PER_MOL / 1000));
    param_eq.s = param_eq.vinf / param_eq.vth;
end
