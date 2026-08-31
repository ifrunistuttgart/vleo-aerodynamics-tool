function show_mesh(satellite, triangle_visbility, velocity__m_per_s)
    arguments
        satellite (1,1) RotatableMeshSatellite
        triangle_visbility (:,1) single
        velocity__m_per_s (1,3) double
    end
    MexGateway("Visualization.show_mesh", int32(satellite.handle_), single(triangle_visbility), velocity__m_per_s);
end