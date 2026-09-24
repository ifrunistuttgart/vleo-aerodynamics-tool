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
    %   get_mesh_quality         - Size and shape statistics of the triangles.
    %   predict_remesh           - What remesh would produce, without remeshing.
    %   remesh                   - A copy with near-equilateral triangles.
    %   export_obj               - Writes the geometry to an .obj file.
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
                file_path (1,1) string = missing
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            % Without a file the object stays empty, for remesh to attach the C++
            % geometry it created.
            if ismissing(file_path)
                return
            end
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

        function quality = get_mesh_quality(this)
            % GET_MESH_QUALITY Size and shape statistics of the geometry's triangles.
            %
            %   quality = get_mesh_quality(this) returns a struct with the triangle
            %   count, total area, bounding radius, and min/p05/median/p95/max of each
            %   triangle's area, aspect ratio (1 = equilateral) and narrowest width
            %   (min_altitude__m). The narrowest width, not the area, decides whether
            %   the shading raster resolves a triangle.
            %
            arguments
                this (1,1) vat.geometry.RotatableMeshGeometry
            end
            quality = MexGateway("geometry.RotatableMeshGeometry.get_mesh_quality", int32(this.handle_));
        end

        function prediction = predict_remesh(this, options)
            % PREDICT_REMESH What remesh would produce, without remeshing.
            %
            %   prediction = predict_remesh(this, TriangleCount=N) or
            %   predict_remesh(this, EdgeLength=h) returns the edge length and the
            %   predicted triangle count and memory. Takes the same options as remesh.
            %   Cheap enough to try sizes. The num_pixel suited to the result depends
            %   on its narrowest triangles, so it is reported by remesh, not here.
            %
            %   The predicted count assumes ideal equilateral triangles; the real count
            %   lands above it, by ~5 % for a few large flat parts and 30 % or more for
            %   many small parts with sharp edges.
            %
            arguments
                this (1,1) vat.geometry.RotatableMeshGeometry
                options.TriangleCount (1,1) double {mustBeInteger, mustBeNonnegative} = 0
                options.EdgeLength (1,1) double {mustBeNonnegative} = 0
                options.FeatureAngle (1,1) double {mustBeInRange(options.FeatureAngle, 0, 180)} = 30
                options.Iterations (1,1) double {mustBeInteger, mustBePositive} = 3
            end
            vat.geometry.RotatableMeshGeometry.require_one_size(options);
            prediction = MexGateway("geometry.RotatableMeshGeometry.predict_remesh", int32(this.handle_), ...
                int32(options.TriangleCount), options.EdgeLength, options.FeatureAngle, int32(options.Iterations));
        end

        function [remeshed, report] = remesh(this, options)
            % REMESH A copy of the geometry with near-equilateral triangles of one size.
            %
            %   [remeshed, report] = remesh(this, TriangleCount=N) or
            %   remesh(this, EdgeLength=h) returns a new geometry; this one is left as
            %   it is. Give exactly one of TriangleCount and EdgeLength [m].
            %
            %   Every mesh keeps its mesh_id and name, so hinge definitions still apply.
            %   Edges sharper than FeatureAngle [deg] and the rims of open panels stay
            %   where they are, and no triangle is flipped. The new geometry starts
            %   with all meshes unturned. Warns above 250k triangles and refuses more
            %   than 1M; use predict_remesh to try sizes first.
            %
            %   report holds before/after mesh quality, the area change in percent,
            %   what repair changed, and suggested_num_pixel.cop / .binary for the
            %   new mesh -- what ShadingPipeline picks when num_pixel is omitted.
            %
            %   Name-Value Arguments:
            %       TriangleCount - Approximate number of triangles.
            %       EdgeLength    - Target edge length [m].
            %       FeatureAngle  - Dihedral angle above which an edge is kept sharp
            %                       [deg]. Default 30.
            %       Iterations    - Remeshing passes. Default 3.
            %
            arguments
                this (1,1) vat.geometry.RotatableMeshGeometry
                options.TriangleCount (1,1) double {mustBeInteger, mustBeNonnegative} = 0
                options.EdgeLength (1,1) double {mustBeNonnegative} = 0
                options.FeatureAngle (1,1) double {mustBeInRange(options.FeatureAngle, 0, 180)} = 30
                options.Iterations (1,1) double {mustBeInteger, mustBePositive} = 3
            end
            vat.geometry.RotatableMeshGeometry.require_one_size(options);
            [id, report] = MexGateway("geometry.RotatableMeshGeometry.remesh", int32(this.handle_), ...
                int32(options.TriangleCount), options.EdgeLength, options.FeatureAngle, int32(options.Iterations));
            remeshed = vat.geometry.RotatableMeshGeometry();
            remeshed.handle_ = int32(id);
        end

        function export_obj(this, file_path)
            % EXPORT_OBJ Writes the geometry to a Wavefront .obj file.
            %
            %   export_obj(this, file_path) writes one object per mesh, in mesh_id
            %   order and with its name, so the file loads back with the same
            %   mesh_ids. Coordinates read back to within one float ulp.
            %
            %   All meshes must be unturned (angle 0): the file must hold the
            %   geometry that hinge angles are applied to.
            %
            arguments
                this (1,1) vat.geometry.RotatableMeshGeometry
                file_path (1,1) string
            end
            MexGateway("geometry.RotatableMeshGeometry.export_obj", int32(this.handle_), file_path);
        end
    end

    methods (Static, Access = private)
        function require_one_size(options)
            if (options.TriangleCount > 0) == (options.EdgeLength > 0)
                error("vat:remesh:size", "Give exactly one of TriangleCount and EdgeLength.");
            end
        end
    end
end
