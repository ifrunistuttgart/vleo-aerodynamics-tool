% Test script for import_bodies_from_gmsh
import vleo_aerodynamics_core.*
% Path to your GMSH-exported .m mesh file
gmsh_mfile = 'C:\Users\Jan_L\OneDrive\Dokumente\Studium\Bacherlorarbeit\vleo-aerodynamics-tool-fork\satellite_import\satellite_parts\full_satellite.m';

% Load mesh to determine number of bodies
run(gmsh_mfile);
body_ids = unique(msh.TRIANGLES(:,4));
num_bodies = numel(body_ids);
disp(num_bodies);
% Assign dummy metadata for each body
surface_temperatures__K = num2cell(300*(0:4));
surface_energy_accommodation_coefficients = num2cell(0.01*ones(1,5));

rotation_hinge_points_CAD = [0,0,0,0.05,-0.05;...
                            0,-0.05,0.05,0,0;...
                            0,0,0,0,0];

rotation_directions_CAD = [1,-1,1,0,0;...
                            0,0,0,-1,1;...
                            0,0,0,0,0];
%DCM_B_from_CAD = eye(3);
 DCM_B_from_CAD = [0, 0, 1;...
                     0, 1, 0; ...
                     -1, 0, 0];
CoM_CAD = [0; 0; 0.1];

% Call the import function
bodies = import_bodies_from_gmsh(gmsh_mfile, ...
    rotation_hinge_points_CAD, ...
    rotation_directions_CAD, ...
    temperatures__K, ...
    energy_accommodation_coefficients, ...
    DCM_B_from_CAD, ...
    CoM_CAD);

% Display summary of imported bodies
disp(['Imported ', num2str(numel(bodies)), ' body/bodies.']);
for i = 1:numel(bodies)
    disp(['Body ', num2str(i), ' (ID ', num2str(body_ids(i)), '):']);
    disp(['  Number of faces: ', num2str(size(bodies{i}.vertices_B, 3))]);
    disp(['  First centroid: ', mat2str(bodies{i}.centroids_B(:,1))]);
    disp(['  First normal: ', mat2str(bodies{i}.normals_B(:,1))]);
    disp(['  First area: ', num2str(bodies{i}.areas(1))]);
end

showBodies(bodies,[0,0,0,0,0],0.75,0.25)
