#pragma once


class ISatelliteshadingData {
public:
	virtual int get_vertices();
	virtual int get_indices();
	virtual int get_vertices_count();
	virtual int get_indices_count();
	virtual ~ISatelliteshadingData() = default;
};