f = fullfile("examples","geometries","soar_satellite.obj");
g = vat.geometry.RotatableMeshGeometry(f);
v = [1,0,0];
num_triangles = g.get_num_triangles();
disp(num_triangles)
vat.visualization.show_shading(g,zeros(num_triangles,1),v);   % close the window
disp("visualization closed")
%%
clear all %took ca. 24 s
f = fullfile("examples","geometries","soar_satellite.obj");
g = vat.geometry.RotatableMeshGeometry(f);
num_triangles = g.get_num_triangles();
v = [1,0,0];
vat.visualization.show_shading(g,zeros(num_triangles,1),v);   % access violation