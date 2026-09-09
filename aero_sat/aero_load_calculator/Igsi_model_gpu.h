#pragma once
#include <string>
#include "shader.h"

/**
 * Interface for Gas-Surface Interaction (GSI) models on GPU.
 *
 * This interface defines the contract for models that calculate aerodynamic
 * forces and torques acting on individual surface elements based on physical
 * interaction theories (e.g., Sentman, Maxwell, etc.) using GPU shaders.
 */
class IGSIModelGPU {
public:
    /**
     * Defines the vertex shader GLSL code that computes the pressure vector
     * by considering the surface normal, relative velocity, and other relevant parameters.
     *
     * @return GLSL code for a vertex shader
     */
    [[nodiscard]] virtual std::string get_vertex_shader_code() = 0;

    /**
     * Set the required uniforms for the Fragment shader defined by get_vertex_shader_code()
     *
     * @param shader Shader that results from the vertex shader above and a fragment shader
     */
    virtual void set_shader_uniforms(Shader* shader) = 0;
    virtual ~IGSIModelGPU() = default;
    /**
     * Sets a parameter for the GSI model.
     *
     * @param name The name of the parameter to set.
     * @param value The value to set the parameter to.
     */
    virtual void set_gsi_parameter(std::string name, float value) = 0;
    /**
     * Retrive the value of a specific gsi parameter
     *
     * @param name The name of the parameter
     * @return The value of the parameter
     */
    [[nodiscard]] virtual float get_gsi_parameter(std::string name) const = 0;
};
