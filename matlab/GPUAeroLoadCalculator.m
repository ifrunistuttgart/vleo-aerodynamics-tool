classdef GPUAeroLoadCalculator < handle
    % GPUAEROLOADCALCULATOR Calculates aerodynamic loads on a satellite using the GPU pipeline.
    %
    % This class combines a satellite model, a GPU GSI model, and an OpenGL
    % rendering pipeline to compute aerodynamic force and torque.
    %
    % GPUAeroLoadCalculator methods:
    %   GPUAeroLoadCalculator - Constructor for the calculator.
    %   calc_aero_torque_force - Calculates total force and torque.
    %   delete                - Releases the underlying C++ resources.

    properties %(Access = private, Hidden = true)
        handle_ = int32(-1);
    end

    methods
        function this = GPUAeroLoadCalculator(satellite, gsi_model, num_pixel)
            % GPUAEROLOADCALCULATOR Constructor for GPUAeroLoadCalculator.
            %
            %   obj = GPUAeroLoadCalculator(satellite, gsi_model, num_pixel)
            %   initializes the calculator with the necessary components.
            %
            %   Input Arguments:
            %       satellite  - A RotatableMeshSatellite object.
            %       gsi_model  - A GPUNewton object.
            %       num_pixel  - The render resolution used for the GPU pass.
            %
            arguments
                satellite (1,1) RotatableMeshSatellite
                gsi_model (1,1) GPUNewton
                num_pixel (1,1) {mustBeInteger, mustBePositive} = 800
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("GPUAeroLoadCalculator.new", int32(satellite.handle_), int32(gsi_model.handle_), int32(num_pixel)));
        end

        function [force__N, torque__Nm] = calc_aero_torque_force(this, v_rel__m_per_s, surface_temp__K, aero_conditions)
            % CALC_AERO_TORQUE_FORCE Calculates the total aerodynamic force and torque.
            %
            %   [force__N, torque__Nm] = calc_aero_torque_force(this, v_rel,
            %   surface_temp, aero_conditions) computes the resulting force and
            %   torque using the GPU rendering pipeline.
            %
            %   Input Arguments:
            %       v_rel__m_per_s - Relative velocity vector in satellite body frame [m/s].
            %       surface_temp__K - Uniform satellite surface temperature [K].
            %       aero_conditions - An AeroConditions object containing atmospheric data.
            %
            %   Output Arguments:
            %       force__N       - Total aerodynamic force vector [N].
            %       torque__Nm     - Total aerodynamic torque vector [Nm].
            %
            arguments
                this (1,1) GPUAeroLoadCalculator
                v_rel__m_per_s (1,3) double
                surface_temp__K (1,1) double {mustBePositive}
                aero_conditions (1,1) AeroConditions
            end
            [force__N, torque__Nm] = MexGateway("GPUAeroLoadCalculator.calc_aero_torque_force", int32(this.handle_), v_rel__m_per_s, surface_temp__K, int32(aero_conditions.handle_));
        end

        function delete(this)
            % DELETE Destructor for GPUAeroLoadCalculator.
            arguments
                this (1,1) GPUAeroLoadCalculator
            end
            if this.handle_ ~= int32(-1)
                MexGateway("GPUAeroLoadCalculator.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end
    end
end

