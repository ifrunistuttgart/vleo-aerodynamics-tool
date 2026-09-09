classdef HybridForceTorqueCalculator < handle
    % HYBRIDFORCETORQUECALCULATOR Calculates aerodynamic loads on a geometry.
    %
    % This class provides a high-level interface for calculating the total 
    % aerodynamic force and torque acting on a geometry. It combines a 
    % geometry, a shading pipeline to handle self-shadowing, and a 
    % Gas-Surface Interaction (GSI) model for surface-level physics.
    %
    % HybridForceTorqueCalculator methods:
    %   HybridForceTorqueCalculator - Constructor for the calculator.
    %   calc_aero_load           - Calculates total force and torque.
    %
    properties %(Access = private, Hidden = true)
        handle_ = int32(-1);
    end

    methods
        function this = HybridForceTorqueCalculator(geometry, shading_pipeline, gsi_model)
            % HYBRIDFORCETORQUECALCULATOR Constructor for HybridForceTorqueCalculator.
            %
            %   obj = HybridForceTorqueCalculator(geometry, shading_pipeline, gsi_model)
            %   initializes the calculator with the necessary components.
            %
            %   Input Arguments:
            %       geometry        - A RotatableMeshGeometry object representing the geometry.
            %       shading_pipeline - A ShadingPipeline object for visibility analysis.
            %       gsi_model        - A GSI model object (e.g., Sentman) for load calculation.
            %
            arguments
                geometry (1,1) vat.geometry.RotatableMeshGeometry
                shading_pipeline (1,1) vat.shading.ShadingPipeline
                gsi_model (1,1) handle
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("loads.HybridForceTorqueCalculator.new", int32(geometry.handle_), int32(shading_pipeline.handle_), int32(gsi_model.handle_)));
        end

        function [force__N, torque__Nm] = calc_aero_load(this, v_rel__m_per_s,surface_temp__K, aero_conditions)
            % CALC_AERO_LOAD Calculates the total aerodynamic force and torque.
            %
            %   [force__N, torque__Nm] = calc_aero_load(this, v_rel, surface_temp, aero_conditions)
            %   performs the complete aerodynamic analysis, including shading and
            %   surface interactions, to compute the resulting loads.
            %
            %   Input Arguments:
            %       v_rel__m_per_s   - Relative velocity vector in geometry body frame [m/s].
            %       surface_temp__K  - Uniform surface temperature of the geometry [K].
            %       aero_conditions  - An AeroConditions object containing atmospheric properties.
            %
            %   Output Arguments:
            %       force__N         - Total aerodynamic force vector in body frame [N].
            %       torque__Nm       - Total aerodynamic torque vector about the origin [Nm].
            %
            arguments
                this (1,1) vat.loads.HybridForceTorqueCalculator
                v_rel__m_per_s (1,3) double
                surface_temp__K (1,1) double {mustBePositive}
                aero_conditions (1,1) vat.AeroConditions
            end
            [force__N, torque__Nm] = MexGateway("loads.HybridForceTorqueCalculator.calc_aero_load", int32(this.handle_), v_rel__m_per_s, surface_temp__K, int32(aero_conditions.handle_));
        end

        function delete(this)
            % DELETE Destructor for HybridForceTorqueCalculator.
            %
            %   Releases the underlying C++ resources.
            %
            arguments
                this (1,1) vat.loads.HybridForceTorqueCalculator
            end
            if this.handle_ ~= int32(-1)
                MexGateway("loads.HybridForceTorqueCalculator.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end
    end
end