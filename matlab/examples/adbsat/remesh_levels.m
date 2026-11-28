%% Remesh the ADBSat comparison geometry to several resolutions
% Remeshes shuttlecock_base_rotated_panels.obj with the vat remesher into
% num_levels uniform meshes, each with about twice the triangles of the
% previous one, and exports them to the remeshed/ subfolder. Every part keeps
% its mesh_id and name. The files can be imported into ADBSat with ADBSatImport.

clear; close all;
vat.setLogLevel("warn");

base_triangle_count = 100; % target triangle count of the coarsest level
num_levels = 10;             % number of resolutions, each ~2x the previous

folder = fileparts(mfilename('fullpath'));
base_name = 'shuttlecock_base_rotated_panels';
source_file = fullfile(folder, [base_name '.obj']); % the unremeshed source lives next to this script
output_folder = fullfile(folder, 'remeshed');
if ~isfolder(output_folder)
    mkdir(output_folder);
end

geometry = vat.geometry.RotatableMeshGeometry(source_file);
quality = geometry.get_mesh_quality();
fprintf('Source: %s, %d triangles\n\n', [base_name '.obj'], quality.num_triangles);

fprintf('%5s %8s %10s %8s %12s %14s %10s\n', 'level', 'target', 'triangles', 'ratio', ...
        'edge [mm]', 'area change %', 'AR median');
previous_count = NaN;
for level = 1:num_levels
    target_count = base_triangle_count * 2^(level - 1);
    prediction = geometry.predict_remesh(TriangleCount = target_count);
    [remeshed, report] = geometry.remesh(TriangleCount = target_count);
    count = report.after.num_triangles;

    file = fullfile(output_folder, sprintf('%s_L%d_%dtri.obj', base_name, level, count));
    remeshed.export_obj(file);

    fprintf('%5d %8d %10d %8.2f %12.2f %+14.4f %10.2f\n', level, target_count, count, ...
            count / previous_count, 1e3 * prediction.target_edge_length__m, report.area_change__percent, ...
            report.after.aspect_ratio.median);
    previous_count = count;
end
fprintf('\nWritten to %s\n', output_folder);
