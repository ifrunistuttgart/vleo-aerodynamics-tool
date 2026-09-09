function show_mesh(geometry, triangle_visbility, velocity__m_per_s)
    % SHOW_MESH Visualizes the geometry mesh with shading information.
    %   This function displays the geometry's mesh in a 3D plot, coloring each triangle
    %   based on its visibility factor (shading) relative to the incoming flow direction.
    %   Input Arguments:
    %       geometry          - A RotatableMeshGeometry object representing the geometry.
    %       triangle_visbility - A vector of visibility factors for each triangle (1.0 = exposed, 0.0 = shaded).
    %       velocity__m_per_s  - The relative velocity vector [x, y, z] in the geometry body frame.

    arguments
        geometry (1,1) vat.geometry.RotatableMeshGeometry
        triangle_visbility (:,1) single
        velocity__m_per_s (1,3) double
    end
    MexGateway("visualization.show_mesh", int32(geometry.handle_), single(triangle_visbility), velocity__m_per_s);
end