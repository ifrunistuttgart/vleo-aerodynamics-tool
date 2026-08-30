classdef Storch < handle
    % STORCH Storch's gas-surface interaction (GSI) model.
    properties %(Access = private, Hidden = true)
        handle_ = int32(-1);
    end

    methods
        function this = Storch(V_w, sigma_n, sigma_t)
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("Storch.new", V_w, sigma_n, sigma_t));
        end

        function delete(this)
            if this.handle_ ~= int32(-1)
                MexGateway("Storch.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end

        function [force__N, torque__Nm] = calc_aero_force_torque(this, area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K, aero_cond)
            [force__N, torque__Nm] = MexGateway("Storch.calc_aero_force_torque", int32(this.handle_), area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K, int32(aero_cond.handle_));
        end

        function V_w = get_V_w(this)
            V_w = MexGateway("Storch.get_gsi_parameter", int32(this.handle_), "V_w");
        end

        function sigma_n = get_sigma_n(this)
            sigma_n = MexGateway("Storch.get_gsi_parameter", int32(this.handle_), "sigma_n");
        end

        function sigma_t = get_sigma_t(this)
            sigma_t = MexGateway("Storch.get_gsi_parameter", int32(this.handle_), "sigma_t");
        end

        function set_V_w(this, V_w)
            MexGateway("Storch.set_gsi_parameter", int32(this.handle_), "V_w", V_w);
        end

        function set_sigma_n(this, sigma_n)
            MexGateway("Storch.set_gsi_parameter", int32(this.handle_), "sigma_n", sigma_n);
        end

        function set_sigma_t(this, sigma_t)
            MexGateway("Storch.set_gsi_parameter", int32(this.handle_), "sigma_t", sigma_t);
        end
    end
end