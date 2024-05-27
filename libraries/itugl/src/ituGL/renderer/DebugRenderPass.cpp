#include <ituGL/renderer/DebugRenderPass.h>

#include <ituGL/camera/Camera.h>
#include <ituGL/renderer/Renderer.h>
#include <ituGL/shader/Shader.h>
#include <ituGL/geometry/VertexAttribute.h>
#include <vector>
#include <iostream>
#include <cassert>
#include <array>
#include <fstream>
#include <sstream>

void LoadAndCompileShader(Shader& shader, const char* path)
{
    // Open the file for reading
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "Can't find file: " << path << std::endl;
        std::cout << "Is your working directory properly set?" << std::endl;
        return;
    }

    // Dump the contents into a string
    std::stringstream stringStream;
    stringStream << file.rdbuf() << '\0';

    // Set the source code from the string
    shader.SetSource(stringStream.str().c_str());

    // Try to compile
    if (!shader.Compile())
    {
        // Get errors in case of failure
        std::array<char, 256> errors;
        shader.GetCompilationErrors(errors);
        std::cout << "Error compiling shader: " << path << std::endl;
        std::cout << errors.data() << std::endl;
        assert(false);
    }
}

DebugRenderPass::DebugRenderPass()
{
    m_vbo.Bind();
    m_vbo.AllocateData(MAX_DEBUG_VERTICES * sizeof(DebugVertex), BufferObject::Usage::DynamicDraw);

    m_vao.Bind();
    GLsizei stride = sizeof(DebugVertex);

    // Declare attributes
    VertexAttribute positionAttribute(Data::Type::Float, 3);
    VertexAttribute colorAttribute(Data::Type::Float, 4);

    m_vao.SetAttribute(0, positionAttribute, 0, stride);
    m_vao.SetAttribute(1, colorAttribute, sizeof(glm::vec3), stride);

    m_vao.Unbind();
    m_vbo.Unbind();

    Shader vertexShader(Shader::VertexShader);
    LoadAndCompileShader(vertexShader, "shaders/debug.vert");
    Shader fragmentShader(Shader::FragmentShader);
    LoadAndCompileShader(fragmentShader, "shaders/debug.frag");

    // Attach shaders and link
    if (!m_shaderProgram.Build(vertexShader, fragmentShader))
    {
        std::cout << "Error linking shaders" << std::endl;
    }

    m_worldViewProjMatrix = m_shaderProgram.GetUniformLocation("WorldViewProjMatrix");
}

DebugRenderPass::~DebugRenderPass()
{
}

void DebugRenderPass::Render()
{
    Renderer& renderer = GetRenderer();

    m_vbo.Bind();
    m_vbo.UpdateData(std::span(m_points), 0);

    // Set our shader program
    m_shaderProgram.Use();
    m_shaderProgram.SetUniform(m_worldViewProjMatrix,
        renderer.GetCurrentCamera().GetViewProjectionMatrix());

    // Bind the VAO
    m_vao.Bind();

    glDisable(GL_DEPTH_TEST);

    // Draw points. The amount of points can't exceed the capacity
    int points = std::min((int)m_points.size(), MAX_DEBUG_VERTICES);
    glDrawArrays(GL_LINES, 0, points);

    glEnable(GL_DEPTH_TEST);

    m_points.clear();
}

void DebugRenderPass::DrawAABB(const glm::vec3& center, const glm::vec3& extents, Color color)
{
    glm::vec3 v0 = center - extents;
    glm::vec3 v1 = center + extents;

    // Bottom
    m_points.push_back({ { v0.x, v0.y, v0.z }, color });
    m_points.push_back({ { v1.x, v0.y, v0.z }, color });
    m_points.push_back({ { v1.x, v0.y, v0.z }, color });
    m_points.push_back({ { v1.x, v0.y, v1.z }, color });
    m_points.push_back({ { v1.x, v0.y, v1.z }, color });
    m_points.push_back({ { v0.x, v0.y, v1.z }, color });
    m_points.push_back({ { v0.x, v0.y, v1.z }, color });
    m_points.push_back({ { v0.x, v0.y, v0.z }, color });

    // Top
    m_points.push_back({ { v0.x, v1.y, v0.z }, color });
    m_points.push_back({ { v1.x, v1.y, v0.z }, color });
    m_points.push_back({ { v1.x, v1.y, v0.z }, color });
    m_points.push_back({ { v1.x, v1.y, v1.z }, color });
    m_points.push_back({ { v1.x, v1.y, v1.z }, color });
    m_points.push_back({ { v0.x, v1.y, v1.z }, color });
    m_points.push_back({ { v0.x, v1.y, v1.z }, color });
    m_points.push_back({ { v0.x, v1.y, v0.z }, color });

    // Vertical edges
    m_points.push_back({ { v0.x, v0.y, v0.z }, color });
    m_points.push_back({ { v0.x, v1.y, v0.z }, color });
    m_points.push_back({ { v1.x, v0.y, v0.z }, color });
    m_points.push_back({ { v1.x, v1.y, v0.z }, color });
    m_points.push_back({ { v0.x, v0.y, v1.z }, color });
    m_points.push_back({ { v0.x, v1.y, v1.z }, color });
    m_points.push_back({ { v1.x, v0.y, v1.z }, color });
    m_points.push_back({ { v1.x, v1.y, v1.z }, color });
}

