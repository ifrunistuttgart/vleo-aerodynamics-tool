classdef Sentman < handle
    % SENTMAN Sentman's Gas-Surface Interaction (GSI) model.
    %
    % This class implements the Sentman model for calculating aerodynamic 
    % forces and torques on surface elements. It considers the diffuse 
    % reflection of particles and the energy accommodation between the flow 
    % and the satellite surface.
    %
    % Sentman methods:
    %   Sentman                - Constructor for the Sentman model.
    %   calc_aero_force_torque - Calculates loads for a single surface element.
    %
    properties %(Access = private, Hidden = true)
        handle_ = int32(-1);
    end
    
    methods
        function this = Sentman(temperature_ratio_method,alpha_e)
            % SENTMAN Constructor for the Sentman model.
            %
            %   obj = Sentman(temperature_ratio_method) initializes the model.
            %
            %   Input Arguments:
            %       temperature_ratio_method - An integer flag specifying the 
            %           method used for calculating the temperature ratio 
            %           between the surface and the incoming flow.
            %       alpha_e - The accommodation coefficient for energy exchange.
            %
            arguments
                temperature_ratio_method (1,1) int32
                alpha_e (1,1) single
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("Sentman.new", int32(temperature_ratio_method), alpha_e));
        end
        function delete(this)
            % DELETE Destructor for Sentman model.
            %
            %   Releases the underlying C++ model object.
            %
            arguments
                this (1,1) Sentman
            end
            if this.handle_ ~= int32(-1)
                MexGateway("Sentman.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end
        
        function [force__N, torque__Nm] = calc_aero_force_torque(this, area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K, aero_cond)
            % CALC_AERO_FORCE_TORQUE Calculates aerodynamic loads for a surface element.
            %
            %   [force__N, torque__Nm] = calc_aero_force_torque(this, area, 
            %   normal, centroid, v_rel, surf_temp, aero_cond) computes the 
            %   local force and torque vectors based on Sentman's equations.
            %
            %   Input Arguments:
            %       area__m2       - Surface area of the element [m^2].
            %       normal         - Unit normal vector of the surface element.
            %       centroid_m     - 3D centroid position of the element [m].
            %       v_rel__m_per_s - Relative velocity vector of the flow [m/s].
            %       surf_temp__K   - Temperature of the satellite surface [K].
            %       aero_cond      - An AeroConditions object with atmospheric data.
            %
            %   Output Arguments:
            %       force__N       - Resulting aerodynamic force vector [N].
            %       torque__Nm     - Resulting aerodynamic torque vector [Nm].
            %
            arguments
                this (1,1) Sentman
                area__m2 (1,1) double
                normal (1,3) double
                centroid_m (1,3) double
                v_rel__m_per_s (1,3) double
                surf_temp__K (1,1) double
                aero_cond (1,1) AeroConditions
            end
            [force__N, torque__Nm] = MexGateway("Sentman.calc_aero_force_torque", int32(this.handle_), area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K, int32(aero_cond.handle_));
        end
        function alpha_e = get_alpha_e(this)
            % GET_ALPHA_E Returns the value of alpha_e.
            %
            %   alpha_e = get_alpha_e(this) retrieves the alpha_e parameter from
            %   the underlying sentman model.
            %
            arguments
                this (1,1) Sentman
            end
            alpha_e = MexGateway("Sentman.get_gsi_parameter", int32(this.handle_), "alpha_e");
        end
        function set_alpha_e(this, alpha_e)
            % SET_ALPHA_E Sets the value of alpha_e.
            %
            %   set_alpha_e(this, alpha_e) sets the alpha_e parameter in
            %   the underlying sentman model.
            %
            arguments
                this (1,1) Sentman
                alpha_e (1,1) single
            end
            MexGateway("Sentman.set_gsi_parameter", int32(this.handle_), "alpha_e", alpha_e);
        end
    end
end