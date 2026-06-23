classdef HybridAeroLoadCalculator < handle
    properties %(Access = private, Hidden = true)
        handle_ = -1;
    end

    methods
        function this = HybridAeroLoadCalculator(satellite, shading_pipeline, gsi_model)
            assert(this.handle_ == -1, "This object is already constructed.");
            this.handle_ = MexGateway("HybridAeroLoadCalculator.new", satellite.handle_, shading_pipeline.handle_, gsi_model.handle_);
        end
        function [force__N, torque__Nm] = calc_aero_load(this, v_rel__m_per_s,surface_temp__K, aero_conditions)
            [force__N, torque__Nm] = MexGateway("HybridAeroLoadCalculator.calc_aero_load", this.handle_, v_rel__m_per_s, surface_temp__K, aero_conditions.handle_);
        end
        function delete(this)
            MexGateway("HybridAeroLoadCalculator.delete", this.handle_);
            this.handle_ = -1;
        end
    end
end