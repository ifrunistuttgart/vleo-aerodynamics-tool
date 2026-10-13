classdef PixelForceTorqueCalculator < handle
    % PIXELFORCETORQUECALCULATOR Integrates aerodynamic loads per pixel on the GPU.
    %
    % Renders the geometry along the flow and evaluates the Gas-Surface
    % Interaction (GSI) model in every pixel the flow reaches, so partly
    % shaded triangles count with exactly their exposed part and the result
    % depends on the resolution only, not on how the geometry is meshed.
    % Surfaces facing away from the flow are evaluated per triangle, without
    % shadowing, as in HybridForceTorqueCalculator. Needs no ShadingPipeline.
    %
    % PixelForceTorqueCalculator methods:
    %   PixelForceTorqueCalculator - Constructor for the calculator.
    %   calc_aero_load             - Calculates total force and torque.
    %   pressure_image             - Per-pixel pressure of the last evaluation.
    %   last_areas                 - Wetted and projected area of the last evaluation.
    %
    properties %(Access = private, Hidden = true)
        handle_ = int32(-1);
    end
    properties (Access = private)
        % The C++ calculator refers to these; holding them keeps them alive.
        geometry_
        gsi_model_
    end

    methods
        function this = PixelForceTorqueCalculator(geometry, gsi_model, num_pixel, options)
            % PIXELFORCETORQUECALCULATOR Constructor for PixelForceTorqueCalculator.
            %
            %   obj = PixelForceTorqueCalculator(geometry, gsi_model, num_pixel)
            %   obj = PixelForceTorqueCalculator(..., MinCosDelta=1e-3, KeepPressureImage=true)
            %
            %   Input Arguments:
            %       geometry          - A RotatableMeshGeometry object representing the geometry.
            %       gsi_model         - A GSI model object (e.g., Sentman) for load calculation.
            %       num_pixel         - Edge length of the square render target, the
            %                           accuracy/runtime knob. One pixel covers
            %                           (2R/num_pixel)^2, R the bounding sphere radius.
            %       MinCosDelta       - Lower bound for cos(delta) of surfaces seen almost
            %                           edge-on (default 1e-3).
            %       KeepPressureImage - Read back the per-pixel pressure after every
            %                           evaluation, see pressure_image (default false).
            %
            arguments
                geometry (1,1) vat.geometry.RotatableMeshGeometry
                gsi_model (1,1) handle
                num_pixel (1,1) {mustBeInteger, mustBePositive} = 1000
                options.MinCosDelta (1,1) double {mustBePositive, mustBeLessThanOrEqual(options.MinCosDelta, 1)} = 1e-3
                options.KeepPressureImage (1,1) logical = false
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("loads.PixelForceTorqueCalculator.new", ...
                int32(geometry.handle_), int32(gsi_model.handle_), int32(num_pixel), ...
                double(options.MinCosDelta), int32(options.KeepPressureImage)));
            this.geometry_ = geometry;
            this.gsi_model_ = gsi_model;
        end

        function [force__N, torque__Nm] = calc_aero_load(this, v_rel__m_per_s, surface_temp__K, aero_conditions)
            % CALC_AERO_LOAD Calculates the total aerodynamic force and torque.
            %
            %   [force__N, torque__Nm] = calc_aero_load(this, v_rel, surface_temp, aero_conditions)
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
                this (1,1) vat.loads.PixelForceTorqueCalculator
                v_rel__m_per_s (1,3) double
                surface_temp__K (1,1) double {mustBePositive}
                aero_conditions (1,1) vat.AeroConditions
            end
            [force__N, torque__Nm] = MexGateway("loads.PixelForceTorqueCalculator.calc_aero_load", ...
                int32(this.handle_), v_rel__m_per_s, surface_temp__K, int32(aero_conditions.handle_));
        end

        function pressure__N_per_m2 = pressure_image(this)
            % PRESSURE_IMAGE Per-pixel pressure of the last evaluation [N/m^2].
            %
            %   p = pressure_image(this) returns a num_pixel x num_pixel matrix
            %   seen from upstream, looking along -v_rel, top row first, ready for
            %   imagesc. Each entry is the normal component of the force per
            %   wetted area; pixels without a windward surface are zero. Empty
            %   unless the calculator was built with KeepPressureImage=true.
            %
            arguments
                this (1,1) vat.loads.PixelForceTorqueCalculator
            end
            pressure__N_per_m2 = MexGateway("loads.PixelForceTorqueCalculator.get_pressure_image", int32(this.handle_));
        end

        function [wetted__m2, projected__m2] = last_areas(this)
            % LAST_AREAS Areas the last evaluation integrated over [m^2].
            %
            %   [wetted, projected] = last_areas(this)
            %       wetted    - Surface area of the windward surfaces the flow reaches.
            %       projected - The same surfaces projected normal to the flow.
            %
            arguments
                this (1,1) vat.loads.PixelForceTorqueCalculator
            end
            [wetted__m2, projected__m2] = MexGateway("loads.PixelForceTorqueCalculator.get_last_areas", int32(this.handle_));
        end

        function delete(this)
            % DELETE Destructor for PixelForceTorqueCalculator.
            %
            %   Releases the underlying C++ resources.
            %
            arguments
                this (1,1) vat.loads.PixelForceTorqueCalculator
            end
            if this.handle_ ~= int32(-1)
                MexGateway("loads.PixelForceTorqueCalculator.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end
    end
end
