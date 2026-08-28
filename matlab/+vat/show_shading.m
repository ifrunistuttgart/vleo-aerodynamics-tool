function show_shading(geometry, triangle_visbility, velocity__m_per_s)
    % SHOW_SHADING Visualizes the geometry coloured by its shading.
    %   This function displays the geometry in a 3D plot, coloring each triangle
    %   based on its visibility factor (shading) relative to the incoming flow direction.
    %   Input Arguments:
    %       geometry           - A vat.RotatableMeshGeometry object.
    %       triangle_visbility - A vector of visibility factors for each triangle (1.0 = exposed, 0.0 = shaded).
    %       velocity__m_per_s  - The relative velocity vector [x, y, z] in the satellite body frame.

    %   See also vat.show_meshes, vat.show_hinges, vat.ShadingPipeline/shade

    arguments
        geometry (1,1) vat.RotatableMeshGeometry
        triangle_visbility (:,1) single
        velocity__m_per_s (1,3) double
    end
    
    MexGateway("Visualization.show_shading", int32(geometry.handle_), triangle_visbility, velocity__m_per_s);
end
