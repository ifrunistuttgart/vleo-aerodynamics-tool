clear all;
close all;
clc

% Importieren der einzelnen Körperteile
satellite = gmsh_import('full_satellite.m');

% % Mesh-Visualisierung vorbereiten
% figure;
% hold on;
% axis equal;
% view(3);
% xlabel('X');
% ylabel('Y');
% zlabel('Z');
% title('Satelliten-Mesh');
% 
% % Farben definieren
% faceColor = [0.7, 0.7, 0.8];  % Hellgrau
% edgeColor = 'k';          % Keine Kanten
% 
% % Dreiecke visualisieren
% for i = 1:satellite.n
%     % Aktuelle Dreieckspunkte
%     tri = satellite.vertices_B(:, :, i);
% 
%     % Patch-Objekt zeichnen
%     patch('Faces', [1 2 3], ...
%           'Vertices', tri', ...
%           'FaceColor', faceColor, ...
%           'EdgeColor', edgeColor, ...
%           'FaceAlpha', 0.9);
% end
% 
% % Optional: Schwerpunkte und Normalen anzeigen
% quiver3(satellite.centroids_B(1,:), ...
%         satellite.centroids_B(2,:), ...
%         satellite.centroids_B(3,:), ...
%         satellite.normals_B(1,:), ...
%         satellite.normals_B(2,:), ...
%         satellite.normals_B(3,:), ...
%         0.05, 'r');
% 
% legend('Mesh-Flächen', 'Normalenvektoren');