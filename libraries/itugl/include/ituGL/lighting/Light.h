#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include <vector>

class TextureObject;

class Light
{
public:
    enum class Type
    {
        Directional,
        Point,
        Spot,
    };

public:
    Light();
    virtual ~Light();

    virtual Type GetType() const = 0;

    glm::vec3 GetPosition() const;
    virtual glm::vec3 GetPosition(const glm::vec3& fallback) const;
    virtual void SetPosition(const glm::vec3& position);

    glm::vec3 GetDirection() const;
    virtual glm::vec3 GetDirection(const glm::vec3& fallback) const;
    virtual void SetDirection(const glm::vec3& direction);

    virtual glm::vec4 GetAttenuation() const;

    glm::vec3 GetColor() const;
    void SetColor(const glm::vec3& color);

    float GetIntensity() const;
    void SetIntensity(float intensity);

    std::shared_ptr<const Texture2DArrayObject> GetShadowMap() const;
    void SetShadowMap(std::shared_ptr<const Texture2DArrayObject> shadowMap);
    virtual bool CreateShadowMap(glm::ivec2 resolution, int numOfCascades);

    std::vector<glm::mat4> GetShadowMatrices() const;
    glm::mat4 GetShadowMatrix(int index) const;
    void SetShadowMatrices(const std::vector<glm::mat4>& matrices);
    void SetShadowMatrix(const glm::mat4& matrix, int index);

    float GetShadowBias() const;
    void SetShadowBias(float bias);

    glm::vec2 GetShadowMapResolution() const;
    void SetShadowMapResolution(glm::vec2 resolution);

    unsigned int GetNumOfCascades() const;

protected:
    glm::vec3 m_color;
    float m_intensity;
    std::shared_ptr<const Texture2DArrayObject> m_shadowMap;
    std::vector<glm::mat4> m_shadowMatrices;
    float m_shadowBias;
    glm::ivec2 m_shadowMapResolution;
    unsigned int m_numOfCascades;
};