void DebugRenderPass::DrawOBB3D(const glm::vec3& center, const glm::vec3& extents, const glm::quat& rotation, Color color)
{
    glm::vec3 corners[8] = {
        center + rotation * glm::vec3(-extents.x, -extents.y, -extents.z),
        center + rotation * glm::vec3(extents.x, -extents.y, -extents.z),
        center + rotation * glm::vec3(extents.x, -extents.y,  extents.z),
        center + rotation * glm::vec3(-extents.x, -extents.y,  extents.z),
        center + rotation * glm::vec3(-extents.x,  extents.y, -extents.z),
        center + rotation * glm::vec3(extents.x,  extents.y, -extents.z),
        center + rotation * glm::vec3(extents.x,  extents.y,  extents.z),
        center + rotation * glm::vec3(-extents.x,  extents.y,  extents.z)
    };

    // Bottom
    m_points.push_back({ corners[0], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[0], color });

    // Top
    m_points.push_back({ corners[4], color });
    m_points.push_back({ corners[5], color });
    m_points.push_back({ corners[5], color });
    m_points.push_back({ corners[6], color });
    m_points.push_back({ corners[6], color });
    m_points.push_back({ corners[7], color });
    m_points.push_back({ corners[7], color });
    m_points.push_back({ corners[4], color });

    // Vertical edges
    m_points.push_back({ corners[0], color });
    m_points.push_back({ corners[4], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[5], color });
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[6], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[7], color });
}

void DebugRenderPass::DrawMinMaxBox(const glm::vec3& min, const glm::vec3& max, Color color)
{
    glm::vec3 corners[8] = {
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(min.x, max.y, max.z)
    };

    // Bottom
    m_points.push_back({ corners[0], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[0], color });

    // Top
    m_points.push_back({ corners[4], color });
    m_points.push_back({ corners[5], color });
    m_points.push_back({ corners[5], color });
    m_points.push_back({ corners[6], color });
    m_points.push_back({ corners[6], color });
    m_points.push_back({ corners[7], color });
    m_points.push_back({ corners[7], color });
    m_points.push_back({ corners[4], color });

    // Vertical edges
    m_points.push_back({ corners[0], color });
    m_points.push_back({ corners[4], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[5], color });
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[6], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[7], color });
}

void DebugRenderPass::DrawSquare(const std::vector<glm::vec3>& corners, Color color)
{
    m_points.push_back({ corners[0], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[0], color });
}

void DebugRenderPass::DrawArbitraryBox(const std::vector<glm::vec3>& corners, Color color)
{
    // Assuming loop of foreach x foreach y foreach z

    // Bottom
    m_points.push_back({ corners[0], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[5], color });
    m_points.push_back({ corners[5], color });
    m_points.push_back({ corners[4], color });
    m_points.push_back({ corners[4], color });
    m_points.push_back({ corners[0], color });

    // Top
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[7], color });
    m_points.push_back({ corners[7], color });
    m_points.push_back({ corners[6], color });
    m_points.push_back({ corners[6], color });
    m_points.push_back({ corners[2], color });

    // Vertical edges
    m_points.push_back({ corners[0], color });
    m_points.push_back({ corners[2], color });
    m_points.push_back({ corners[1], color });
    m_points.push_back({ corners[3], color });
    m_points.push_back({ corners[4], color });
    m_points.push_back({ corners[6], color });
    m_points.push_back({ corners[5], color });
    m_points.push_back({ corners[7], color });
}

