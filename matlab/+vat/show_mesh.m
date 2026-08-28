function show_mesh(geometry, triangle_visbility, velocity__m_per_s)
    % SHOW_MESH Deprecated alias for vat.show_shading.
    %
    %   Renamed because show_mesh and show_meshes differed by a single letter
    %   while showing completely different things. The viewers are now named
    %   after what they display:
    %       vat.show_shading - triangles coloured by visibility to the flow
    %       vat.show_meshes  - each mesh in its own colour, with a legend
    %       vat.show_hinges  - hinge points and rotation axes
    %
    %   Use vat.show_shading instead. This alias will be removed in a future
    %   release.
    %
    %   See also vat.show_shading

    arguments
        geometry (1,1) vat.RotatableMeshGeometry
        triangle_visbility (:,1) single
        velocity__m_per_s (1,3) double
    end

    warning("vat:deprecated", ...
        "vat.show_mesh is deprecated and will be removed. " + ...
        "Use vat.show_shading instead.");
    vat.show_shading(geometry, triangle_visbility, velocity__m_per_s);
end
