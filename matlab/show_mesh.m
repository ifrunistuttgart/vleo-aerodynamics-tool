function show_mesh(satellite, triangle_visbility, velocity__m_per_s)
    MexGateway("Visualization.show_mesh", satellite.handle_, double(triangle_visbility), velocity__m_per_s);
end