#pragma once
#include <string>
#include <vector>

namespace core {

    struct SoftModel{

        // point cloud
        // material = mesh shader (surface reconstruction) + fragment
        // or
        // skin that works like bones + some shader that reconstructs surface on nodes elimination;
    };

    struct Model{

        //mesh -> mesh chunks + material


        Model()
        {

        }
    };


    struct Material{

        
        Material(std::vector<std::string> shaders)
        {

        }

    };
}
