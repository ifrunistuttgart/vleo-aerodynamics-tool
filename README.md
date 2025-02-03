# How to: VLEO Aerodynamic Tool

## This README is a guidance for the use of the VELO Areodynamic Tool. It explains the varoius commands and resolves questions that could come up while working with the tool
 

### Import CAD-Files with importMultipleBodies.m

After crating your satellite configuration in your preferred CAD-Tool, you can export each part (individually!!!) as an .obj. These objects will be used in the vleo-aerodynamic-tool submodule to accordingly import and handle the satellite components in your simulation.
Question: Is the order, in which the parts are fed into the function important? Regarding the main object, and the fins, that are attached at different sides of the satellite.

Yes its very important!


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




#### rotation_hinge_points_CAD


Definition  
A point in the CAD model (in local coordinates) around which a specific component (e.g., a control surface) rotates.

Format  
A **3×N matrix**, where each column **[x; y; z]** represents the hinge point of a component.  
The dimensions are in the units of the CAD model (often millimeters or meters).

Example  
```matlab
rotation_hinge_points_CAD = [0, 0, 0, 0, 0; ...
                             0, 3.75, 3.75, 3.75, 3.75; ...
                             0, 0, 0, 0, 0];
```

Interpretation  
- The **first column** `[0; 0; 0]` represents the hinge point of the **main body** (e.g., `body.obj`), which is often fixed.  
- The **other columns** represent the hinge points of the **control surfaces** (e.g., `right_control_surface.obj`), all rotating around a point on the **y-axis at 3.75**.



#### rotation_directions_CAD

Definition  
A vector that indicates the direction of the rotation axis. This describes around which axis the component rotates relative to its hinge point.

Format  
A **3×N matrix**, where each column **[x; y; z]** represents the direction of the rotation axis in local coordinates.

Example  
```matlab
rotation_directions_CAD = [1, 1, 0, -1, 0; ...
                            0, 0, 0, 0, 0; ...
                            0, 0, 1, 0, -1];
```

Interpretation  
- The **first column** `[1; 0; 0]` describes a rotation around the x-axis for the **main body**.
- The **second column** `[1; 0; 0]` describes a rotation of the **right control surface** also around the x-axis.
- The **third column** `[0; 0; 1]` describes a rotation around the **z-axis** (e.g., for the lower control surface).
- The **negative values** (e.g., `-1` in the fourth column) indicate an opposite rotation direction.





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

 ![alt text](optimal_dimensions.png)

, wobei hierin die Geometrie des main bodies des Satelliten beschrieben wird. 

Momentan sind die Dimensionen allerdings so: 

![alt text](subobtimal_dimensions.png)

Ich begebe mich auf die Suche nach dem Ursprung dessen.

Änderung des main-bodies im CAD-Modell. Fehler ist weg und es wird ein plot erstellt. Der Satellit ist darin allerdings noch nicht sichtbar.


![alt text](first_try_no_satellite.png)

Trotz erster Änderung der hinge-points und der rotation-direction ändert sich an den body structs nichts.