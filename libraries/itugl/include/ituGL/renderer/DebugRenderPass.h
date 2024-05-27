#pragma once

#include <ituGL/renderer/RenderPass.h>
#include <ituGL/geometry/VertexBufferObject.h>
#include <ituGL/geometry/VertexArrayObject.h>
#include <ituGL/shader/ShaderProgram.h>
#include <ituGL/core/Color.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <vector>

#define MAX_DEBUG_VERTICES 64000

class Renderer;
class FramebufferObject;

struct DebugVertex
{
    glm::vec3 pos;
    Color color;
};

class DebugRenderPass : public RenderPass
{
public:
    DebugRenderPass();
    virtual ~DebugRenderPass();

    void Render() override;

    void DrawAABB(const glm::vec3& center, const glm::vec3& extents, Color color);
    void DrawOBB3D(const glm::vec3& center, const glm::vec3& extents, const glm::quat& rotation, Color color);
    void DrawMinMaxBox(const glm::vec3& min, const glm::vec3& max, Color color);
    void DrawSquare(const std::vector<glm::vec3>& corners, Color color);
    void DrawArbitraryBox(const std::vector<glm::vec3>& corner, Color color);
    void DrawArbitraryBoxWithTransformation(const std::vector<glm::vec4>& corners, const glm::mat4& transformMatrix, Color color);
    void DrawFrustum(const glm::mat4x4& viewProjectionMatrix, Color color);
    void DrawMatrix(const glm::mat4x4& matrix, float scale);
    void DrawMatrix(const glm::mat4x4& matrix, const glm::vec3& origin, float scale);
    void DrawLine3D(const glm::vec3& from, const glm::vec3& to, Color color);
    glm::vec3 UnProject(const glm::vec3& point, const glm::mat4x4& m);

private:
    std::vector<DebugVertex> m_points;

    VertexArrayObject m_vao;
    VertexBufferObject m_vbo;

    ShaderProgram m_shaderProgram;
    ShaderProgram::Location m_worldViewProjMatrix;
};
