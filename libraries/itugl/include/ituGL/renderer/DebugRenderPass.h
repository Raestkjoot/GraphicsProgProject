#pragma once

#include <ituGL/renderer/RenderPass.h>
#include <ituGL/geometry/VertexBufferObject.h>
#include <ituGL/geometry/VertexArrayObject.h>
#include <ituGL/shader/ShaderProgram.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>

#define MAX_DEBUG_VERTICES 64000

class Renderer;
class FramebufferObject;

struct DebugVertex
{
    glm::vec3 pos;
    unsigned int color;
};

class DebugRenderPass : public RenderPass
{
public:
    DebugRenderPass();
    virtual ~DebugRenderPass();

    void Render() override;

    void AddAABB(const glm::vec3& center, const glm::vec3& extents, unsigned int color);

private:
    //VertexBufferObject m_vbo;
    //VertexArrayObject m_vao;
    //ElementBufferObject m_ebo;

    std::vector<DebugVertex> m_points;

    VertexArrayObject m_vao;
    VertexBufferObject m_vbo;

    ShaderProgram m_shaderProgram;
    ShaderProgram::Location m_worldViewProjMatrix;
};
