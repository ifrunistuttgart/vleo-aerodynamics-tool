classdef RotatableMeshSatellite < vat.RotatableMeshGeometry
    % ROTATABLEMESHSATELLITE Deprecated alias for vat.RotatableMeshGeometry.
    %
    %   This class only exists so that existing scripts keep working. It behaves
    %   exactly like vat.RotatableMeshGeometry and can be passed anywhere a
    %   vat.RotatableMeshGeometry is expected.
    %
    %   Use vat.RotatableMeshGeometry instead. This alias will be removed in a
    %   future release.
    %
    %   See also vat.RotatableMeshGeometry
    methods
        function this = RotatableMeshSatellite(file_path)
            arguments
                file_path (1,1) string
            end
            this@vat.RotatableMeshGeometry(file_path);
            warning("vat:deprecated", ...
                "vat.RotatableMeshSatellite is deprecated and will be removed. " + ...
                "Use vat.RotatableMeshGeometry instead.");
        end
    end
end
