function [force__N, torque__Nm, lit_fraction] = rect_shuttlecock_analytical(pitch__deg, param_eq, options)
    % RECT_SHUTTLECOCK_ANALYTICAL Exact Sentman load on the rectangular shuttlecock.
    %
    %   [force__N, torque__Nm] = rect_shuttlecock_analytical(pitch__deg, param_eq)
    %   returns the aerodynamic force and the torque about the mesh origin, both
    %   in the mesh frame, for an ADBSat angle of attack pitch__deg (sideslip 0).
    %   The derivation is in rect_shuttlecock_analytical.md. The Sentman model is
    %   implemented here in closed form, independent of vat and ADBSat.
    %
    %   Input Arguments:
    %       pitch__deg - ADBSat angle of attack [deg], |pitch| < 45.
    %       param_eq   - ADBSat parameter struct (environment() plus alpha, Tw):
    %                    rho (rho(6) = total mass density), vinf, s, Tinf, Tw, alpha.
    %       Shading    - (name-value, default true) false ignores the body's
    %                    shadow on the wings, for the unshaded sanity check.
    %
    %   Output Arguments:
    %       force__N     - total force [N] (1x3).
    %       torque__Nm   - total torque about the origin [Nm] (1x3).
    %       lit_fraction - lit fraction of the shaded wing's outer face [-].
    %
    arguments
        pitch__deg (1, 1) double {mustBeGreaterThan(pitch__deg, -45), mustBeLessThan(pitch__deg, 45)}
        param_eq (1, 1) struct
        options.Shading (1, 1) logical = true
    end

    BODY_LENGTH__M = 0.3;

    if isscalar(param_eq.rho)
        density__kg_per_m3 = param_eq.rho;
    else
        density__kg_per_m3 = param_eq.rho(6);
    end
    q__Pa = 0.5 * density__kg_per_m3 * param_eq.vinf^2;
    s = param_eq.s;
    % Reflected-to-incident speed ratio (hyperthermal energy accommodation), see the md, eq. (8)
    speed_ratio = sqrt(0.5 * (1 + param_eq.alpha * (2 * param_eq.Tw / (param_eq.Tinf * s^2) - 1)));

    alpha = deg2rad(pitch__deg);
    v_hat = [cos(alpha), 0, -sin(alpha)]; % body velocity direction relative to the gas

    % Length of the shaded root strip of the downstream wing, eq. (12)
    shaded_wing = '';
    shaded_length__m = 0;
    if options.Shading && alpha ~= 0
        shaded_length__m = BODY_LENGTH__M * sqrt(2) * tan(abs(alpha)) / (1 - tan(abs(alpha)));
        if alpha > 0
            shaded_wing = 'top';
        else
            shaded_wing = 'bottom';
        end
    end

    force__N = zeros(1, 3);
    torque__Nm = zeros(1, 3);
    lit_fraction = 1;
    for face = rect_shuttlecock_faces()
        normal = cross(face.e1, face.e2);
        area__m2 = norm(normal);
        normal = normal / area__m2;
        centroid = face.P0 + 0.5 * (face.e1 + face.e2);

        if ~isempty(shaded_wing) && strcmp(face.shadow, shaded_wing)
            if face.length_axis == 1
                along = face.e1;
            else
                along = face.e2;
            end
            wing_length__m = norm(along);
            shaded = min(shaded_length__m, wing_length__m);
            lit_fraction = 1 - shaded / wing_length__m;
            area__m2 = area__m2 * lit_fraction;
            % the lit part runs from shaded to wing_length along the wing
            centroid = centroid + along * (0.5 * shaded / wing_length__m);
        end
        if area__m2 == 0
            continue
        end

        face_force__N = q__Pa * area__m2 * sentman_force_coefficient(normal, v_hat, s, speed_ratio);
        force__N = force__N + face_force__N;
        torque__Nm = torque__Nm + cross(centroid, face_force__N);
    end
end

function coefficient = sentman_force_coefficient(normal, v_hat, s, speed_ratio)
    % Force per area / q on a flat face, eqs. (5)-(7) of the md: -c_p n + c~ (gamma n - v_hat).
    gamma = dot(normal, v_hat);
    e = exp(-s^2 * gamma^2);
    E = 1 + erf(s * gamma);
    c_p = (gamma^2 + 1 / (2 * s^2)) * E + gamma * e / (sqrt(pi) * s) + ...
        0.5 * speed_ratio * (sqrt(pi) * gamma * E + e / s);
    c_tilde = gamma * E + e / (sqrt(pi) * s);
    coefficient = -c_p * normal + c_tilde * (gamma * normal - v_hat);
end
