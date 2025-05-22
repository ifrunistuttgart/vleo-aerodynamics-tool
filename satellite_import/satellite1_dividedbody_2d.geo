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
Field[2].SizeMin = 0.001;
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
//+
Hide "*";
//+
Show {
  Point{1}; Point{2}; Point{3}; Point{4}; Point{5}; Point{6}; Point{7}; Point{8}; Point{13}; Point{14}; Point{15}; Point{16}; Point{19}; Point{20}; Point{23}; Point{24}; Point{25}; Point{26}; Point{27}; Point{28}; Point{33}; Point{34}; Point{37}; Point{38}; Curve{1}; Curve{2}; Curve{3}; Curve{4}; Curve{5}; Curve{6}; Curve{7}; Curve{8}; Curve{9}; Curve{10}; Curve{11}; Curve{12}; Curve{17}; Curve{18}; Curve{19}; Curve{20}; Curve{27}; Curve{31}; Curve{35}; Curve{36}; Curve{37}; Curve{38}; Curve{39}; Curve{40}; Curve{49}; Curve{53}; Curve{57}; Curve{58}; Surface{1}; Surface{2}; Surface{3}; Surface{4}; Surface{5}; Surface{6}; Surface{8}; Surface{16}; Surface{19}; Surface{27}; 
}
