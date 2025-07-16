
# Readme: Satellite Design in Gmsh

The VLEO Aerodynamics Tool allows us to compute the forces and torques acting on satellites and helps us analyze their characteristics and behavior in scenarios where aerodynamic effects are the primary source of disturbance.

To support this, we need the flexibility to design, adapt, and reproduce various satellite configurations quickly and efficiently.

Gmsh is a finite element mesh generator that allows you to either import existing CAD files or design your own geometry directly within the program. Additionally, it provides powerful utilities for complex mesh processing, which are essential for applications such as aerodynamic shadowing.

---

## Installation

1. Go to the [official Gmsh website](https://gmsh.info)
2. Click *Download* and choose your installer
3. Run the installer (`.exe`) and follow the instructions
4. Optional: Add Gmsh to your system's PATH if you want to run it from the command line
5. Launch Gmsh via the desktop shortcut or from the terminal

---

## Satellite Design

Typically, Gmsh is used by importing a CAD file, followed by the meshing process (see below).  
However, this project follows a different approach: designing the satellite geometry directly within Gmsh using its scripting capabilities. This method allows for quick prototyping and full control over the geometry.

Here is how to get started:

### 1. Open Gmsh and Create a New File

- Launch Gmsh.
- Go to `File → New` to create a new project.
- Navigate to `Geometry → Edit Script` to open the code editor.

You can now describe your satellite geometry using basic shapes like boxes, spheres, and cylinders by specifying their dimensions and positions.

### 2. Example Geometry Script

Below is a minimal example that defines a satellite consisting of a main body and four solar panel-like wings:

```c
SetFactory("OpenCASCADE");

// === Main Body ===
Box(1) = {-0.05, -0.05, 0, 0.1, 0.1, 0.2};

// === Wings ===
// Right (+X)
Box(2) = {0.05, -0.049, 0, -0.001, 0.098, -0.133333};
// Top (+Y)
Box(3) = {-0.049, 0.05, 0, 0.098, -0.001, -0.133333};
// Left (-X)
Box(4) = {-0.049, -0.049, 0, -0.001, 0.098, -0.133333};
// Bottom (-Y)
Box(5) = {-0.049, -0.049, 0, 0.098, -0.001, -0.133333};
```

### 3. Define Physical Groups

Physical groups allow you to assign names to surfaces or volumes, which makes it easier to select or export specific regions (e.g., in MATLAB):

```c
Physical Surface("Body")        = {1, 2, 3, 4, 5, 6};
Physical Surface("WingRight")   = {8};
Physical Surface("WingTop")     = {16};
Physical Surface("WingLeft")    = {19};
Physical Surface("WingBottom")  = {27};
```

---

## Meshing

After defining your geometry, you can proceed with mesh generation. Gmsh gives you full control over mesh size and density through fields and conditions.

### 1. Define Mesh Refinement

You can refine the mesh around specific surfaces (e.g., solar panels) using distance fields and thresholds:

```c
Field[1] = Distance;
Field[1].SurfacesList = {5}; // Select a surface to refine
Field[1].NumPointsPerCurve = 100;

Field[2] = Threshold;
Field[2].InField = 1;
Field[2].SizeMin = 0.01;
Field[2].SizeMax = 0.05;
Field[2].DistMin = 0.001;
Field[2].DistMax = 0.05;

Field[3] = Restrict;
Field[3].IField = 2;
Field[3].SurfacesList = {8, 16, 19, 27}; // Apply to wing surfaces

// Apply background field for mesh sizing
Background Field = 3;
```

### 2. Set Mesh Characteristics

You can control global and local mesh density as follows:

```c
Mesh.CharacteristicLengthMax = 1.0;
Mesh.CharacteristicLengthMin = 0.001;

// Coarse mesh for the main body
Characteristic Length{ PointsOf{ Volume{1}; } } = 1.0;
```

### 3. Choose Export Format

Set the output format, e.g., for MATLAB compatibility:

```c
Mesh.Format = 39; // MATLAB format
```

### 4. Generate the Mesh

Finally, add this line to actually generate the 2D mesh:

```c
Mesh 2;
```

---
## Result
![satellite designed in gmsh](image.png)

## Export

Once the mesh has been generated, you can export it in various formats depending on the application:

- Go to `File → Export` or use the `Mesh.Format` option in the script.
- For MATLAB use, set `Mesh.Format = 39` in the script (as shown above).
- You can also export in formats like `.msh`, `.stl`, `.vtk`, or `.inp` for use in FEM solvers or post-processing tools.

Make sure to include any relevant *Physical Groups* so you can easily identify regions in downstream applications.

---

## Example: Use in MATLAB
