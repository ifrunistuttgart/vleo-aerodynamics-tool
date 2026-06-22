classdef RotatableMeshSatellite < handle
    properties %(Access = private, Hidden = true)
        handle_ = -1;
    end
    methods
        function this = RotatableMeshSatellite(file_path)
            assert(this.handle_ == -1, "This object is already constructed.");
            this.handle_ = MexGateway("Satellite.new", string(file_path));
        end
      
        function delete(this)
            MexGateway("Satellite.delete", this.handle_);
            this.handle_ = -1;
        end

        function vertices = get_vertices(this)
            vertices = MexGateway("Satellite.get_vertices", this.handle_);
        end

        function num_triangles = get_num_triangles(this)
            num_triangles = MexGateway("Satellite.get_num_triangles",this.handle_);
        end

        function turn_surface_around_axis(this,surface_id, angle__rad, origin, axis)
            MexGateway("Satellite.turn_surface_around_axis",this.handle_,int32(surface_id),angle__rad,origin,axis)
        end
    end
end
