classdef AeroConditions < handle
    properties %(Access = private, Hidden = true)
        handle_ = -1;
    end

    methods
        function this = AeroConditions(density__kg_per_m3,...
                            temperature__K,...
                            particle_mass__kg,...
                            alpha_e)
            assert(this.handle_ == -1, "This object is already constructed.");
            this.handle_ = MexGateway("AeroCond.new", density__kg_per_m3,temperature__K,particle_mass__kg, alpha_e);
        end
        function delete(this)
            MexGateway("AeroCond.delete", this.handle_);
            this.handle_ = -1;
        end
    end
end