#pragma once


class ISatellite {
public: 
    
    virtual int get_envelope() = 0;
    
    virtual int get_vertices() = 0;

    virtual int get_transformation_vertices() = 0;

    virtual int get_areas() = 0;

    virtual int get_normals() = 0;

    virtual int get_centroids() = 0;

    virtual int get_com() = 0;

	virtual ~ISatellite() = default;
};