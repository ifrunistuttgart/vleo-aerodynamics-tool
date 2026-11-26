%% Generate the rectangular shuttlecock at several mesh resolutions
% Writes rect_shuttlecock/rect_shuttlecock_L<k>_<N>tri.obj for k = 0..max_level.
% Every flat face of rect_shuttlecock_faces() is split into a structured grid of
% n1 x n2 quads (two triangles each) with n = ceil(edge length / h_k). The
% element size h_k of each level is chosen so the triangle count is as close
% as possible to base_count * 2^k (rounding the short edges up would otherwise
% make the counts jump irregularly). Edges of equal length get the same
% subdivision, so the body mesh is conforming. The wings are two coincident
% sheets with opposite winding (zero thickness, two-sided).

clear;

base_count = 60; % triangles of level 0 (h = 0.1 m, one quad across the short edges)
max_level = 5;   % finest level (~2000 triangles)

folder = fullfile(fileparts(mfilename('fullpath')), 'rect_shuttlecock');
if ~isfolder(folder)
    mkdir(folder);
end
old_files = dir(fullfile(folder, 'rect_shuttlecock_L*.obj'));
for k = 1:numel(old_files)
    delete(fullfile(old_files(k).folder, old_files(k).name));
end

faces = rect_shuttlecock_faces();
exact_area__m2 = sum(arrayfun(@(f) norm(cross(f.e1, f.e2)), faces));
objects = unique({faces.object}, 'stable');

fprintf('%5s %10s %10s %14s\n', 'level', 'h [mm]', 'triangles', 'area error');
candidate_h__m = logspace(log10(0.1), log10(1e-3), 4000);
candidate_count = arrayfun(@(h) count_triangles(faces, h), candidate_h__m);
for level = 0:max_level
    [~, best] = min(abs(log(candidate_count / (base_count * 2^level))));
    h__m = candidate_h__m(best);
    lines = {sprintf('# Rectangular shuttlecock, level %d, h = %.4f m (make_rect_shuttlecock.m)', level, h__m)};
    vertex_offset = 0;
    num_triangles = 0;
    area__m2 = 0;
    for o = 1:numel(objects)
        lines{end + 1} = ['o ' objects{o}]; %#ok<SAGROW>
        face_lines = {};
        for f = faces(strcmp({faces.object}, objects{o}))
            [vertices, triangles] = grid_face(f, h__m);
            lines = [lines, compose('v %.9f %.9f %.9f', vertices)']; %#ok<AGROW>
            face_lines = [face_lines, compose('f %d %d %d', triangles + vertex_offset)']; %#ok<AGROW>
            vertex_offset = vertex_offset + size(vertices, 1);
            num_triangles = num_triangles + size(triangles, 1);
            area__m2 = area__m2 + triangle_area(vertices, triangles);
        end
        lines = [lines, face_lines]; %#ok<AGROW>
    end
    file = fullfile(folder, sprintf('rect_shuttlecock_L%d_%dtri.obj', level, num_triangles));
    writelines(lines, file);
    fprintf('%5d %10.2f %10d %14.2e\n', level, 1e3 * h__m, num_triangles, area__m2 / exact_area__m2 - 1);
end
fprintf('Written to %s\n', folder);

%% Local functions
function n = num_divisions(edge__m, h__m)
    n = max(1, ceil(norm(edge__m) / h__m - 1e-9));
end

function count = count_triangles(faces, h__m)
    count = sum(arrayfun(@(f) 2 * num_divisions(f.e1, h__m) * num_divisions(f.e2, h__m), faces));
end

function [vertices, triangles] = grid_face(face, h__m)
    % Structured grid of one rectangle; triangle normals along cross(e1, e2).
    n1 = num_divisions(face.e1, h__m);
    n2 = num_divisions(face.e2, h__m);
    [u, v] = ndgrid((0:n1) / n1, (0:n2) / n2);
    vertices = face.P0 + u(:) * face.e1 + v(:) * face.e2;
    index = @(i, j) i + (n1 + 1) * j + 1; % 0-based grid index -> 1-based vertex
    triangles = zeros(2 * n1 * n2, 3);
    t = 0;
    for j = 0:n2 - 1
        for i = 0:n1 - 1
            triangles(t + 1, :) = [index(i, j), index(i + 1, j), index(i + 1, j + 1)];
            triangles(t + 2, :) = [index(i, j), index(i + 1, j + 1), index(i, j + 1)];
            t = t + 2;
        end
    end
end

function area__m2 = triangle_area(vertices, triangles)
    edge_1 = vertices(triangles(:, 2), :) - vertices(triangles(:, 1), :);
    edge_2 = vertices(triangles(:, 3), :) - vertices(triangles(:, 1), :);
    area__m2 = sum(vecnorm(cross(edge_1, edge_2, 2), 2, 2)) / 2;
end
