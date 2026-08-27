classdef AeroConditions < handle
    % AEROCONDITIONS Atmospheric and interaction properties for aerodynamic load calculations.
    %
    % This class stores the environmental parameters required by Gas-Surface 
    % Interaction (GSI) models to calculate forces and torques. It includes
    % atmospheric density, temperature, and particle properties.
    %
    % AeroConditions methods:
    %   AeroConditions - Constructor for the atmospheric conditions.
    %
    properties %(Access = private, Hidden = true)
        handle_ = int32(-1);
    end

    methods
        function this = AeroConditions(density__kg_per_m3,...
                            T_atmospheric__K,...
                            particle_mass__kg)
            % AEROCONDITIONS Constructor for AeroConditions.
            %
            %   obj = AeroConditions(density, temperature, mass, alpha_e) creates an
            %   AeroConditions object with the specified atmospheric parameters.
            %
            %   Input Arguments:
            %       density__kg_per_m3 - Atmospheric density [kg/m^3].
            %       T_atmospheric__K     - Atmospheric temperature [K].
            %       particle_mass__kg  - Mean particle mass of the atmosphere [kg].
            %       alpha_e            - Energy accommodation coefficient (usually 0.0 to 1.0).
            %
            %
            arguments
                density__kg_per_m3 (1,1) double
                T_atmospheric__K (1,1) double
                particle_mass__kg (1,1) double
            end
            assert(this.handle_ == int32(-1), "This object is already constructed.");
            this.handle_ = int32(MexGateway("AeroCond.new", density__kg_per_m3, T_atmospheric__K, particle_mass__kg));
        end
        function this = setDensity(density__kg_per_m3)
            % SETDENSITY Sets the atmospheric density.
            %
            %   obj.setDensity(density) sets the atmospheric density [kg/m^3].
            %
            MexGateway("AeroCond.set_", int32(this.handle_), density__kg_per_m3);
        end
        function this = setTatmospheric(this, T_atmospheric__K)
            % SETTATMOSPHERIC Sets the atmospheric temperature.
            %
            %   obj.setTatmospheric(temperature) sets the atmospheric temperature [K].
            %
            MexGateway("AeroCond.set_T_atmospheric", int32(this.handle_), T_atmospheric__K);
        end
        function this = setParticleMass(this, particle_mass__kg)
            % SETPARTICLEMASS Sets the mean particle mass of the atmosphere.
            %
            %   obj.setParticleMass(mass) sets the mean particle mass [kg].
            %
            MexGateway("AeroCond.set_particle_mass", int32(this.handle_), particle_mass__kg);
        end
        function delete(this)
            % DELETE Destructor for AeroConditions.
            %
            %   Releases the underlying C++ AeroConditions object.
            %
            if this.handle_ ~= int32(-1)
                MexGateway("AeroCond.delete", int32(this.handle_));
                this.handle_ = int32(-1);
            end
        end
    end
end