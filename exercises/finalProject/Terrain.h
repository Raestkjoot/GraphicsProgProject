#pragma once

#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/geometry/Mesh.h>
#include <ituGL/camera/Camera.h>
#include <ituGL/shader/Material.h>
#include <glm/mat4x4.hpp>
#include <vector>

class Texture2DObject;

class Terrain
{
public:
    static std::vector<float> CreateTerrainMesh(std::shared_ptr<Mesh> mesh, unsigned int gridX, unsigned int gridY, float height, float size);

private:
    static std::vector<float> CreateHeightMap(unsigned int horizontal, unsigned int vertical, float height);
};
