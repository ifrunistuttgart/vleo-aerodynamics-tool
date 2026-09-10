function show_meshes(geometry, options)
    % SHOW_MESHES Visualizes the geometry with each mesh in its own color.
    %   This function displays the geometry in a 3D plot, coloring each mesh
    %   differently and adding a legend that names them. Every legend entry reads
    %   'id <mesh_id>  "<name>"', where mesh_id is the value turn_mesh_around_axis expects
    %   and name is the mesh name stored in the model file. Use this view to work
    %   out which mesh_id belongs to which part of the model.
    %
    %   The function blocks until the window is closed.
    %
    %   Input Arguments:
    %       geometry - A vat.geometry.RotatableMeshGeometry object.
    %
    %   Example:
    %       geometry = vat.geometry.RotatableMeshGeometry("my_satellite.obj");
    %       vat.visualization.show_meshes(geometry)
    %
    %
    %   Name-Value Arguments:
    %       ShowTriangleEdges - Draw the triangle mesh over the surfaces (default true).
    %           The overlay shows how the model is discretised, which is what the
    %           shading pass actually rasterises. It can also be toggled in the window
    %           with the T key, and fades out on its own as you zoom out.
    %
    %   See also vat.visualization.show_shading, vat.visualization.show_hinges

    arguments
        geometry (1,1) vat.geometry.RotatableMeshGeometry
        options.ShowTriangleEdges (1,1) logical = true
    end

    MexGateway("visualization.show_meshes", int32(geometry.handle_), ...
        logical(options.ShowTriangleEdges));
end
