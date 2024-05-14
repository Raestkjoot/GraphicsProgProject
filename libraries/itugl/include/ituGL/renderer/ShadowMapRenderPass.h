#pragma once

#include <ituGL/renderer/RenderPass.h>

#include <glm/glm.hpp>
#include <vector>

class Light;
class Camera;
class Material;

struct Triangle
{
    glm::vec3 points[3];
    bool isCulled;

    void SwapPoints(unsigned int x, unsigned int y) {
        assert(x < 3);
        assert(y < 3);
        glm::vec3 temp = points[x];
        points[x] = points[y];
        points[y] = temp;
    }
};

class ShadowMapRenderPass : public RenderPass
{
public:
    ShadowMapRenderPass(std::shared_ptr<Light> light, std::shared_ptr<const Material> material, int drawcallCollectionIndex = 0);

    void SetVolume(glm::vec3 volumeCenter, glm::vec3 volumeSize);
    void SetSceneExtents(glm::vec3 sceneExtents);
    void SetCascadeLevels(float viewCameraFarPlane);

    void Render() override;

private:
    void InitFramebuffer();
    void InitLightCamera(Camera& lightCamera, const Camera& viewCamera);
    void ComputeNearAndFar(float& near, float& far, glm::vec2 lightFrustMin, glm::vec2 lightFrustMax, std::vector<glm::vec3> sceneAABB);
    std::vector<glm::vec3> GetSceneAABBLightSpace(const glm::mat4& lightView);


private:
    std::shared_ptr<Light> m_light;

    std::shared_ptr<const Material> m_material;

    int m_drawcallCollectionIndex;

    glm::vec3 m_volumeCenter;
    glm::vec3 m_volumeSize;
    glm::vec3 m_sceneAABBExtents;
    float m_shadowBufferSize;
    std::vector<float> m_cascadeLevels;
};
