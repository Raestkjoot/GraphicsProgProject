#include <ituGL/renderer/ShadowMapRenderPass.h>

#include <ituGL/renderer/Renderer.h>
#include <ituGL/lighting/Light.h>
#include <ituGL/camera/Camera.h>
#include <ituGL/shader/Material.h>
#include <ituGL/texture/Texture2DObject.h>
#include <ituGL/texture/FramebufferObject.h>
#include <ituGL/core/Color.h>

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

void ShadowMapRenderPass::SetVolume(const glm::vec3& volumeCenter, const glm::vec3& volumeSize)
{
    m_volumeCenter = volumeCenter;
    m_volumeSize = volumeSize;
}

void ShadowMapRenderPass::SetSceneAABBBounds(const glm::vec3& min, const glm::vec3& max)
{
    m_sceneAABBMin = min;
    m_sceneAABBMax = max;

    m_sceneAABBExtents = (max - min) * 0.5f;
    m_sceneAABBCenter = min + m_sceneAABBExtents;
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
    m_shadowBufferSize = static_cast<float>(m_light->GetShadowMapResolution().x);
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

    // Set viewport to shadow map texture size
    device.SetViewport(0, 0, static_cast<GLsizei>(m_light->GetShadowMapResolution().x), static_cast<GLsizei>(m_light->GetShadowMapResolution().y));

    // Backup current camera and use to clip light shadow projection.
    const Camera& currentCamera = renderer.GetCurrentCamera();

    // Set up light as the camera
    Camera lightCamera;

    if (!shouldFreeze)
        m_mainCameraCopy = currentCamera;

    glEnable(GL_DEPTH_CLAMP); // pancaking
    InitLightCamera(lightCamera, m_mainCameraCopy);

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

    // Restore depth culling
    glDisable(GL_DEPTH_CLAMP);
}

