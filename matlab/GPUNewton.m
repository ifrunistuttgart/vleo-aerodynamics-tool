classdef GPUNewton < handle
    % GPUNEWTON GPU Newton gas-surface interaction model.
    %
    % This class provides an opaque MATLAB handle for the GPU Newton model
    % used by GPUAeroLoadCalculator.
    %
    % GPUNewton methods:
    %   GPUNewton - Constructor for the GPU Newton model.
    %   delete    - Releases the underlying C++ model object.

    properties %(Access = private, Hidden = true)
        handle_ = int32(-1);
    end

    methods
        function this = GPUNewton()
            % GPUNEWTON Constructor for the GPU Newton model.
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("GPUNewton.new"));
        end

        function value = get_gsi_parameter(this, name)
            % GET_GSI_PARAMETER Returns the value of a GPU Newton GSI parameter.
            arguments
                this (1,1) GPUNewton
                name (1,1) string
            end
            value = MexGateway("GPUNewton.get_gsi_parameter", int32(this.handle_), name);
        end

        function set_gsi_parameter(this, name, value)
            % SET_GSI_PARAMETER Sets a GPU Newton GSI parameter.
            arguments
                this (1,1) GPUNewton
                name (1,1) string
                value (1,1) single
            end
            MexGateway("GPUNewton.set_gsi_parameter", int32(this.handle_), name, value);
        end

        function delete(this)
            % DELETE Destructor for GPUNewton.
            arguments
                this (1,1) GPUNewton
            end
            if this.handle_ ~= int32(-1)
                MexGateway("GPUNewton.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end
    end
end


