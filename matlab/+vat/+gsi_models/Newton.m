classdef Newton < handle
    % NEWTON newton's Gas-Surface Interaction (GSI) model.
    %
    % This class implements the newton model for calculating aerodynamic 
    % forces and torques on triangles.
    %
    % newton methods:
    %   newton                - Constructor for the newton model.
    %   calc_aero_force_torque - Calculates loads for a single triangle.
    %
    properties %(Access = private, Hidden = true)
        handle_ = int32(-1);
    end
    
    methods
        function this = Newton()
            % NEWTON Constructor for the newton model.
            %
            %   obj = newton() initializes the model.
            %
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("gsi_models.Newton.new"));
        end
        function delete(this)
            % DELETE Destructor for newton model.
            %
            %   Releases the underlying C++ model object.
            %
            arguments
                this (1,1) vat.gsi_models.Newton
            end
            if this.handle_ ~= int32(-1)
                MexGateway("gsi_models.Newton.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end
        
        function [force__N, torque__Nm] = calc_aero_force_torque(this, area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K, aero_cond)
            % CALC_AERO_FORCE_TORQUE Calculates aerodynamic loads for a triangle.
            %
            %   [force__N, torque__Nm] = calc_aero_force_torque(this, area, 
            %   normal, centroid, v_rel, surf_temp, aero_cond) computes the 
            %   local force and torque vectors based on newton's equations.
            %
            %   Input Arguments:
            %       area__m2       - Area of the triangle [m^2].
            %       normal         - Unit normal vector of the triangle.
            %       centroid_m     - 3D centroid position of the element [m].
            %       v_rel__m_per_s - Relative velocity vector of the flow [m/s].
            %       surf_temp__K   - Temperature of the geometry surface [K].
            %       aero_cond      - An AeroConditions object with atmospheric data.
            %
            %   Output Arguments:
            %       force__N       - Resulting aerodynamic force vector [N].
            %       torque__Nm     - Resulting aerodynamic torque vector [Nm].
            %
            arguments
                this (1,1) vat.gsi_models.Newton
                area__m2 (1,1) double
                normal (1,3) double
                centroid_m (1,3) double
                v_rel__m_per_s (1,3) double
                surf_temp__K (1,1) double
                aero_cond (1,1) vat.AeroConditions
            end
            [force__N, torque__Nm] = MexGateway("gsi_models.Newton.calc_aero_force_torque", int32(this.handle_), area__m2, normal, centroid_m, v_rel__m_per_s, surf_temp__K, int32(aero_cond.handle_));
        end
    end
end