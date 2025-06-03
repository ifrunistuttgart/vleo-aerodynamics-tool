import vleo_aerodynamics_core.*
clear all
close all
clc

% Import the satellite data using your GMSH importer
satellite = import_gmsh('full_satellite.m');

fprintf('Imported satellite with %d bodies\n', length(satellite));

%% Method 1: Convert GMSH data to format compatible with showBodies
bodies = cell(1, length(satellite));

for i = 1:length(satellite)
    % Copy the mesh data
    bodies{i}.vertices_B = satellite(i).vertices_B;
    bodies{i}.centroids_B = satellite(i).centroids_B;
    bodies{i}.normals_B = satellite(i).normals_B;
    bodies{i}.areas = satellite(i).areas_B;
    
    % Dummy rotation parameters
    bodies{i}.rotation_hinge_point_B = [0; 0; 0];
    bodies{i}.rotation_direction_B = [0; 0; 1];
    
    % Dummy temperature and accommodation coefficients
    num_faces = size(satellite(i).vertices_B, 3);
    bodies{i}.temperatures__K = 300 * ones(1, num_faces);
    bodies{i}.energy_accommodation_coefficients = 0.8 * ones(1, num_faces);
end

% Set zero rotation angles for all bodies
bodies_rotation_angles__rad = zeros(1, length(bodies));

% Visualize using showBodies
fprintf('Visualizing satellite using showBodies function...\n');
showBodies(bodies, bodies_rotation_angles__rad, 0.7, 0.02);
title('Satellite Mesh - Using showBodies');

%% Method 2: Direct visualization
figure;
hold on;
axis equal;
view(3);
xlabel('X [m]'); ylabel('Y [m]'); zlabel('Z [m]');
title('Satellite Mesh - Direct Visualization');
grid on;

colors = lines(length(satellite));

for i = 1:length(satellite)
    vertices = satellite(i).vertices_B;
    centroids = satellite(i).centroids_B;
    normals = satellite(i).normals_B;
    current_color = colors(i, :);
    
    for tri_idx = 1:size(vertices, 3)
        tri_vertices = squeeze(vertices(:, :, tri_idx))';
        patch('Faces', [1 2 3], ...
              'Vertices', tri_vertices, ...
              'FaceColor', current_color, ...
              'EdgeColor', 'k', ...
              'FaceAlpha', 0.8);
    end
    
    scatter3(centroids(1,:), centroids(2,:), centroids(3,:), 20, current_color, 'filled');
    quiver3(centroids(1,:), centroids(2,:), centroids(3,:), ...
            normals(1,:), normals(2,:), normals(3,:), ...
            0.02, 'Color', current_color, 'LineWidth', 1.5);
end

%legend('show');

%% Print statistics
fprintf('\n=== Satellite Mesh Statistics ===\n');
fprintf('Number of bodies: %d\n', length(satellite));

total_triangles = 0;
total_area = 0;

for i = 1:length(satellite)
    num_triangles = size(satellite(i).vertices_B, 3);
    body_area = sum(satellite(i).areas_B);

    fprintf('Body %d: %d triangles, total area = %.6f m²\n', i, num_triangles, body_area);
    
    total_triangles = total_triangles + num_triangles;
    total_area = total_area + body_area;
end

fprintf('Total: %d triangles, total area = %.6f m²\n', total_triangles, total_area);

% Mesh bounds
all_vertices = [];
for i = 1:length(satellite)
    reshaped = reshape(satellite(i).vertices_B, 3, []);
    all_vertices = [all_vertices, reshaped];
end

min_coords = min(all_vertices, [], 2);
max_coords = max(all_vertices, [], 2);

fprintf('\nMesh bounds:\n');
fprintf('X: [%.4f, %.4f] m\n', min_coords(1), max_coords(1));
fprintf('Y: [%.4f, %.4f] m\n', min_coords(2), max_coords(2));
fprintf('Z: [%.4f, %.4f] m\n', min_coords(3), max_coords(3));
