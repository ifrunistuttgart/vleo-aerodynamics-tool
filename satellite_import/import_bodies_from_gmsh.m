function bodies = import_bodies_from_gmsh(gmsh_mfile, ...
                                          rotation_hinge_points_CAD, ...
                                          rotation_directions_CAD, ...
                                          temperatures__K, ...
                                          energy_accommodation_coefficients, ...
                                          DCM_B_from_CAD, ...
                                          CoM_CAD)
% IMPORT_BODIES_FROM_GMSH Import bodies from a GMSH .m mesh file and convert to VLEO format
%
%   bodies = import_bodies_from_gmsh(gmsh_mfile, rotation_hinge_points_CAD, rotation_directions_CAD, ...
%                                    temperatures__K, energy_accommodation_coefficients, ...
%                                    DCM_B_from_CAD, CoM_CAD)
%
%   - gmsh_mfile: path to GMSH-exported .m file (defines 'msh' struct)
%   - rotation_hinge_points_CAD: 3xN array
%   - rotation_directions_CAD: 3xN array
%   - temperatures__K: 1xN cell array
%   - energy_accommodation_coefficients: 1xN cell array
%   - DCM_B_from_CAD: 3x3 matrix
%   - CoM_CAD: 3x1 vector
%
%   Returns:
%     bodies: 1xN cell array of structures compatible with importMultipleBodies.m

% Import mesh data
body_data = import_gmsh(gmsh_mfile);

num_bodies = length(body_data);
bodies = cell(1, num_bodies);

for i = 1:num_bodies
    % Transform vertices from CAD to body frame
    vertices_CAD = body_data(i).vertices_CAD; % These are in CAD frame
    n_faces = size(vertices_CAD, 3);
    vertices_CAD_list = reshape(vertices_CAD, 3, []);
    vertices_CAD_list = vertices_CAD_list - CoM_CAD;
    vertices_B_list = DCM_B_from_CAD * vertices_CAD_list;
    vertices_B = reshape(vertices_B_list, 3, 3, n_faces);
    bodies{i}.vertices_B = vertices_B;

    % Transform centroids from CAD to body frame
    centroids_CAD = body_data(i).centroids_CAD;
    centroids_B = DCM_B_from_CAD * (centroids_CAD - CoM_CAD);
    bodies{i}.centroids_B = centroids_B;

    % Transform normals from CAD to body frame (rotation only)
    normals_CAD = body_data(i).normals_CAD;
    normals_B = DCM_B_from_CAD * normals_CAD;
    bodies{i}.normals_B = normals_B;

    % Areas (no transformation needed)
    bodies{i}.areas = body_data(i).areas_CAD;

    % Rotation hinge point and direction (transform from CAD to body frame)
    bodies{i}.rotation_hinge_point_B = DCM_B_from_CAD * (rotation_hinge_points_CAD(:,i) - CoM_CAD);
    bodies{i}.rotation_direction_B = DCM_B_from_CAD * rotation_directions_CAD(:,i);

    % Surface temperatures
    t = temperatures__K{i};
    bodies{i}.temperatures__K = nan(1, n_faces);
    bodies{i}.temperatures__K(:) = t;

    % Energy accommodation coefficients
    eac = energy_accommodation_coefficients{i};
    bodies{i}.energy_accommodation_coefficients = nan(1, n_faces);
    bodies{i}.energy_accommodation_coefficients(:) = eac;
end
end
