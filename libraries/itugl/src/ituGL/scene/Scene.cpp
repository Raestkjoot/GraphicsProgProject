#include <ituGL/scene/Scene.h>

#include <ituGL/scene/SceneNode.h>
#include <ituGL/scene/SceneModel.h>
#include <ituGL/scene/SceneVisitor.h>
#include <cassert>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

Scene::Scene() : m_AABBMinBounds({ 0.0f }), m_AABBMaxBounds({ 0.0f }), m_AABBCenter({ 0.0f }), m_AABBExtents({ 0.0f })
{
}

Scene::~Scene()
{
    for (auto& pair : m_nodes)
    {
        pair.second->SetOwnerScene(nullptr);
    }
}

std::shared_ptr<SceneNode> Scene::GetSceneNode(const std::string& name) const
{
    auto it = m_nodes.find(name);
    if (it != m_nodes.end())
    {
        return it->second;
    }
    return nullptr;
}

bool Scene::AddSceneNode(std::shared_ptr<SceneNode> node)
{
    assert(node);
    m_nodes[node->GetName()] = node;
    node->SetOwnerScene(this);

    glm::vec3 nodeMin = node->GetAabbBounds().GetMin();
    glm::vec3 nodeMax = node->GetAabbBounds().GetMax();
    m_AABBMinBounds = glm::min(nodeMin, m_AABBMinBounds);
    m_AABBMaxBounds = glm::max(nodeMax, m_AABBMaxBounds);

    m_AABBExtents = (m_AABBMaxBounds - m_AABBMinBounds) * 0.5f;
    m_AABBCenter = m_AABBMinBounds + m_AABBExtents;

    return true;
}

bool Scene::RemoveSceneNode(std::shared_ptr<SceneNode> node)
{
    assert(m_nodes.find(node->GetName()) == m_nodes.end() || m_nodes.find(node->GetName())->second == node);
    return RemoveSceneNode(node->GetName());
}

bool Scene::RemoveSceneNode(const std::string& name)
{
    auto it = m_nodes.find(name);
    if (it != m_nodes.end())
    {
        assert(it->second);
        assert(it->second->GetOwnerScene() == this);
        it->second->SetOwnerScene(nullptr);
        m_nodes.erase(it);
        return true;
    }
    return false;
}

void Scene::AcceptVisitor(SceneVisitor& visitor)
{
    for (auto& pair : m_nodes)
    {
        pair.second->AcceptVisitor(visitor);
    }
}

void Scene::AcceptVisitor(SceneVisitor& visitor) const
{
    for (auto& pair : m_nodes)
    {
        pair.second->AcceptVisitor(visitor);
    }
}

void Scene::GetAABBBounds(glm::vec3& min, glm::vec3& max)
{
    min = m_AABBMinBounds;
    max = m_AABBMaxBounds;
}

glm::vec3 Scene::GetAABBExtents() const
{
    return m_AABBExtents;
}