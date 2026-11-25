function [F, T] = exact_solution_shadowing(alpha__deg, gsi_model, speed, T_wall, aero_cond)
    % Exact load on the two two-sided plates for any IGSIModel (e.g. Sentman).
    % Valid for flow in the x-y plane with |alpha| < 90 deg. Beyond
    % atand(2) = 63.4 deg the shadow has left the back plate (overlap 0),
    % which the formulas below handle. At +-90 deg the shift diverges.
    assert(isscalar(alpha__deg) && abs(alpha__deg) < 90, ...
           ['exact_solution_shadowing is only valid for |alpha| < 90 deg ' ...
            '(shadow shift 0.5*tand(alpha) diverges at 90 deg), got %g.'], alpha__deg);
    v_rel = flow(alpha__deg, speed);
    [visible_area, visible_centroid] = downstream_visible_surface(alpha__deg);

    % Plate 1 is at x = 0 (y, z in [-0.5, 0.5]) and plate 2 at x = 0.5
    % (y in [-0.5, 0.5], z in [0, 1]). v_rel is the body's velocity, so the
    % +x faces are windward and plate 2 is in front: it shadows the +x face
    % of plate 1. Both -x faces are leeward and fully exposed. Every +x face
    % has the same normal, so which plate is shadowed changes only the torque.
    surfaces = {
                visible_area, [1, 0, 0], visible_centroid; ...
                1.0, [-1, 0, 0], [0, 0, 0]; ...
                1.0, [1, 0, 0], [0.5, 0, 0.5]; ...
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
    % Visible part of the +x face of plate 1 (x = 0). The flow reaches it
    % along -v_rel, so a point (0.5, y, z) of plate 2 casts its shadow at
    % (0, y - shift, z): the shadow is plate 2's y range moved by -shift.
    shift = 0.5 * tand(alpha__deg);
    overlap_lo = max(-0.5, -0.5 - shift);
    overlap_hi = min(0.5, 0.5 - shift);
    overlap_width = max(0, overlap_hi - overlap_lo);
    overlap_area = 0.5 * overlap_width;  % overlap in z is [0, 0.5]
    overlap_centroid = [0, 0.5 * (overlap_lo + overlap_hi), 0.25];

    visible_area = 1.0 - overlap_area;
    visible_centroid = ([0, 0, 0] - overlap_area * overlap_centroid) / visible_area;
end

function v = flow(alpha__deg, speed)
    v = speed * [cosd(alpha__deg), sind(alpha__deg), 0];
end
