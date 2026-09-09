classdef RotatableMeshGeometry < handle
    % ROTATABLEMESHGEOMETRY Represents a geometry with rotatable meshes.
    %
    % This class manages the geometric data of a geometry, including vertices
    % and triangles, and allows for the rotation of specific parts around defined
    % axes. It is used as the base geometry for shading and aerodynamic load 
    % calculations.
    %
    % RotatableMeshGeometry methods:
    %   RotatableMeshGeometry   - Constructor to load a geometry.
    %   get_vertices             - Retrieves the vertex data of the geometry.
    %   get_num_triangles        - Retrieves the total number of triangles.
    %   turn_mesh_around_axis - Rotates a specific mesh of the geometry.
    %
    properties %(Access = private, Hidden = true)
        % store handle as int32 to match MexGateway expectations
        handle_ = int32(-1);
    end
    methods
        function this = RotatableMeshGeometry(file_path)
            % ROTATABLEMESHGEOMETRY Constructor for RotatableMeshGeometry.
            %
            %   obj = RotatableMeshGeometry(file_path) creates a geometry 
            %   object by loading the geometry from the specified file.
            %
            %   Input Arguments:
            %       file_path - Path to the 3D model file (e.g., .obj or .stl).
            %
            arguments
                file_path (1,1) string
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("geometry.RotatableMeshGeometry.new", string(file_path)));
        end
      
        function delete(this)
            % DELETE Destructor for RotatableMeshGeometry.
            %
            %   Releases the underlying C++ geometry object.
            %
            arguments
                this (1,1) vat.geometry.RotatableMeshGeometry
            end
            if this.handle_ ~= int32(-1)
                MexGateway("geometry.RotatableMeshGeometry.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end

        function vertices = get_vertices(this)
            % GET_VERTICES Retrieves the vertex positions of the geometry.
            %
            %   vertices = get_vertices(this) returns a flat array of vertex 
            %   positions (x, y, z triplets).
            %
            %   Output Arguments:
            %       vertices - Array of vertex coordinates [3 x N].
            %
            arguments
                this (1,1) vat.geometry.RotatableMeshGeometry
            end
            vertices = MexGateway("geometry.RotatableMeshGeometry.get_vertices", int32(this.handle_));
        end

        function num_triangles = get_num_triangles(this)
            % GET_NUM_TRIANGLES Retrieves the total number of triangles in the model.
            %
            %   num_triangles = get_num_triangles(this) returns the total count
            %   of triangular faces across all meshes of the geometry.
            %
            %   Output Arguments:
            %       num_triangles - The number of triangles.
            %
            arguments
                this (1,1) vat.geometry.RotatableMeshGeometry
            end
            num_triangles = MexGateway("geometry.RotatableMeshGeometry.get_num_triangles", int32(this.handle_));
        end

        function turn_mesh_around_axis(this,mesh_id, angle__rad, origin, axis)
            % TURN_MESH_AROUND_AXIS Rotates a specific mesh of the geometry.
            %
            %   turn_mesh_around_axis(this, mesh_id, angle, origin, axis)
            %   applies a rotation to the specified mesh.
            %
            %   Input Arguments:
            %       mesh_id - the ID of the mesh to rotate.
            %       angle__rad - The rotation angle in radians.
            %       origin     - The 3D coordinates [x, y, z] of the rotation origin.
            %       axis       - The 3D vector [x, y, z] defining the axis of rotation.
            %
            arguments
                this (1,1) vat.geometry.RotatableMeshGeometry
                mesh_id (1,1) {mustBeInteger, mustBeNonnegative}
                angle__rad (1,1) double
                origin (1,3) double
                axis (1,3) double
            end
            MexGateway("geometry.RotatableMeshGeometry.turn_mesh_around_axis", int32(this.handle_), int32(mesh_id), angle__rad, origin, axis)
        end
    end
end
