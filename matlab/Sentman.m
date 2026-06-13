classdef Sentman < handle
    properties %(Access = private, Hidden = true)
        handle_ = -1;
    end
    
    methods
        function this = Sentman(temperature_ratio_method)
            assert(this.handle_ == -1, "This object is already constructed.");
            this.handle_ = MexGateway("Sentman.new", int32(temperature_ratio_method));
        end
        function delete(this)
            MexGateway("Sentman.delete", this.handle_);
            this.handle_ = -1;
        end
        
        function [force__N, torque__Nm] = calc_aero_force_torque(this, area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K, aero_cond)
            [force__N, torque__Nm] = MexGateway("Sentman.calc_aero_force_torque", this.handle_, area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K,aero_cond.handle_);
        end
    end
end