# How to: VLEO Aerodynamic Tool

 
## Clone repository and necessary settings 
Start VSCode, navigate to the directory you want the work in and open a new terminal. I recommend using git bash.

 Go to Github and navigate to the vleo-aerodynamics-tool via  https://github.com/ifrunistuttgart/vleo-aerodynamics-tool  and copy the HTTPS-URL of the repository.

Go back to your terminal and type:
```bash
git clone https://github.com/ifrunistuttgart/vleo-aerodynamics-tool.git
```
You are now cloning in the repository.

Your should now be in the main branch and can open the folder and its files with
```bash
code .
```

Before you can start with coding and exploiting the given example you need to initialize the two integrated submodules that are implementing the main functions of the VLEO Aerodynamic Tool. Use: 
```bash
git submodule init 
git submodule update --recursive
```
After that you should be able to run the example code and exploit the main functions from the submodules.


## Import CAD-Files with importMultipleBodies.m

After crating your satellite configuration in your preferred CAD-Tool, you can export each part **individually** as an .obj-file. These objects will be used in the vleo-aerodynamic-tool submodule to accordingly import and handle the satellite components in your simulation.
**Caution with the order of your imports**. 

The function ``importMultipleBodies`` has the following output: 

- 1xN cell array of structures containing the vertices, surface centroids, surface normals, rotation direction, rotation hinge point, surface temperatures,surface energy accommodation coefficients, and surface areas of the bodies

There are several parameters to define as the functions input:

- object_files: 1xN array of strings of the paths to the .obj files

- rotation_hinge_points_CAD: 3xN array of the rotation hinge points of the bodies in the CAD frame

- rotation_directions_CAD: 3xN array of the rotation directions of the bodies in the CAD frame

- temperatures__K: 1xN cell array of the surface temperatures of the bodies

- energy_accommodation_coefficients: 1xN cell array of the surface energy accommodation coefficients of the bodies

- DCM_B_from_CAD: 3x3 array of the direction cosine matrix from the CAD frame to the body frame

- CoM_CAD: 3x1 array of the center of mass of the bodies in the CAD frame


For a more detailed description of these arguments refer to the [HowTo.md](HowTo.md).


## Visualize your satellite configuration with showBodies.m
This function plots the bodies and their surface normals rotated by the given angles. The bodies are plotted in a 3D figure with the surface centroids and normals. In addition,scalar and vectorial values can be given to be plotted on the surfaces. Scalar values are plotted as surface colors and vectorial values are plotted as vectors.

Input arguments:

- bodies: 1xN cell array of structures containing the vertices, surface centroids, surface normals, rotation direction, rotation hinge point
- bodies_rotation_angles__rad: 1xN array of the rotation angles of the bodies
- face_alpha: scalar, the transparency of the faces
- scale_normals: scalar, the scale factor for the normals
- scalar_values: 1xN cell array of scalar values to be plotted on the surfaces
- vectorial_values: 1xN cell array of vectorial values to be plotted on the surfaces
- scale_vectorial_values: scalar, the scale factor for the vectorial values

If you have problems with the ``showBodies.m``-function, check the dimensions of your imported control surfaces and compare them to the ones in the example. **This simulation can handle a variety of satellite configurations, but if your CAD satellite isn´t imported part by part, errors will occur**

Also remember to scale your ``rotation_directions_CAD`` and ``rotation_hinge_points_CAD`` matrices if the dimensions of your satellite don't fit.

Having a 30x30x100 milimeter satellite in CAD and rotation hinge points set at 3.75 as above, the visualization seems empty. Thats not because the showBodies.m-function doesn't work, its a dimensions problem.



### Notizen für mich

Der erste Versuch die bodies zu plotten wirft folgenden Fehler in der showBodies-Funktion: 
``Error using patch
Specify the coordinates as vectors or matrices of the same size, or as a vector and a matrix that share the same length in at least one
dimension.``

die bodies sind glaube ich irgendwie nicht richtig importiert worden, folgender Fehler, wenn ich nur bis importMultipleBodies: ``Error using vleo_aerodynamics_core.importMultipleBodies (line 40)
Invalid argument at position 1. These files do not exist:
'C:\Users\maier\AppData\Local\Temp\Editor_wksbg\obj_files_test\body.obj',
'C:\Users\maier\AppData\Local\Temp\Editor_wksbg\obj_files_test\right_control_surface.obj',
'C:\Users\maier\AppData\Local\Temp\Editor_wksbg\obj_files_test\bottom_control_surface.obj',
'C:\Users\maier\AppData\Local\Temp\Editor_wksbg\obj_files_test\left_control_surface.obj', and
'C:\Users\maier\AppData\Local\Temp\Editor_wksbg\obj_files_test\top_control_surface.obj'.`` 
obwohl ein 1x5 struct ausgegeben wird 

Korrektur dazu: Auch beim Origignalskript wird dieser Fehler geworfen, wenn man den Code Section für Section durchgeht. Dann sollte beim Importieren doch alles gut gelaufen sein und das Problem entsteht in 
```Matlab
showBodies.m
```
Versuch nur ein Part zu importieren wirft folgenden Fehler: 
``Error using vleo_aerodynamics_core.importMultipleBodies (line 41)
     rotation_hinge_points_CAD, ...
     ^^^^^^^^^^^^^^^^^^^^^^^^^
Invalid argument at position 2. Length of dimension 2 of first input must equal length of dimension 2 second input.``
, daher werden nun auch die rotation_hinge_points_CAD und rotations_directions_CAD usw. angepasst. Es zeigt sich, dass der selbe Fehler, das mit dem patch, wieder geworfen wird.

Ich beschftige mich nun erneut mit den CAD Files. 

Die Finnen in CAD wurden richtig positioniert. Es wird allerdings immer noch der Fehler geworfen.

Fehler liegt als wirklich in der Funktion showBodies. Die bodies werden nämlich mit der Funktion importMultipleBodies erstellt. ICh schaue mir die Dimensionen der bodies nun genauer an.

Fehler gefunden. Er liegt in der Dimension des main bodies. Dieser sollte so aussehen:

 ![alt text](images\optimal_dimensions.png)

, wobei hierin die Geometrie des main bodies des Satelliten beschrieben wird. 

Momentan sind die Dimensionen allerdings so: 

![alt text](images\subobtimal_dimensions.png)

