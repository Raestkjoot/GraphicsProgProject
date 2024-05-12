#include <ituGL/renderer/ShadowMapRenderPass.h>

#include <ituGL/renderer/Renderer.h>
#include <ituGL/lighting/Light.h>
#include <ituGL/camera/Camera.h>
#include <ituGL/shader/Material.h>
#include <ituGL/texture/Texture2DObject.h>
#include <ituGL/texture/FramebufferObject.h>

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

ShadowMapRenderPass::ShadowMapRenderPass(std::shared_ptr<Light> light, std::shared_ptr<const Material> material, int drawcallCollectionIndex)
    : m_light(light)
    , m_material(material)
    , m_drawcallCollectionIndex(drawcallCollectionIndex)
    , m_volumeCenter(0.0f)
    , m_volumeSize(1.0f)
{
    InitFramebuffer();
}

void ShadowMapRenderPass::SetVolume(glm::vec3 volumeCenter, glm::vec3 volumeSize)
{
    m_volumeCenter = volumeCenter;
    m_volumeSize = volumeSize;
}

void ShadowMapRenderPass::SetSceneExtents(glm::vec3 sceneExtents)
{
    // Hardcoded because deadline. But, we know the scene starts from 0 and grows in positive x and z,
    // so we can just assume the aabb borders go from 0 - extents*2
    m_sceneAABBExtents = sceneExtents * 2.0f;
}

void ShadowMapRenderPass::InitFramebuffer()
{
    std::shared_ptr<FramebufferObject> targetFramebuffer = std::make_shared<FramebufferObject>();

    targetFramebuffer->Bind();

    std::shared_ptr<const TextureObject> shadowMap = m_light->GetShadowMap();
    assert(shadowMap);
    targetFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Depth, *shadowMap);

    FramebufferObject::Unbind();

    m_targetFramebuffer = targetFramebuffer;
}

void ShadowMapRenderPass::Render()
{
    Renderer& renderer = GetRenderer();
    DeviceGL& device = renderer.GetDevice();

    const auto& drawcallCollection = renderer.GetDrawcalls(m_drawcallCollectionIndex);

    device.Clear(false, Color(), true, 1.0f);

    // Use shadow map shader
    m_material->Use();
    std::shared_ptr<const ShaderProgram> shaderProgram = m_material->GetShaderProgram();

    // Backup current viewport
    glm::ivec4 currentViewport;
    device.GetViewport(currentViewport.x, currentViewport.y, currentViewport.z, currentViewport.w);

    // TODO: texture size should not be hardcoded, but be equal to shadow map resolution.
    // Set viewport to shadow map texture size
    device.SetViewport(0, 0, 1080, 1080);

    // TODO: Follow current camera
    // Backup current camera
    const Camera& currentCamera = renderer.GetCurrentCamera();

    // Set up light as the camera
    Camera lightCamera;
    InitLightCamera(lightCamera, currentCamera);
    renderer.SetCurrentCamera(lightCamera);

    // for all drawcalls
    bool first = true;
    for (const Renderer::DrawcallInfo& drawcallInfo : drawcallCollection)
    {
        // Bind the vao
        drawcallInfo.vao.Bind();

        // Set up object matrix
        renderer.UpdateTransforms(shaderProgram, drawcallInfo.worldMatrixIndex, first);

        // Render drawcall
        drawcallInfo.drawcall.Draw();

        first = false;
    }

    m_light->SetShadowMatrix(lightCamera.GetViewProjectionMatrix());

    // Restore viewport
    renderer.GetDevice().SetViewport(currentViewport.x, currentViewport.y, currentViewport.z, currentViewport.w);

    // Restore current camera
    renderer.SetCurrentCamera(currentCamera);

    // Restore default framebuffer to avoid drawing to the shadow map
    renderer.SetCurrentFramebuffer(renderer.GetDefaultFramebuffer());
}

void ShadowMapRenderPass::InitLightCamera(Camera& lightCamera, const Camera& curCamera)
{
    std::vector<glm::vec4> corners = curCamera.GetFrustumCornersWorldSpace();

    // Find the volume center
    glm::vec3 center(0.0f);
    for (const auto& v : corners)
    {
        center += glm::vec3(v);
    }
    center /= corners.size();
    glm::vec3 position = center;

    // Find the min and max corner values and set the volume size accordingly.
    // We can do this every frame, since eventually with CSM we will actually need to do that.
    const auto lightView = glm::lookAt(center + m_light->GetDirection(), center, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::lowest());
    for (const auto& v : corners)
    {
        const auto trf = lightView * v;
        min.x = std::min(min.x, trf.x);
        min.y = std::min(min.y, trf.y);
        //min.z = std::min(min.z, trf.z);
        max.x = std::max(max.x, trf.x);
        max.y = std::max(max.y, trf.y);
        //max.z = std::max(max.z, trf.z);
    }

    // TODO: Get a scene AABB and use it to clip against the four known light frustum side planes and 
    //       find the min and max z values for the near and far planes.
    //       Currently just clipping to the full scene AABB instead of only what's relevant to shadows visible to camera.
    auto sceneLightSpace = lightView * glm::vec4(m_sceneAABBExtents, 1.0f);

    min.z = -m_sceneAABBExtents.z;
    max.z = m_sceneAABBExtents.z;

    // View matrix (basically a duplicate of glm::lookAt that we are already doing
    // TODO: Don't call the same function twice.
    lightCamera.SetViewMatrix(position, position + m_light->GetDirection());

    // Projection matrix
    switch (m_light->GetType())
    {
    case Light::Type::Directional:
        lightCamera.SetOrthographicProjectionMatrix(min, max);
        break;
    default:
        assert(false);
        break;
    }
}

//void ShadowMapRenderPass::InitLightCamera(Camera& lightCamera) const
//{
//    // View matrix
//    glm::vec3 position = m_light->GetPosition(m_volumeCenter);
//    lightCamera.SetViewMatrix(position, position + m_light->GetDirection());
//
//    // Projection matrix
//    glm::vec4 attenuation = m_light->GetAttenuation();
//    switch (m_light->GetType())
//    {
//    case Light::Type::Directional:
//        lightCamera.SetOrthographicProjectionMatrix(-0.5f * m_volumeSize, 0.5f * m_volumeSize);
//        break;
//    case Light::Type::Spot:
//        lightCamera.SetPerspectiveProjectionMatrix(attenuation.w, 1.0f, 0.01f, attenuation.y);
//        break;
//    default:
//        assert(false);
//        break;
//    }
//}