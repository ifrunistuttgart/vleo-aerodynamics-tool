function show_meshes(geometry)
    % SHOW_MESHES Visualizes the geometry with each mesh in its own color.
    %   This function displays the geometry in a 3D plot, coloring each mesh
    %   differently and adding a legend that names them. Every legend entry reads
    %   "[mesh_id] name", where mesh_id is the value turn_mesh_around_axis expects
    %   and name is the mesh name stored in the model file. Use this view to work
    %   out which mesh_id belongs to which part of the model.
    %
    %   The function blocks until the window is closed.
    %
    %   Input Arguments:
    %       geometry - A vat.RotatableMeshGeometry object.
    %
    %   Example:
    %       geometry = vat.RotatableMeshGeometry("my_satellite.obj");
    %       vat.show_meshes(geometry)
    %
    %   See also vat.show_shading, vat.show_hinges

    arguments
        geometry (1,1) vat.RotatableMeshGeometry
    end

    MexGateway("Visualization.show_meshes", int32(geometry.handle_));
end
