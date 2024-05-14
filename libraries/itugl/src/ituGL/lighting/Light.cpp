#include <ituGL/lighting/Light.h>

#include <ituGL/texture/Texture2DObject.h>
#include <ituGL/texture/Texture2DArrayObject.h>

#include <vector>

Light::Light() : m_color(1.0f), m_intensity(1.0f)
{
}

Light::~Light()
{
}

glm::vec3 Light::GetPosition() const
{
    return GetPosition(glm::vec3(0));
}

glm::vec3 Light::GetPosition(const glm::vec3& fallback) const
{
    return fallback;
}

void Light::SetPosition(const glm::vec3& position)
{
}

glm::vec3 Light::GetDirection() const
{
    return GetDirection(glm::vec3(0, 1, 0));
}

glm::vec3 Light::GetDirection(const glm::vec3& fallback) const
{
    return fallback;
}

void Light::SetDirection(const glm::vec3& direction)
{
}

glm::vec4 Light::GetAttenuation() const
{
    return glm::vec4(-1);
}

glm::vec3 Light::GetColor() const
{
    return m_color;
}

void Light::SetColor(const glm::vec3& color)
{
    m_color = color;
}

float Light::GetIntensity() const
{
    return m_intensity;
}

void Light::SetIntensity(float intensity)
{
    m_intensity = intensity;
}

std::shared_ptr<const TextureObject> Light::GetShadowMap() const
{
    return m_shadowMap;
}

void Light::SetShadowMap(std::shared_ptr<const TextureObject> shadowMap)
{
    m_shadowMap = shadowMap;
}

bool Light::CreateShadowMap(glm::ivec2 resolution, unsigned int numOfCascades)
{
    assert(!m_shadowMap);
    std::shared_ptr<Texture2DArrayObject> shadowMap = std::make_shared<Texture2DArrayObject>();
    shadowMap->Bind();
    shadowMap->SetImage(0, resolution.x, resolution.y, numOfCascades, TextureObject::FormatDepth, TextureObject::InternalFormatDepth32);
    shadowMap->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);
    shadowMap->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);
    shadowMap->SetParameter(TextureObject::ParameterEnum::WrapS, GL_CLAMP_TO_BORDER);
    shadowMap->SetParameter(TextureObject::ParameterEnum::WrapT, GL_CLAMP_TO_BORDER);
    glm::vec4 borderColor(1.0f);
    shadowMap->SetParameter(TextureObject::ParameterColor::BorderColor, std::span<float, 4>(&borderColor[0], &borderColor[0] + 4));

    m_shadowMap = shadowMap;
    m_numOfCascades = numOfCascades;
    SetShadowMapResolution(resolution);

    return true;
}

std::vector<glm::mat4> Light::GetShadowMatrix() const
{
    return m_shadowMatrix;
}

void Light::SetShadowMatrix(size_t index, const glm::mat4& matrix)
{
    m_shadowMatrix[index] = matrix;
}

float Light::GetShadowBias() const
{
    return m_shadowBias;
}

void Light::SetShadowBias(float bias)
{
    assert(bias >= 0.0f);
    m_shadowBias = bias;
}


glm::vec2 Light::GetShadowMapResolution() const
{
    return m_shadowMapResolution;
}

void Light::SetShadowMapResolution(glm::vec2 resolution)
{
    m_shadowMapResolution = resolution;
}

unsigned int Light::GetNumOfCascades() const
{
    return m_numOfCascades;
}