function vat_setup = adbsat_to_vat(param_eq, gsi_model)
    % ADBSAT_TO_VAT Converts ADBSat solver parameters to the matching toolbox objects.
    %
    %   vat_setup = adbsat_to_vat(param_eq, gsi_model) takes the ADBSat
    %   parameter struct (as filled by ADBSat's environment() plus the GSI
    %   fields alpha, sigmaN, sigmaT, Tw) and the ADBSat GSI model (a handle
    %   such as @coeff_sentman, or its name 'coeff_sentman' / 'sentman') and
    %   returns everything needed to evaluate the same model with vat.
    %
    %   Input Arguments:
    %       param_eq  - ADBSat parameter struct. Used fields:
    %                     rho   - NRLMSISE-00 output (rho(6) = total mass
    %                             density [kg/m^3]) or a scalar density.
    %                     Tinf  - Free-stream temperature [K].
    %                     mmean - Mean molecular mass [g/mol].
    %                     vinf  - Free-stream speed [m/s].
    %                     Rmean - Specific gas constant [J/(kg K)] (Cook, Storch).
    %                     Tw    - Wall temperature [K].
    %                     alpha, sigmaN, sigmaT - accommodation coefficients,
    %                             as required by the chosen model.
    %       gsi_model - ADBSat GSI function handle or name.
    %
    %   Output Arguments:
    %       vat_setup - Struct with fields
    %                     gsi_model          - vat.gsi_models.* object.
    %                     aero_cond          - vat.AeroConditions object.
    %                     speed__m_per_s     - |v_rel| to use [m/s].
    %                     T_wall__K          - surface temperature [K].
    %                     dynamic_pressure__Pa - 0.5 * rho * V^2, the ADBSat
    %                                          normalisation of all coefficients.
    %                     model_name         - normalised ADBSat model name.
    %
    %   Notes on the mapping (ADBSat toolbox/fmf_eq -> src/gsi_models):
    %     coeff_sentman  -> Sentman(3, alpha). ADBSat uses the hyperthermal
    %                       reflected-velocity ratio sqrt(0.5*(1+alpha*(4kTw/(mV^2)-1))),
    %                       which is temperature_ratio_method 3 (Koppenwallner).
    %     coeff_maxwell  -> Maxwell(alpha). ADBSat's f = 1 - alpha is our epsilon.
    %     coeff_cook     -> Cook(alpha). ADBSat replaces Tinf by the kinetic
    %                       temperature V^2/(3*Rmean), so a separate AeroConditions
    %                       with that temperature is returned.
    %     coeff_schaaf   -> SchaafChambre(sigmaN, sigmaT).
    %     coeff_storchHyp -> Storch(V_w, sigmaN, sigmaT), V_w = sqrt(pi*Rmean*Tw/2).
    %     coeff_newton   -> Newton().
    %     coeff_DRIA, coeff_CLL -> no equivalent (species-resolved models).
    %
    %   ADBSat uses kb = 1.3806503e-23 J/K (CODATA 1998), the toolbox uses the
    %   exact SI value 1.380649e-23 J/K (since 2019). The particle mass is scaled
    %   by kb_vat/kb_adbsat so the speed ratio s = V/sqrt(2*kb*T/m) is identical
    %   in both codes; the mass enters every model only through kb/m, so this
    %   reproduces ADBSat exactly. The effect (~5e-7 relative) is below single
    %   precision and is kept for consistency only.
    %
    arguments
        param_eq (1, 1) struct
        gsi_model
    end

    KB_ADBSAT__J_PER_K = 1.3806503e-23;
    KB_VAT__J_PER_K = 1.380649e-23;
    AVOGADRO__PER_MOL = 6.02214076e23; % same value as ADBSatConstants

    model_name = normalise_model_name(gsi_model);

    density__kg_per_m3 = total_density(param_eq.rho);
    speed__m_per_s = param_eq.vinf;
    T_atmospheric__K = param_eq.Tinf;
    T_wall__K = param_eq.Tw;
    particle_mass__kg = param_eq.mmean / AVOGADRO__PER_MOL / 1000 * KB_VAT__J_PER_K / KB_ADBSAT__J_PER_K;

    switch model_name
        case 'sentman'
            model = vat.gsi_models.Sentman(3, single(param_eq.alpha));
        case 'maxwell'
            model = vat.gsi_models.Maxwell(single(param_eq.alpha));
        case 'cook'
            model = vat.gsi_models.Cook(single(param_eq.alpha));
            T_atmospheric__K = speed__m_per_s^2 / (3 * param_eq.Rmean);
        case 'schaaf'
            model = vat.gsi_models.SchaafChambre(single(param_eq.sigmaN), single(param_eq.sigmaT));
        case 'storchhyp'
            V_w__m_per_s = sqrt(pi * param_eq.Rmean * T_wall__K / 2);
            model = vat.gsi_models.Storch(single(V_w__m_per_s), single(param_eq.sigmaN), single(param_eq.sigmaT));
        case 'newton'
            model = vat.gsi_models.Newton();
        case {'dria', 'cll'}
            error('adbsat_to_vat:unsupported', ...
                  'ADBSat model coeff_%s is species-resolved and has no equivalent in the toolbox.', ...
                  upper(model_name));
        otherwise
            error('adbsat_to_vat:unknown', 'Unknown ADBSat GSI model: %s', model_name);
    end

    vat_setup = struct();
    vat_setup.gsi_model = model;
    vat_setup.aero_cond = vat.AeroConditions(density__kg_per_m3, T_atmospheric__K, particle_mass__kg);
    vat_setup.speed__m_per_s = speed__m_per_s;
    vat_setup.T_wall__K = T_wall__K;
    vat_setup.dynamic_pressure__Pa = 0.5 * density__kg_per_m3 * speed__m_per_s^2;
    vat_setup.model_name = model_name;
end

function name = normalise_model_name(gsi_model)
    if isa(gsi_model, 'function_handle')
        name = func2str(gsi_model);
    else
        name = char(gsi_model);
    end
    name = lower(regexprep(name, '^@?(coeff_)?', ''));
end

function density__kg_per_m3 = total_density(rho)
    % NRLMSISE-00 (atmosnrlmsise00) puts the total mass density at index 6;
    % the other entries are number densities [1/m^3].
    if isscalar(rho)
        density__kg_per_m3 = rho;
    else
        density__kg_per_m3 = rho(6);
    end
end
