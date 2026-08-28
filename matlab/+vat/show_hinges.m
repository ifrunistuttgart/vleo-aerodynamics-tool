function show_hinges(geometry, hinges)
    % SHOW_HINGES Visualizes hinge points and rotation axes on the geometry.
    %   This function displays the geometry together with the given hinge
    %   definitions, so they can be checked before being used to actually rotate
    %   anything. 
    %
    %
    %   The function blocks until the window is closed.
    %
    %   Input Arguments:
    %       geometry - A vat.RotatableMeshGeometry object.
    %       hinges   - A struct array with fields:
    %                    mesh_id - index of the mesh this hinge turns
    %                    origin  - hinge point [x, y, z] in the body frame
    %                    axis    - rotation axis [x, y, z], need not be normalized
    %
    %   Example:
    %       h(1).mesh_id = 0; h(1).origin = [ 0.5 0 0]; h(1).axis = [0 1 0];
    %       h(2).mesh_id = 1; h(2).origin = [-0.5 0 0]; h(2).axis = [0 1 0];
    %       vat.show_hinges(geometry, h)
    %
    %   The fields are exactly the arguments turn_mesh_around_axis takes, so a
    %   hinge that looks right here can be applied directly:
    %       geometry.turn_mesh_around_axis(h(1).mesh_id, deg2rad(20), ...
    %                                      h(1).origin, h(1).axis)
    %
    %   See also vat.show_meshes, vat.show_shading,
    %            vat.RotatableMeshGeometry/turn_mesh_around_axis

    arguments
        geometry (1,1) vat.RotatableMeshGeometry
        hinges (1,:) struct
    end

    required_fields = ["mesh_id", "origin", "axis"];
    missing = required_fields(~isfield(hinges, required_fields));
    if ~isempty(missing)
        error("vat:show_hinges:missingField", ...
            "hinges is missing the field(s): %s", strjoin(missing, ", "));
    end
    if isempty(hinges)
        error("vat:show_hinges:noHinges", "hinges must contain at least one hinge.");
    end

    num_hinges = numel(hinges);
    mesh_ids = zeros(1, num_hinges);
    origins = zeros(3, num_hinges);
    axes_ = zeros(3, num_hinges);
    for k = 1:num_hinges
        validateattributes(hinges(k).mesh_id, {'numeric'}, ...
            {'scalar', 'integer', 'nonnegative'}, mfilename, ...
            sprintf("hinges(%d).mesh_id", k));
        validateattributes(hinges(k).origin, {'numeric'}, ...
            {'vector', 'numel', 3, 'finite'}, mfilename, ...
            sprintf("hinges(%d).origin", k));
        validateattributes(hinges(k).axis, {'numeric'}, ...
            {'vector', 'numel', 3, 'finite'}, mfilename, ...
            sprintf("hinges(%d).axis", k));
        if norm(hinges(k).axis) == 0
            error("vat:show_hinges:zeroAxis", ...
                "hinges(%d).axis must not be the zero vector.", k);
        end
        mesh_ids(k) = hinges(k).mesh_id;
        origins(:, k) = hinges(k).origin(:);
        axes_(:, k) = hinges(k).axis(:);
    end

    MexGateway("Visualization.show_hinges", int32(geometry.handle_), ...
        int32(mesh_ids), double(origins), double(axes_));
end
