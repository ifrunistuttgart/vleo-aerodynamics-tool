function [F, T] = exact_solution_shadowing(alpha__deg, gsi_model, speed, T_wall, aero_cond)
    v_rel = flow(alpha__deg, speed);
    [visible_area, visible_centroid] = downstream_visible_surface(alpha__deg);

    % Plate 1 is at x = 0 and plate 2 at x = 0.5. The +x face of plate 1
    % shadows plate 2; both -x faces are leeward and fully exposed.
    surfaces = {
        1.0, [1, 0, 0], [0, 0, 0]; ...
        1.0, [-1, 0, 0], [0, 0, 0]; ...
        visible_area, [1, 0, 0], visible_centroid; ...
        1.0, [-1, 0, 0], [0.5, 0, 0.5]
    };
    F = zeros(1, 3);
    T = zeros(1, 3);
    for i = 1:size(surfaces, 1)
        [force, torque] = gsi_model.calc_aero_force_torque( ...
            surfaces{i, 1}, surfaces{i, 2}, surfaces{i, 3}, ...
            v_rel, T_wall, aero_cond);
        F = F + force(:).';
        T = T + torque(:).';
    end
end

function [visible_area, visible_centroid] = downstream_visible_surface(alpha__deg)
    % The x spacing translates the upstream plate shadow by this amount in y.
    shift = 0.5 * tand(alpha__deg);
    overlap_lo = max(-0.5, -0.5 + shift);
    overlap_hi = min(0.5, 0.5 + shift);
    overlap_width = max(0, overlap_hi - overlap_lo);
    overlap_area = 0.5 * overlap_width;  % overlap in z is [0, 0.5]
    overlap_centroid = [0.5, 0.5 * (overlap_lo + overlap_hi), 0.25];

    visible_area = 1.0 - overlap_area;
    visible_centroid = ([0.5, 0, 0.5] - overlap_area * overlap_centroid) / visible_area;
end

function v = flow(alpha__deg, speed)
    v = speed * [cosd(alpha__deg), sind(alpha__deg), 0];
end