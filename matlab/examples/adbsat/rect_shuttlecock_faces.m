function faces = rect_shuttlecock_faces()
    % RECT_SHUTTLECOCK_FACES Flat rectangular faces of the rectangular shuttlecock.
    %
    %   faces = rect_shuttlecock_faces() returns one struct per flat face, shared
    %   by the mesh generator (make_rect_shuttlecock.m) and the analytic solution
    %   (rect_shuttlecock_analytical.m), so both describe the same geometry.
    %
    %   Geometry (mesh frame, see rect_shuttlecock_analytical.md):
    %     Body  : closed box x in [-0.15, 0.15], y, z in [-0.05, 0.05] [m]
    %     Wings : four zero-thickness, two-sided rectangles, length 0.2 m and
    %             width 0.09 m, rooted on the rear edges of the body and flared
    %             backwards at 45 deg. Each wing is two coincident sheets with
    %             opposite normals ("outer" faces forward/outward, "inner" back).
    %
    %   Each face is the rectangle P0 + u*e1 + v*e2, u, v in [0, 1], with outward
    %   normal along cross(e1, e2). Fields:
    %     object     - mesh object name (Body, wing_top, ...)
    %     P0, e1, e2 - corner and edge vectors [m] (1x3)
    %     shadow     - '' or the wing ('top' / 'bottom') whose outer face the body
    %                  can shade in a pitch sweep; length_axis says which edge
    %                  (1 = e1, 2 = e2) runs along the wing from its root.
    %

    BODY_HALF_LENGTH = 0.15;
    BODY_HALF_WIDTH = 0.05;
    WING_LENGTH = 0.2;
    WING_HALF_WIDTH = 0.045;

    a = BODY_HALF_LENGTH;
    w = BODY_HALF_WIDTH;
    c = WING_LENGTH / sqrt(2);
    b = 2 * WING_HALF_WIDTH;

    faces = struct('object', {}, 'P0', {}, 'e1', {}, 'e2', {}, 'shadow', {}, 'length_axis', {});

    % Body (outward normals)
    faces = add(faces, 'Body', [a, -w, -w], [0, 2 * w, 0], [0, 0, 2 * w]);    % +x front
    faces = add(faces, 'Body', [-a, -w, -w], [0, 0, 2 * w], [0, 2 * w, 0]);   % -x rear
    faces = add(faces, 'Body', [-a, w, -w], [0, 0, 2 * w], [2 * a, 0, 0]);    % +y
    faces = add(faces, 'Body', [-a, -w, -w], [2 * a, 0, 0], [0, 0, 2 * w]);   % -y
    faces = add(faces, 'Body', [-a, -w, w], [2 * a, 0, 0], [0, 2 * w, 0]);    % +z
    faces = add(faces, 'Body', [-a, -w, -w], [0, 2 * w, 0], [2 * a, 0, 0]);   % -z

    % Wings: outer sheet first, then the inner sheet with swapped edges
    wings = { ...
             'wing_top',    [-a, -b / 2, w],  [0, b, 0], c * [-1, 0, 1],  'top',    false; ...
             'wing_bottom', [-a, -b / 2, -w], [0, b, 0], c * [-1, 0, -1], 'bottom', true; ...
             'wing_right',  [-a, w, -b / 2],  [0, 0, b], c * [-1, 1, 0],  '',       true; ...
             'wing_left',   [-a, -w, -b / 2], [0, 0, b], c * [-1, -1, 0], '',       false};
    for k = 1:size(wings, 1)
        [name, root, across, along, shadow, along_first] = wings{k, :};
        if along_first
            faces = add(faces, name, root, along, across, shadow, 1);   % outer
            faces = add(faces, name, root, across, along, '', 2);       % inner
        else
            faces = add(faces, name, root, across, along, shadow, 2);   % outer
            faces = add(faces, name, root, along, across, '', 1);       % inner
        end
    end
end

function faces = add(faces, object, P0, e1, e2, shadow, length_axis)
    if nargin < 6
        shadow = '';
        length_axis = 0;
    end
    faces(end + 1) = struct('object', object, 'P0', P0, 'e1', e1, 'e2', e2, 'shadow', shadow, ...
                            'length_axis', length_axis);
end
