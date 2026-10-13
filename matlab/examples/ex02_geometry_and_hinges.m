%% Example 2: Geometry and hinges
% A geometry file can hold several meshes (one `o` object each in an .obj),
% and every mesh can be turned about its own hinge: solar arrays, drag
% sails, control panels. This example finds out which mesh is which,
% defines a hinge for each wing of the shuttlecock, checks the hinges
% visually and turns the wings.
%
% The show_* functions open an interactive 3D window and wait until it is
% closed.
%
% Prerequisites, from the repository root:
%   pixi run build-matlab
%   addpath('matlab'); addpath('matlab\bin')

clear; close all;
vat.setLogLevel("warn");

obj_file = fullfile(fileparts(mfilename('fullpath')), 'geometries', 'shuttlecock_15k.obj');
geometry = vat.geometry.RotatableMeshGeometry(obj_file);

%% 1. Which mesh is which?
% Every mesh gets its own colour; the legend reads  id <mesh_id>  "<name>".
% The shuttlecock has the body (id 0) and four wings (ids 1 to 4).
vat.visualization.show_meshes(geometry);

%% 2. Define a hinge per wing
% A hinge is a mesh, a point on the hinge line and the direction of that
% line, both in the body frame. A positive angle turns the wing by the
% right-hand rule about the axis; these axes are chosen so that a positive
% angle turns each wing out into the flow.
hinges(1).mesh_id = 1;  hinges(1).origin = [-0.15,  0.10,  0.05];  hinges(1).axis = [0,  1,  0];  % bottom
hinges(2).mesh_id = 2;  hinges(2).origin = [-0.15, -0.05,  0.10];  hinges(2).axis = [0,  0,  1];  % left
hinges(3).mesh_id = 3;  hinges(3).origin = [-0.15,  0.05, -0.10];  hinges(3).axis = [0,  0, -1];  % right
hinges(4).mesh_id = 4;  hinges(4).origin = [-0.15, -0.10, -0.05];  hinges(4).axis = [0, -1,  0];  % up

% Check the hinges before using them: the plot shows each hinge line on
% its mesh, so a wrong origin or axis is easy to spot.
vat.visualization.show_hinges(geometry, hinges);

%% 3. Turn the wings
% turn_mesh_around_axis sets an absolute angle (radians), it does not add
% to the previous one. Turning back to 0 restores the original shape.
angles__deg = [10, 20, 50, -5];
for i = 1:numel(hinges)
    geometry.turn_mesh_around_axis(hinges(i).mesh_id, deg2rad(angles__deg(i)), ...
        hinges(i).origin, hinges(i).axis);
end
vat.visualization.show_meshes(geometry);

% Every calculator built on this geometry sees the turned wings from now
% on; no calculator or pipeline has to be rebuilt (see example 7).