void DebugRenderPass::DrawArbitraryBoxWithTransformation(const std::vector<glm::vec4>& corners, const glm::mat4& transformMatrix, Color color)
{
    // Assuming loop of foreach x foreach y foreach z
    std::vector<glm::vec3> transformedCorners;
    for (auto v : corners)
    {
        transformedCorners.push_back(glm::vec3(transformMatrix * v));
    }

    // Bottom
    m_points.push_back({ transformedCorners[0], color });
    m_points.push_back({ transformedCorners[1], color });
    m_points.push_back({ transformedCorners[1], color });
    m_points.push_back({ transformedCorners[5], color });
    m_points.push_back({ transformedCorners[5], color });
    m_points.push_back({ transformedCorners[4], color });
    m_points.push_back({ transformedCorners[4], color });
    m_points.push_back({ transformedCorners[0], color });

    // Top
    m_points.push_back({ transformedCorners[2], color });
    m_points.push_back({ transformedCorners[3], color });
    m_points.push_back({ transformedCorners[3], color });
    m_points.push_back({ transformedCorners[7], color });
    m_points.push_back({ transformedCorners[7], color });
    m_points.push_back({ transformedCorners[6], color });
    m_points.push_back({ transformedCorners[6], color });
    m_points.push_back({ transformedCorners[2], color });

    // Vertical edges
    m_points.push_back({ transformedCorners[0], color });
    m_points.push_back({ transformedCorners[2], color });
    m_points.push_back({ transformedCorners[1], color });
    m_points.push_back({ transformedCorners[3], color });
    m_points.push_back({ transformedCorners[4], color });
    m_points.push_back({ transformedCorners[6], color });
    m_points.push_back({ transformedCorners[5], color });
    m_points.push_back({ transformedCorners[7], color });
}

void DebugRenderPass::DrawFrustum(const glm::mat4x4& viewProjectionMatrix, Color color)
{
    const glm::mat4x4 m = glm::inverse(viewProjectionMatrix);

    const glm::vec3 near0 = UnProject(glm::vec3(-1.0f, -1.0f, 0.0f), m);
    const glm::vec3 near1 = UnProject(glm::vec3(+1.0f, -1.0f, 0.0f), m);
    const glm::vec3 near2 = UnProject(glm::vec3(+1.0f, +1.0f, 0.0f), m);
    const glm::vec3 near3 = UnProject(glm::vec3(-1.0f, +1.0f, 0.0f), m);

    const glm::vec3 far0 = UnProject(glm::vec3(-1.0f, -1.0f, 1.0f), m);
    const glm::vec3 far1 = UnProject(glm::vec3(+1.0f, -1.0f, 1.0f), m);
    const glm::vec3 far2 = UnProject(glm::vec3(+1.0f, +1.0f, 1.0f), m);
    const glm::vec3 far3 = UnProject(glm::vec3(-1.0f, +1.0f, 1.0f), m);

    // Near plane
    DrawLine3D(near0, near1, color);
    DrawLine3D(near1, near2, color);
    DrawLine3D(near2, near3, color);
    DrawLine3D(near3, near0, color);

    // Far plane
    DrawLine3D(far0, far1, color);
    DrawLine3D(far1, far2, color);
    DrawLine3D(far2, far3, color);
    DrawLine3D(far3, far0, color);

    // Edges
    DrawLine3D(near0, far0, color);
    DrawLine3D(near1, far1, color);
    DrawLine3D(near2, far2, color);
    DrawLine3D(near3, far3, color);
}

void DebugRenderPass::DrawMatrix(const glm::mat4x4& matrix, float scale)
{
    const glm::vec3 origin = glm::vec3(matrix[3].x, matrix[3].y, matrix[3].z);

    DrawLine3D(origin, origin + (glm::vec3(matrix[0].x, matrix[0].y, matrix[0].z) * scale), Color(1.0f, 0.0f, 0.0f));
    DrawLine3D(origin, origin + (glm::vec3(matrix[1].x, matrix[1].y, matrix[1].z) * scale), Color(0.0f, 1.0f, 0.0f));
    DrawLine3D(origin, origin + (glm::vec3(matrix[2].x, matrix[2].y, matrix[2].z) * scale), Color(0.0f, 0.0f, 1.0f));
}

void DebugRenderPass::DrawMatrix(const glm::mat4x4& matrix, const glm::vec3& origin, float scale)
{
    DrawLine3D(origin, origin + (glm::vec3(matrix[0].x, matrix[0].y, matrix[0].z) * scale), Color(1.0f, 0.0f, 0.0f));
    DrawLine3D(origin, origin + (glm::vec3(matrix[1].x, matrix[1].y, matrix[1].z) * scale), Color(0.0f, 1.0f, 0.0f));
    DrawLine3D(origin, origin + (glm::vec3(matrix[2].x, matrix[2].y, matrix[2].z) * scale), Color(0.0f, 0.0f, 1.0f));
}

void DebugRenderPass::DrawLine3D(const glm::vec3& from, const glm::vec3& to, Color color)
{
    m_points.push_back({ from, color });
    m_points.push_back({ to, color });
}

glm::vec3 DebugRenderPass::UnProject(const glm::vec3& point, const glm::mat4x4& m)
{
    glm::vec4 obj = m * glm::vec4(point, 1.0f);
    obj /= obj.w;
    return glm::vec3(obj);
}