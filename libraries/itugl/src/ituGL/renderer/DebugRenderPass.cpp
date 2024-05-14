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

#undef min

void DrawLine(std::vector<DebugVertex>& points, glm::vec3 from, glm::vec3 to, unsigned int color);


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
    VertexAttribute colorAttribute(Data::Type::UInt, 1);

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
    glDisable(GL_CULL_FACE);

    // Draw points. The amount of points can't exceed the capacity
    int points = std::min((int)m_points.size(), MAX_DEBUG_VERTICES);
    glDrawArrays(GL_LINES, 0, points);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_points.clear();
}

void DebugRenderPass::AddAABB(const glm::vec3& center, const glm::vec3& extents, unsigned int color)
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

void DrawLine(std::vector<DebugVertex>& points, const glm::vec3& from, const glm::vec3& to, unsigned int color)
{
    points.push_back(DebugVertex(from, color));
    points.push_back(DebugVertex(to, color));
}