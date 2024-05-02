#include "Terrain.h"

#include <ituGL/geometry/VertexFormat.h>
#include <ituGL/texture/Texture2DObject.h>

#include <glm/gtx/transform.hpp>  // for matrix transformations

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

#include <stb_image.h>

#include <cmath>
#include <iostream>
#include <numbers>  // for PI constant

std::vector<float> Terrain::CreateHeightMap(unsigned int width, unsigned int height)
{
    std::vector<float> pixels(height * width);
    for (unsigned int j = 0; j < height; ++j)
    {
        for (unsigned int i = 0; i < width; ++i)
        {
            float x = static_cast<float>(i) / (width - 1);
            float y = static_cast<float>(j) / (height - 1);
            pixels[j * width + i] = stb_perlin_fbm_noise3(x, y, 0.0f, 1.9f, 0.5f, 8) * 0.5f;
        }
    }

    return pixels;
}

void Terrain::CreateTerrainMesh(std::shared_ptr<Mesh> mesh, unsigned int gridX, unsigned int gridY)
{
    // Define the vertex structure
    struct Vertex
    {
        Vertex() = default;
        Vertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec2 texCoord)
            : position(position), normal(normal), tangent(tangent), bitangent(bitangent), texCoord(texCoord) {}
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        glm::vec2 texCoord;
    };

    // Define the vertex format (should match the vertex structure)
    VertexFormat vertexFormat;
    vertexFormat.AddVertexAttribute<float>(3); // position
    vertexFormat.AddVertexAttribute<float>(3); // normal
    vertexFormat.AddVertexAttribute<float>(3); // tangent
    vertexFormat.AddVertexAttribute<float>(3); // bitangent
    vertexFormat.AddVertexAttribute<float>(2); // texture coordinates

    // List of vertices (VBO)
    std::vector<Vertex> vertices;
    std::vector<glm::vec3> positions;

    // List of indices (EBO)
    std::vector<unsigned int> indices;

    // Grid scale to convert the entire grid to size 1x1
    float size = 10.0f;
    glm::vec2 scale(size / (gridX - 1), size / (gridY - 1));
    float height = 5.0f;

    // Number of columns and rows
    unsigned int columnCount = gridX + 1;
    unsigned int rowCount = gridY + 1;

    // Generate heightmap
    std::vector<float> heightmap = CreateHeightMap(columnCount, rowCount);

    // Iterate over each VERTEX
    for (unsigned int j = 0; j < rowCount; ++j)
    {
        for (unsigned int i = 0; i < columnCount; ++i)
        {
            // Vertex data for this vertex only
            glm::vec3 position(i * scale.x, heightmap[(j * columnCount + i)] * height, j* scale.y);
            //glm::vec3 position(i * scale.x, 0.0f, j* scale.y);
            positions.push_back(position);

            // Index data for quad formed by previous vertices and current
            if (i > 0 && j > 0)
            {
                unsigned int top_right = j * columnCount + i; // Current vertex
                unsigned int top_left = top_right - 1;
                unsigned int bottom_right = top_right - columnCount;
                unsigned int bottom_left = bottom_right - 1;

                //Triangle 1
                indices.push_back(bottom_left);
                indices.push_back(top_left);
                indices.push_back(bottom_right);

                //Triangle 2
                indices.push_back(bottom_right);
                indices.push_back(top_left);
                indices.push_back(top_right);
            }
        }
    }


    // Compute normals when we have the positions of all the vertices
    // Iterate AGAIN over each vertex
    for (unsigned int j = 0u; j < rowCount; ++j)
    {
        for (unsigned int i = 0u; i < columnCount; ++i)
        {

            unsigned int index = j * columnCount + i;
            unsigned int prevX = i > 0 ? index - 1 : index;
            unsigned int nextX = i < gridX ? index + 1 : index;
            int prevY = j > 0 ? index - columnCount : index;
            int nextY = j < gridY ? index + columnCount : index;

            glm::vec3 tangent = positions[nextX] - positions[prevX];
            tangent = glm::normalize(tangent);
            glm::vec3 bitangent = positions[nextY] - positions[prevY];
            bitangent = glm::normalize(bitangent);
            glm::vec3 normal = glm::cross(bitangent, tangent);
            glm::vec2 texCoord(i, j);

            vertices.emplace_back(positions[index], normal, tangent, bitangent, texCoord);
        }
    }

    mesh.get()->AddSubmesh<Vertex, unsigned int, VertexFormat::LayoutIterator>(Drawcall::Primitive::Triangles, vertices, indices,
        vertexFormat.LayoutBegin(static_cast<int>(vertices.size()), true /* interleaved */), vertexFormat.LayoutEnd());
}