void ShadowMapRenderPass::InitLightCamera(Camera& lightCamera, const Camera& curCamera)
{
    Renderer& renderer = GetRenderer();
    DebugRenderPass& debugRenderer = renderer.GetDebugRenderPass();

    debugRenderer.DrawAABB(m_sceneAABBCenter, m_sceneAABBExtents, Color(1.0f, 1.0f, 1.0f)); // Draw scene AABB

    // Find the view volume center
    std::vector<glm::vec3> viewCorners = curCamera.GetFrustumCornersWorldSpace3D();
    debugRenderer.DrawArbitraryBox(viewCorners, Color(1.0f, 0.0f, 0.0f)); // Draw view frustum
    glm::vec3 center(0.0f);
    for (const auto& v : viewCorners)
    {
        center += v;
    }
    center /= viewCorners.size();

    glm::vec3 lightDirection = center + m_light->GetDirection();
    auto lightView = glm::lookAt(center, lightDirection, glm::vec3(0.0f, 1.0f, 0.0f));
    debugRenderer.DrawMatrix(glm::inverse(lightView), center, 20.0f); // Draw light view matrix in center of view frustum

    // Compute min and max xy for the light projection matrix
    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::lowest());
    float sphereRadius = 0.0f;
    for (const auto& v : viewCorners)
    {
        // calculate radius of a bounding sphere surrounding the view frustum corners
        float dist = glm::distance(v, center);
        sphereRadius = glm::max(sphereRadius, dist);
        // This transformation should take the points into light space, where we want to find the xy bounds
        const auto trf = lightView * glm::vec4(v, 1.0f);
        min.x = std::min(min.x, trf.x);
        min.y = std::min(min.y, trf.y);
        min.z = std::min(min.z, trf.z);
        max.x = std::max(max.x, trf.x);
        max.y = std::max(max.y, trf.y);
        max.z = std::max(max.z, trf.z);
    }

    // DEBUG xy bounds of the light cam
    {
        float lightCenter = (lightView * glm::vec4(center.x, center.y, center.z, 1.0f)).z;
        lightCenter /= (lightView * glm::vec4(center.x, center.y, center.z, 1.0f)).w;
        std::vector<glm::vec4> lightBoundsLightSpace;
        lightBoundsLightSpace.push_back(glm::vec4(min.x, min.y, lightCenter, 1.0f));
        lightBoundsLightSpace.push_back(glm::vec4(min.x, max.y, lightCenter, 1.0f));
        lightBoundsLightSpace.push_back(glm::vec4(max.x, max.y, lightCenter, 1.0f));
        lightBoundsLightSpace.push_back(glm::vec4(max.x, min.y, lightCenter, 1.0f));

        std::vector<glm::vec3> lightBoundsWorldSpace;
        auto invLightView = glm::inverse(lightView);
        for (auto v : lightBoundsLightSpace)
        {
            glm::vec3 corn = glm::vec3(invLightView * v);
            lightBoundsWorldSpace.push_back(corn);
        }

        debugRenderer.DrawSquare(lightBoundsWorldSpace, Color(0.8f, 0.0f, 0.8f)); // Draw 2D square representing the xy bounds of the light camera
    }

    float near, far;
    near = min.z;
    far = max.z;
    float lightNearFarExtent = far - near;

    // Alternate method to pancaking: clip scene AABB against view frustum
    // auto sceneLightSpace = GetSceneAABBLightSpace(lightView);
    // min max in light space. Passing in z as reference so ComputeNearFar can recalculate those to a tight fit.
    //ComputeNearAndFar(near, far, glm::vec2(min.x, min.y), glm::vec2(max.x, max.y), sceneLightSpace);

    // STABILIZATION: Use bounding sphere surrounding the view frustum corners
    // to counteract flickering from camera rotation.
    // This was the original line but it still caused some flickering:
    // sphereRadius = std::ceil(sphereRadius * 16.0f) / 16.0f; 
    sphereRadius = std::ceil(sphereRadius);
    // Maybe we need a more specific bias, since ceil might fail if the radius dithering between two numbers like 300.99 and 301.01

    min.x = center.x - sphereRadius;
    max.x = center.x + sphereRadius;
    min.y = center.y - sphereRadius;
    max.y = center.y + sphereRadius;

    min.z = 0.0f;
    max.z = lightNearFarExtent;

    // STABILIZATION: Move light camera in worldUnitsPerShadowMapTexel increments
    // to counteract flickering from camera translation.
    // https://github.com/TheRealMJP/Shadows/blob/master/Shadows/MeshRenderer.cpp#L1500
    glm::vec3 lightCameraPos = center - m_light->GetDirection() * far;
    lightView = glm::lookAt(lightCameraPos, center, glm::vec3(0.0f, 1.0f, 0.0f));
    lightCamera.SetOrthographicProjectionMatrix(min, max);
    lightCamera.SetViewMatrix(lightView);

    // Create the rounding matrix, by projecting the world-space origin and determining
    // the fractional offset in texel space
    glm::vec4 shadowOrigin = glm::vec4(0.f, 0.f, 0.f, 1.0f);
    shadowOrigin = lightCamera.GetViewProjectionMatrix() * shadowOrigin;
    shadowOrigin *= m_shadowBufferSize / 2.0f;

    glm::vec4 roundOrigin = glm::round(shadowOrigin);
    glm::vec4 roundOffset = roundOrigin - shadowOrigin;
    roundOffset *= 2.0f / m_shadowBufferSize;
    roundOffset.z = 0.0f;
    roundOffset.w = 0.0f;

    glm::mat4x4 shadowProj = lightCamera.GetProjectionMatrix();
    shadowProj[3] += roundOffset;
    lightCamera.SetProjectionMatrix(shadowProj);



    // If we want to support multiple light types
    switch (m_light->GetType())
    {
    case Light::Type::Directional:
        break;
    default:
        assert(false);
        break;
    }
}

