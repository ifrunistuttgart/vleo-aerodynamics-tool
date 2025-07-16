// Gmsh project created on Tue May 06 17:15:01 2025
SetFactory("OpenCASCADE");

// === Hauptkörper ===
Box(1) = {-0.05, -0.05, 0, 0.1, 0.1, 0.2};

// === Flügel ===
// Rechts (+X)
Box(2) = {0.05, -0.049, 0, -0.001, 0.098, -0.133333};
// Oben (+Y)
Box(3) = {-0.049, 0.05, 0, 0.098, -0.001, -0.133333};
// Links (-X)
Box(4) = {-0.049, -0.049, 0, -0.001, 0.098, -0.133333};
// Unten (-Y)
Box(5) = {-0.049, -0.049, 0, 0.098, -0.001, -0.133333};

// === Physical Groups ===
// Sie ermöglichen die gezielte Auswahl beim Export
Physical Surface("Body")        = {1, 2, 3, 4, 5, 6}; // Hauptkörper
Physical Surface("WingRight")   = {8};
Physical Surface("WingTop")     = {16};
Physical Surface("WingLeft")    = {19};
Physical Surface("WingBottom")  = {27};

// === Mesh ===

Field[1] = Distance;
Field[1].SurfacesList = {5}; // Beispiel: Fläche vom Hauptkörper
Field[1].NumPointsPerCurve = 100;

Field[2] = Threshold;
Field[2].InField = 1;
Field[2].SizeMin = 0.01;
Field[2].SizeMax = 0.05;
Field[2].DistMin = 0.001;
Field[2].DistMax = 0.05;

Field[3] = Restrict;
Field[3].IField = 2;
Field[3].SurfacesList = {8, 16, 19, 27};

// Ganz grobes Standard-Mesh
Mesh.CharacteristicLengthMax = 1.;
Mesh.CharacteristicLengthMin = 0.001;

// Hauptkörper explizit grob:
Characteristic Length{ PointsOf{ Volume{1}; } } = 1.0;

// Format für MATLAB-Export einstellen
Mesh.Format = 39; // MATLAB-Format
////////////////////////////////////////////////////

// --- Aktivieren ---
Background Field = 3;

Mesh 2;

