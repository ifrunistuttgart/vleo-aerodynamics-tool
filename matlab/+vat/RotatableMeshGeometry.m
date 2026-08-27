classdef RotatableMeshGeometry < handle
    % ROTATABLEMESHGEOMETRY Represents a geometry whose meshes can be rotated.
    %
    % This class manages the geometric data of a model, including vertices
    % and triangles, and allows for the rotation of individual meshes around
    % defined axes. It is used as the base geometry for shading and aerodynamic
    % load calculations.
    %
    % Terminology used throughout the toolbox:
    %   triangle - the smallest unit, a single triangular face.
    %   mesh     - a group of triangles that moves as one rigid body (e.g. a
    %              single solar panel). A mesh is the unit of rotation, and its
    %              index is the mesh_id passed to turn_mesh_around_axis.
    %   geometry - the complete model, made up of one or more meshes.
    %
    % vat.RotatableMeshGeometry methods:
    %   RotatableMeshGeometry - Constructor to load a geometry from file.
    %   get_vertices          - Retrieves the vertex data of the geometry.
    %   get_num_triangles     - Retrieves the total number of triangles.
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
            %   object by loading the model from the specified file.
            %
            %   Input Arguments:
            %       file_path - Path to the 3D model file (e.g., .obj or .stl).
            %
            arguments
                file_path (1,1) string
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("Geometry.new", string(file_path)));
        end
      
        function delete(this)
            % DELETE Destructor for RotatableMeshGeometry.
            %
            %   Releases the underlying C++ geometry object.
            %
            if this.handle_ ~= int32(-1)
                MexGateway("Geometry.delete", int32(this.handle_));
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
            vertices = MexGateway("Geometry.get_vertices", int32(this.handle_));
        end

        function num_triangles = get_num_triangles(this)
            % GET_NUM_TRIANGLES Retrieves the total number of triangles in the geometry.
            %
            %   num_triangles = get_num_triangles(this) returns the total count
            %   of triangular faces across all meshes of the geometry.
            %
            %   Output Arguments:
            %       num_triangles - The number of triangles.
            %
            num_triangles = MexGateway("Geometry.get_num_triangles", int32(this.handle_));
        end

        function turn_mesh_around_axis(this, mesh_id, angle__rad, origin, axis)
            % TURN_MESH_AROUND_AXIS Rotates a single mesh of the geometry.
            %
            %   turn_mesh_around_axis(this, mesh_id, angle, origin, axis)
            %   applies a rotation to the specified mesh. The rotation replaces
            %   any previous rotation of that mesh rather than accumulating.
            %
            %   Input Arguments:
            %       mesh_id    - The index of the mesh to rotate, in the order the
            %                    meshes appear in the model file.
            %       angle__rad - The rotation angle in radians, positive according
            %                    to the right-hand rule about axis.
            %       origin     - The 3D coordinates [x, y, z] of the hinge point.
            %       axis       - The 3D vector [x, y, z] defining the axis of rotation.
            %
            arguments
                this (1,1) vat.RotatableMeshGeometry
                mesh_id (1,1) {mustBeInteger, mustBeNonnegative}
                angle__rad (1,1) double
                origin (1,3) double
                axis (1,3) double
            end
            MexGateway("Geometry.turn_mesh_around_axis", int32(this.handle_), int32(mesh_id), angle__rad, origin, axis)
        end

        function turn_surface_around_axis(this, mesh_id, angle__rad, origin, axis)
            % TURN_SURFACE_AROUND_AXIS Deprecated alias for turn_mesh_around_axis.
            %
            %   Kept so existing scripts keep working. Use turn_mesh_around_axis
            %   instead; this alias will be removed in a future release.
            %
            warning("vat:deprecated", ...
                "turn_surface_around_axis is deprecated and will be removed. " + ...
                "Use turn_mesh_around_axis instead.");
            this.turn_mesh_around_axis(mesh_id, angle__rad, origin, axis);
        end
    end
end