void ShadowMapRenderPass::ComputeNearAndFar(float& near, float& far, const glm::vec2& lightFrustumMin, const glm::vec2& lightFrustumMax, const std::vector<glm::vec3>& sceneAABBLightSpace)
{
    near = std::numeric_limits<float>::max();
    far = std::numeric_limits<float>::lowest();

    Triangle triangles[16];

    // These are the indices used to tesselate an AABB into a list of triangles.
    assert(sceneAABBLightSpace.size() == 8);
    static const int aabbTriIndexes[] =
    {
        0,1,2,  1,2,3,
        4,5,6,  5,6,7,
        0,2,4,  2,4,6,
        1,3,5,  3,5,7,
        0,1,4,  1,4,5,
        2,3,6,  3,6,7
    };

    // For each triangle in sceneAABB
    for (int aabbTriIter = 0; aabbTriIter < 12; ++aabbTriIter)
    {
        triangles[0].points[0] = sceneAABBLightSpace[aabbTriIndexes[aabbTriIter * 3 + 0]];
        triangles[0].points[1] = sceneAABBLightSpace[aabbTriIndexes[aabbTriIter * 3 + 1]];
        triangles[0].points[2] = sceneAABBLightSpace[aabbTriIndexes[aabbTriIter * 3 + 2]];
        triangles[0].isCulled = false;
        int triCount = 1;

        // Foreach m in lightFrustum borders: minX, maxX, minY, maxY
        for (int m = 0; m < 4; ++m)
        {
            float edge;
            int dimension;

            switch (m)
            {
            case 0:
                dimension = 0;
                edge = lightFrustumMin[dimension];
                break;
            case 1:
                dimension = 0;
                edge = lightFrustumMax[dimension];
                break;
            case 2:
                dimension = 1;
                edge = lightFrustumMin[dimension];
                break;
            case 3:
                dimension = 1;
                edge = lightFrustumMax[dimension];
                break;
            }

            // Foreach triangle until triangleCount
            for (int triIter = 0; triIter < triCount; ++triIter)
            {
                if (triangles[triIter].isCulled) {
                    // skip culled triangles. They have been clipped and are basically considered deleted.
                    continue;
                }

                // count how many points of the triangle are inside the light frustum.
                int inCount = 0;
                bool pointPassesCollision[3];

                for (int triPtIter = 0; triPtIter < 3; ++triPtIter) 
                {
                    switch (m)
                    {
                    case 0:
                    case 2:
                        // Check if it is greater than min, thus inside
                        if (triangles[triIter].points[triPtIter][dimension] > edge) {
                            pointPassesCollision[triPtIter] = true;
                            inCount++;
                        }
                        else {
                            pointPassesCollision[triPtIter] = false;
                        }
                        break;
                    case 1:
                    case 3:
                        // Check if it is less than max, thus inside
                        if (triangles[triIter].points[triPtIter][dimension] < edge) {
                            pointPassesCollision[triPtIter] = true;
                            inCount++;
                        }
                        else {
                            pointPassesCollision[triPtIter] = false;
                        }
                        break;
                    }
                }

                if (inCount == 0)
                {
                    // All points outside, so triangle can be completely discarded.
                    triangles[triIter].isCulled = true;
                    continue;
                }
                if (inCount == 3)
                {
                    // All points inside
                    triangles[triIter].isCulled = false;
                    continue;
                }
                // If we reach this point, then some triangle points are inside and some are outside the light frustum edge. 

                // Move points that pass the frustum test to the begining of the array.
                // This is so we know where the in and out points are in the array, which we will use to move the outside points inside.
                if (pointPassesCollision[1] && !pointPassesCollision[0])
                {
                    triangles[triIter].SwapPoints(0, 1);
                    pointPassesCollision[0] = true;
                    pointPassesCollision[1] = false;
                }
                if (pointPassesCollision[2] && !pointPassesCollision[1])
                {
                    triangles[triIter].SwapPoints(1, 2);
                    pointPassesCollision[1] = true;
                    pointPassesCollision[2] = false;
                }
                if (pointPassesCollision[1] && !pointPassesCollision[0])
                {
                    triangles[triIter].SwapPoints(0, 1);
                    pointPassesCollision[0] = true;
                    pointPassesCollision[1] = false;
                }

                // We will clip the triangle so that the points outside are moved inside.
                if (inCount == 1)
                {
                    // One point passed. Clip triangle against frustum plane by moving the two outside points to the plane.
                    triangles[triIter].isCulled = false;

                    glm::vec3 edge01 = triangles[triIter].points[1] - triangles[triIter].points[0];
                    glm::vec3 edge02 = triangles[triIter].points[2] - triangles[triIter].points[0];

                    // Find the collision ratio to the outside point.
                    float contactPoint = edge - triangles[triIter].points[0][dimension];
                    // Calculate the distance along the vector as ratio of the hit ratio to the dimension.
                    float dist01 = contactPoint / edge01[dimension];
                    float dist02 = contactPoint / edge02[dimension];

                    // Move the outside triangle points to be on the frustum plane in this dimension by 
                    // linearly interpolating from the inside point towards the outside point by the distance to the contact point.
                    triangles[triIter].points[1] = edge01 * dist01 + triangles[triIter].points[0];
                    triangles[triIter].points[2] = edge02 * dist02 + triangles[triIter].points[0];
                }
                else if (inCount == 2)
                {
                    // Two points passed. Clip the triangle by moving the outside point to the two places its edges intersect the plane.
                    // This will clip the triangle point and give us a quad inside the frustum, so we have to create a new triangle.
                    triangles[triCount] = triangles[triIter + 1];

                    triangles[triIter].isCulled = false;
                    triangles[triIter + 1].isCulled = false;

                    glm::vec3 edge20 = triangles[triIter].points[0] - triangles[triIter].points[2];
                    glm::vec3 edge21 = triangles[triIter].points[1] - triangles[triIter].points[2];

                    // Find the collision ratio to the outside point.
                    float contactPoint = edge - triangles[triIter].points[2][dimension];
                    // Calculate the distance along the vector as ratio of the hit ratio to the dimension.
                    float dist20 = contactPoint / edge20[dimension];
                    float dist21 = contactPoint / edge21[dimension];
                    
                    // Add the new triangle
                    triangles[triIter + 1].points[0] = triangles[triIter].points[0];
                    triangles[triIter + 1].points[1] = triangles[triIter].points[1];
                    triangles[triIter + 1].points[2] = edge20 * dist20 + triangles[triIter].points[0];

                    // Update this triangle
                    triangles[triIter].points[0] = triangles[triIter + 1].points[1];
                    triangles[triIter].points[1] = triangles[triIter + 1].points[2];
                    triangles[triIter].points[2] = edge21 * dist21 + triangles[triIter].points[0];

                    // Increment triangle count and skip the triangle we just inserted.
                    triCount++;
                    triIter++;
                }
            }
        }
        // After iterating over the four light frustum points and clipping this AABB triangle to be inside, we check 
        // if this triangle has a point with a tighter z value and update the near and far planes accordingly.
        for (int index = 0; index < triCount; ++index)
        {
            if (!triangles[index].isCulled)
            {
                for (int pointIter = 0; pointIter < 3; pointIter++)
                {
                    float zVal = triangles[index].points[pointIter].z;
                    if (near > zVal)
                    {
                        near = zVal;
                    }
                    if (far < zVal)
                    {
                        far = zVal;
                    }
                }
            }
        }
    }
}

