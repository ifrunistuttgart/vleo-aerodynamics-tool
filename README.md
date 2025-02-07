# Readme: VLEO Aerodynamic Tool

 
## Setting up the developement environment for the VLEO Aerodynamics Tool
### Prerequisites 
- Visual Studio Code (VSCode) installed
- Git installed (recommended: Git Bash as the terminal)
- GitHub Access

### Cloning the repository

1. **Start VSCode** and navigate to your desired working directory
2. **Open a new terminal** (recommended: Git Bash)
3. **Clone the repository from GitHub** by opening the GitHub project page https://github.com/ifrunistuttgart/vleo-aerodynamics-tool, copying the HTTPS-URL of the repository and clone into the repository by running the following command in the terminal 
````bash
git clone <copied HTTPS-URL>
````
4. **Navigate into the cloned directory:**
````bash
 cd vleo-aerodynamoics-tool
 ````
### Opening the project in VSCode

To open the project directory direclty in VSCode, run:
````bash
code .
````

### Initializing submodules
The VLEO Aerodynamics Tool integrates two submodules that implement core functionalities. 

**These need to be initialized and updated before use**:
````bash
git submodule init
git submodule update --recursive
````

### Running the project
Once the setup is complete, you should be able to run the example code and utilize the main functions provided by the submodules.
For a QuickStart-Guide consisting of a step-by-step example go to [HowTo.md](HowTo.md)

## Important functions

### Import CAD-Files with importMultipleBodies.m

After creating your satellite configuration in your preferred CAD-Tool, you can export each part **individually** as an .obj-file. These objects will be used in the vleo-aerodynamic-tool submodule to accordingly import and handle the satellite components in your simulation.
**Caution with the order of your imports**. 

The function ``importMultipleBodies`` prepares the .obj-files for the VLEO aerodynamics simulation. The function returns a cell array of structures containing the vertices, surface centroids, surface normals, rotation direction, rotation hinge points, surface temperatures, surface energy accommodation coefficients, and surface areas of the bodies

Input arguments:

- object_files: 1xN array of strings of the paths to the .obj files

- rotation_hinge_points_CAD: 3xN array of the rotation hinge points of the bodies in the CAD frame

- rotation_directions_CAD: 3xN array of the rotation directions of the bodies in the CAD frame

- temperatures__K: 1xN cell array of the surface temperatures of the bodies

- energy_accommodation_coefficients: 1xN cell array of the surface energy accommodation coefficients of the bodies

- DCM_B_from_CAD: 3x3 array of the direction cosine matrix from the CAD frame to the body frame

- CoM_CAD: 3x1 array of the center of mass of the bodies in the CAD frame

Output: 

- bodies: 1xN cell array of structures containing the vertices, surface centroids, surface normals, rotation direction, rotation hinge point, surface temperatures, surface energy accommodation coefficients, and surface areas of the bodies

### Visualize your satellite configuration with showBodies.m
The ``showBodies``-function plots the bodies and their surface normals rotated by the given angles. The bodies are plotted in a 3D figure with the surface centroids and normals. In addition,scalar and vectorial values can be given to be plotted on the surfaces. Scalar values are plotted as surface colors and vectorial values are plotted as vectors.

Input arguments:

- bodies: 1xN cell array of structures containing the vertices, surface centroids, surface normals, rotation direction, rotation hinge point

- bodies_rotation_angles__rad: 1xN array of the rotation angles of the bodies

- face_alpha: scalar, the transparency of the faces

- scale_normals: scalar, the scale factor for the normals

- scalar_values: 1xN cell array of scalar values to be plotted on the surfaces

- vectorial_values: 1xN cell array of vectorial values to be plotted on the surfaces

- scale_vectorial_values: scalar, the scale factor for the vectorial values

If you have problems with the ``showBodies``-function, check the dimensions of your imported control surfaces and compare them to the ones in the example. **This simulation can handle a variety of satellite configurations, but if your CAD satellite isn´t imported part by part, errors will occur**

Also remember to scale your ``rotation_directions_CAD`` and ``rotation_hinge_points_CAD`` matrices if the dimensions of your satellite don't fit.

### Calculate torques and forces with vleoAerodynamics.m

The ``vleoAerodynamics``-function calculates the aerodynamic forces and torque actiong on a satellite in VLEO.

**This function ecpects the space_math_utilities namespace to be available on the MATLAB path.**

Input arguments:

- attitude_quaternion_BI: 4x1 array of the attitude quaternion from the body frame to the inertial frame

- rotational_velocity_BI_B__rad_per_s: 3x1 array of the rotational velocity of the satellite with respect to the inertial frame expressed in the body frame

- velocity_I_I__m_per_s: 3x1 array of the velocity of the satellite with respect to the inertial frame expressed in the inertial frame

- wind_velocity_I_I__m_per_s: 3x1 array of the velocity of the wind with respect to the inertial frame expressed in the inertial frame

- density__kg_per_m3: Scalar value of the density of the gas

- temperature__K: Scalar value of the temperature of the gas

- particles_mass__kg: Scalar value of the mass of the particles

- bodies: 1xN cell array of structures containing the vertices, surface centroids, surface normals, rotation direction, rotation hinge point, surface temperatures, surface energy accommodation coefficients, and surface areas of the bodies

- bodies_rotation_angles__rad: 1xN array of the rotation angles of the bodies

- temperature_ratio_method: Scalar value of the method to calculate the temperature ratio
- - 1: Exact term 
- - 2: Hyperthermal approximation 1 
- - 3: Hyperthermal approximation 2

Outputs:

- aerodynamic_force_B__N: 3x1 array of the aerodynamic force acting on the satellite expressed in the body frame

- aerodynamic_torque_B_B__Nm: 3x1 array of the aerodynamic torque acting on the satellite with respect to the center of mass (origin of body frame) expressed in the body frame

