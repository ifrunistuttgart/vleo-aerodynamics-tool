function show_mesh(satellite, triangle_visbility, velocity__m_per_s)
    % SHOW_MESH Visualizes the satellite mesh with shading information.
    %   This function displays the satellite's mesh in a 3D plot, coloring each triangle
    %   based on its visibility factor (shading) relative to the incoming flow direction.
    %   Input Arguments:
    %       satellite          - A RotatableMeshSatellite object representing the geometry.
    %       triangle_visbility - A vector of visibility factors for each triangle (1.0 = exposed, 0.0 = shaded).
    %       velocity__m_per_s  - The relative velocity vector [x, y, z] in the satellite body frame.

    arguments
        satellite (1,1) RotatableMeshSatellite
        triangle_visbility (:,1) single
        velocity__m_per_s (1,3) double
    end
    MexGateway("Visualization.show_mesh", int32(satellite.handle_), single(triangle_visbility), velocity__m_per_s);
end