std::vector<glm::vec3> ShadowMapRenderPass::GetSceneAABBLightSpace(const glm::mat4& lightView)
{
    // 8 corners of the AABB box
    std::vector<glm::vec3> aabbCorners;

    glm::vec3 sceneAABB[2] = { m_sceneAABBMin, m_sceneAABBMax };
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt = lightView * glm::vec4(sceneAABB[x].x, sceneAABB[y].y, sceneAABB[z].z, 1.0f);
                aabbCorners.push_back(pt / pt.w);
            }
        }
    }

    return aabbCorners;
}

/*
    // TODO: Does the worldUnitsPerTexel require world space? See InitLightCamera.
    glm::vec2 worldEdges(std::numeric_limits<float>::max());
    for (const auto& v : corners)
    {
        const auto trf = lightView * v;
        if (min.x > trf.x)
        {
            min.x = trf.x;
            worldEdges.x = v.x;
        }
        if (min.y > trf.y)
        {
            min.y = trf.y;
        }
        if (max.x < trf.x)
        {
            max.x = trf.x;
            worldEdges.y = v.x;
        }
        if (max.y < trf.y)
        {
            max.y = trf.y;
        }
    }
    float worldUnitsPerTexel = glm::length(worldEdges) / m_shadowBufferSize;
*/