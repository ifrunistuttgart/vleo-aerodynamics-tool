function show_pressure_image(calculator, geometry, options)
    % SHOW_PRESSURE_IMAGE Plots the per-pixel pressure of the last evaluation.
    %   show_pressure_image(calculator, geometry) draws the pressure image of a
    %   vat.loads.PixelForceTorqueCalculator into the current axes: the
    %   geometry seen from upstream, looking along the flow. Pixels without a
    %   surface facing the flow are transparent, and the axes are zoomed to
    %   the surfaces that carry a load.
    %
    %   show_pressure_image(..., Scale="log") uses a logarithmic colour scale,
    %   which shows the small pressure on surfaces seen almost edge-on next to
    %   the large pressure on surfaces the flow hits head-on.
    %   show_pressure_image(..., CLim=[0 p_max]) fixes the colour limits, so
    %   several images can be compared directly, and Colorbar=false leaves
    %   out the colour bar, e.g. to draw one shared bar for several images.
    %
    %   The calculator must have been created with KeepPressureImage=true and
    %   evaluated at least once. Unlike the other visualization functions this
    %   one uses plain MATLAB graphics and does not block.
    %
    %   Input Arguments:
    %       calculator - A vat.loads.PixelForceTorqueCalculator.
    %       geometry   - The vat.geometry.RotatableMeshGeometry it evaluates;
    %                    its size sets the axes in metres.
    %
    %   See also vat.loads.PixelForceTorqueCalculator/pressure_image

    arguments
        calculator (1,1) vat.loads.PixelForceTorqueCalculator
        geometry (1,1) vat.geometry.RotatableMeshGeometry
        options.Scale (1,1) string {mustBeMember(options.Scale, ["linear", "log"])} = "linear"
        options.CLim (1,2) double = [NaN NaN]
        options.Colorbar (1,1) logical = true
    end

    % pressure_image returns the top row first; flip it so y points up.
    p = flipud(calculator.pressure_image());
    if isempty(p)
        error("No pressure image: create the calculator with KeepPressureImage=true and call calc_aero_load first.");
    end

    % The image spans the bounding sphere, [-R, R] in both directions.
    vertices = reshape(geometry.get_vertices(), 3, []);
    R = double(max(vecnorm(vertices)));
    x = linspace(-R, R, size(p, 1));

    imagesc(x, x, p, 'AlphaData', p > 0);
    set(gca, 'YDir', 'normal', 'Color', [0.95 0.95 0.95]);
    axis image;

    limits = options.CLim;
    if any(isnan(limits))
        limits = [0, max(p(:))];
    end
    if options.Scale == "log"
        set(gca, 'ColorScale', 'log');
        limits(1) = max(limits(1), limits(2) / 1e3);
    end
    if limits(2) > limits(1)
        clim(limits);
    end

    if options.Colorbar
        cb = colorbar;
        cb.Label.String = 'pressure [N/m^2]';
    end
    xlabel('across the flow [m]');
    ylabel('across the flow [m]');

    % Zoom to the loaded pixels, plus a small margin.
    cols = find(any(p > 0, 1));
    rows = find(any(p > 0, 2));
    if ~isempty(cols)
        margin = 0.05 * (x(end) - x(1));
        xlim([x(cols(1)) - margin, x(cols(end)) + margin]);
        ylim([x(rows(1)) - margin, x(rows(end)) + margin]);
    end
end
