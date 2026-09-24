classdef ShadingPipeline < handle
    % SHADINGPIPELINE Manages visibility analysis for the triangles of a geometry.
    %
    % This class determines which triangular faces of the geometry mesh are
    % exposed to the incoming flow and which are shaded by other parts of
    % the geometry. It uses a specified shading algorithm (e.g., Binary or
    % COP) to calculate a visibility factor for each triangle.
    %
    % ShadingPipeline methods:
    %   ShadingPipeline - Constructor to initialize the shading pipeline.
    %   shade           - Calculates visibility for all triangles.
    %   get_num_pixel   - The render resolution in use.
    %
    properties %(Access = private, Hidden = true)
        % store handle as int32 to match MexGateway expectations
        handle_ = int32(-1);
    end
    methods
        function this = ShadingPipeline(geometry, shading_algorithm, num_pixel)
            % SHADINGPIPELINE Constructor for ShadingPipeline.
            %
            %   obj = ShadingPipeline(geometry, shading_algorithm, num_pixel)
            %   initializes the pipeline for a specific geometry and algorithm.
            %
            %   Input Arguments:
            %       geometry         - A RotatableMeshGeometry object.
            %       shading_algorithm - An integer specifying the algorithm:
            %                           0 = Binary Shader (Simple on/off)
            %                           1 = COP Shader (test visibility of centroid)
            %       num_pixel         - The resolution (number of pixels)
            %                           used for the visibility analysis.
            %                           Optional: if omitted, it is chosen from
            %                           the mesh so that each triangle spans about
            %                           7 pixels for CoP, where more pixels keep
            %                           improving accuracy, or 3 for Binary, where
            %                           they do not (see get_num_pixel).
            %                           Build the pipeline in the geometry's most
            %                           extended pose, since num_pixel is fixed
            %                           here.
            %
            arguments
                geometry (1,1) vat.geometry.RotatableMeshGeometry
                shading_algorithm (1,1) {mustBeInteger, mustBeMember(shading_algorithm, [0, 1])} = 0
                num_pixel double {mustBeScalarOrEmpty, mustBeInteger, mustBePositive} = []
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            try
                if isempty(num_pixel)
                    this.handle_ = MexGateway("shading.ShadingPipeline.new", int32(geometry.handle_), int32(shading_algorithm));
                else
                    this.handle_ = MexGateway("shading.ShadingPipeline.new", int32(geometry.handle_), int32(shading_algorithm), int32(num_pixel));
                end
            catch ME
                error("Failed to create Shading pipeline: %s", ME.message);
            end
        end
        
        function delete(this)
            % DELETE Destructor for ShadingPipeline.
            %
            %   Releases the underlying C++ pipeline object.
            %
            arguments
                this (1,1) vat.shading.ShadingPipeline
            end
            if this.handle_ ~= int32(-1)
                MexGateway("shading.ShadingPipeline.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end
        
        function visibility = shade(this,velocity)
            % SHADE Calculates the visibility factor for each geometry triangle.
            %
            %   visibility = shade(this, velocity) returns a vector containing
            %   the exposure value (shading) for each triangle in the mesh
            %   relative to the incoming flow direction.
            %
            %   Input Arguments:
            %       velocity   - The relative velocity vector [x, y, z] in the 
            %                    geometry body frame.
            %
            %   Output Arguments:
            %       visibility - A vector of visibility factors (1.0 = exposed, 
            %                    0.0 = shaded).
            %
            arguments
                this (1,1) vat.shading.ShadingPipeline
                velocity (1,3) double
            end
            visibility = MexGateway("shading.ShadingPipeline.shade", int32(this.handle_), velocity);
        end

        function num_pixel = get_num_pixel(this)
            % GET_NUM_PIXEL The render resolution this pipeline uses.
            %
            %   num_pixel = get_num_pixel(this) returns the value given to the
            %   constructor, or the one chosen from the mesh if it was omitted.
            %
            arguments
                this (1,1) vat.shading.ShadingPipeline
            end
            num_pixel = MexGateway("shading.ShadingPipeline.get_num_pixel", int32(this.handle_));
        end
    end
end
