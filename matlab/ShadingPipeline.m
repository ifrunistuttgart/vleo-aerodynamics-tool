classdef ShadingPipeline < handle
    properties %(Access = private, Hidden = true)
        handle_ = -1;
    end
    methods
        function this = ShadingPipeline(satellite, num_pixel)
            assert(this.handle_ == -1, "This object is already constructed.");
            this.handle_ = MexGateway("Shading.new",satellite.handle_, int32(num_pixel));
        end
        
        function delete(this)
            MexGateway("Shading.delete", this.handle_);
            this.handle_ = -1;
        end
        
        function visibility = shade(this,velocity)
            visibility = MexGateway("Shading.shade", this.handle_, velocity);
        end
    end
end
