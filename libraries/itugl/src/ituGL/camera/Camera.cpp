#include <ituGL/camera/Camera.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <numbers>
#include <vector>

Camera::Camera() : m_viewMatrix(1.0f), m_projMatrix(1.0f)
{
}

void Camera::SetViewMatrix(const glm::vec3& position, const glm::vec3& lookAt, const glm::vec3& up)
{
    m_viewMatrix = glm::lookAt(position, lookAt, up);
}

void Camera::SetPerspectiveProjectionMatrix(float fov, float aspect, float near, float far)
{
    m_projMatrix = glm::perspective(fov, aspect, near, far);
}

void Camera::SetOrthographicProjectionMatrix(const glm::vec3& min, const glm::vec3& max)
{
    m_projMatrix = glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);
}

glm::vec3 Camera::ExtractTranslation() const
{
    // Keep only 3x3 rotation part of the matrix
    glm::mat3 transposed = glm::transpose(m_viewMatrix);

    glm::vec3 inverseTranslation = m_viewMatrix[3];

    return transposed * -inverseTranslation;
}

glm::vec3 Camera::ExtractRotation() const
{
    // Columns should be divided by scale first, but scale is (1, 1, 1) 
    glm::vec3 rotation(0.0f);
    float f = m_viewMatrix[1][2];
    if (std::abs(std::abs(f) - 1.0f) < 0.00001f)
    {
        rotation.x = -f * static_cast<float>(std::numbers::pi) * 0.5f;
        rotation.y = std::atan2(-f * m_viewMatrix[0][1], -f * m_viewMatrix[0][0]);
        rotation.z = 0.0f;
    }
    else
    {
        rotation.x = -std::asin(f);
        float cosX = std::cos(rotation.x);
        rotation.y = std::atan2(m_viewMatrix[0][2] / cosX, m_viewMatrix[2][2] / cosX);
        rotation.z = std::atan2(m_viewMatrix[1][0] / cosX, m_viewMatrix[1][1] / cosX);
    }
    return rotation;
}

glm::vec3 Camera::ExtractScale() const
{
    // Should return (1, 1, 1)
    return glm::vec3(m_viewMatrix[0].length(), m_viewMatrix[1].length(), m_viewMatrix[2].length());
}

void Camera::ExtractVectors(glm::vec3& right, glm::vec3& up, glm::vec3& forward) const
{
    // Keep only 3x3 rotation part of the matrix
    glm::mat3 transposed = glm::transpose(m_viewMatrix);

    right = transposed[0];
    up = transposed[1];
    forward = transposed[2];
}

std::vector<glm::vec4> Camera::GetFrustumCornersWorldSpace() const
{
    const glm::mat4 inv = glm::inverse(m_projMatrix * m_viewMatrix);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}

std::vector<glm::vec3> Camera::GetFrustumCorners3D() const
{
    std::vector<glm::vec3> retval;
    std::vector<glm::vec4> corners4D = GetFrustumCornersWorldSpace();
    for (auto& corner : corners4D) {
        retval.push_back(glm::vec3(corner));
    }

    return retval;
}