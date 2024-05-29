#pragma once

#include <ituGL/renderer/RenderPass.h>
#include <ituGL/camera/Camera.h>

#include <glm/glm.hpp>
#include <vector>

class Light;
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
    void SetSceneAABBBounds(const glm::vec3& min, const glm::vec3& max);

    void Render() override;

    bool shouldFreeze = false;

private:
    void InitFramebuffer();
    void InitLightCamera(Camera& lightCamera, const Camera& curCamera);
    void ComputeNearAndFar(float& near, float& far, glm::vec2 lightFrustMin, glm::vec2 lightFrustMax, std::vector<glm::vec3> sceneAABB);
    void ComputeNearAndFarNoClip(float& near, float& far, std::vector<glm::vec3> sceneAABB);
    std::vector<glm::vec3> GetSceneAABBLightSpace(const glm::mat4& lightView);


private:
    std::shared_ptr<Light> m_light;

    std::shared_ptr<const Material> m_material;

    int m_drawcallCollectionIndex;

    glm::vec3 m_volumeCenter;
    glm::vec3 m_volumeSize;
    float m_shadowBufferSize;

    glm::vec3 m_sceneAABBMin;
    glm::vec3 m_sceneAABBMax;
    glm::vec3 m_sceneAABBExtents;
    glm::vec3 m_sceneAABBCenter;

    Camera m_mainCameraCopy;

};
