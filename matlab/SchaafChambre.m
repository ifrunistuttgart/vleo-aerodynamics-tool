classdef SchaafChambre < handle
    % SCHAAF_CHAMBRE schaaf_chambre's Gas-Surface Interaction (GSI) model.
    %
    % This class implements the schaaf_chambre model for calculating aerodynamic 
    % forces and torques on surface elements.
    %
    % schaaf_chambre methods:
    %   schaaf_chambre                - Constructor for the schaaf_chambre model.
    %   calc_aero_force_torque - Calculates loads for a single surface element.
    %
    properties %(Access = private, Hidden = true)
        handle_ = int32(-1);
    end
    
    methods
        function this = SchaafChambre(sigma_n, sigma_t)
            % SCHAAF_CHAMBRE Constructor for the schaaf_chambre model.
            %
            %   obj = schaaf_chambre() initializes the model.
            %
            arguments
                sigma_n (1,1) single
                sigma_t (1,1) single
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("SchaafChambre.new", sigma_n, sigma_t));
        end
        function delete(this)
            % DELETE Destructor for schaaf_chambre model.
            %
            %   Releases the underlying C++ model object.
            %
            arguments
                this (1,1) SchaafChambre
            end
            if this.handle_ ~= int32(-1)
                MexGateway("SchaafChambre.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end
        
        function [force__N, torque__Nm] = calc_aero_force_torque(this, area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K, aero_cond)
            % CALC_AERO_FORCE_TORQUE Calculates aerodynamic loads for a surface element.
            %
            %   [force__N, torque__Nm] = calc_aero_force_torque(this, area, 
            %   normal, centroid, v_rel, surf_temp, aero_cond) computes the 
            %   local force and torque vectors based on schaaf_chambre's equations.
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
                this (1,1) SchaafChambre
                area__m2 (1,1) double
                normal (1,3) double
                centroid_m (1,3) double
                v_rel__m_per_s (1,3) double
                surf_temp__K (1,1) double
                aero_cond (1,1) AeroConditions
            end
            [force__N, torque__Nm] = MexGateway("SchaafChambre.calc_aero_force_torque", int32(this.handle_), area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K, int32(aero_cond.handle_));
        end
        function sigma_n = get_sigma_n(this)
            % GET_SIGMA_N Returns the value of sigma_n.
            %
            %   sigma_n = get_sigma_n(this) retrieves the sigma_n parameter from
            %   the underlying schaaf_chambre model.
            %
            arguments
                this (1,1) SchaafChambre
            end
            sigma_n = MexGateway("SchaafChambre.get_gsi_parameter", int32(this.handle_), "sigma_n");
        end
        function set_sigma_n(this, sigma_n)
            % SET_SIGMA_N Sets the value of sigma_n.
            %
            %   set_sigma_n(this, sigma_n) sets the sigma_n parameter in
            %   the underlying schaaf_chambre model.
            %
            arguments
                this (1,1) SchaafChambre
                sigma_n (1,1) single
            end
            MexGateway("SchaafChambre.set_gsi_parameter", int32(this.handle_), "sigma_n", sigma_n);
        end
        function sigma_t = get_sigma_t(this)
            % GET_SIGMA_T Returns the value of sigma_t.
            %
            %   sigma_t = get_sigma_t(this) retrieves the sigma_t parameter from
            %   the underlying schaaf_chambre model.
            %
            arguments
                this (1,1) SchaafChambre
            end
            sigma_t = MexGateway("SchaafChambre.get_gsi_parameter", int32(this.handle_), "sigma_t");
        end
        function set_sigma_t(this, sigma_t)
            % SET_SIGMA_T Sets the value of sigma_t.
            %
            %   set_sigma_t(this, sigma_t) sets the sigma_t parameter in
            %   the underlying schaaf_chambre model.
            %
            arguments
                this (1,1) SchaafChambre
                sigma_t (1,1) single
            end
            MexGateway("SchaafChambre.set_gsi_parameter", int32(this.handle_), "sigma_t", sigma_t);
        end
    end
end