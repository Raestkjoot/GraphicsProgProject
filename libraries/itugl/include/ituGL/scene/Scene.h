#pragma once

#include <glm/vec3.hpp>

#include <unordered_map>
#include <string>
#include <memory>

class SceneNode;
class SceneVisitor;

class Scene
{
public:
    Scene();
    ~Scene();

    std::shared_ptr<SceneNode> GetSceneNode(const std::string& name) const;

    bool AddSceneNode(std::shared_ptr<SceneNode> node);

    bool RemoveSceneNode(std::shared_ptr<SceneNode> node);
    bool RemoveSceneNode(const std::string& name);

    void AcceptVisitor(SceneVisitor& visitor);
    void AcceptVisitor(SceneVisitor& visitor) const;

    glm::vec3 GetAABBExtents() const;

private:
    std::unordered_map<std::string, std::shared_ptr<SceneNode>> m_nodes;
    glm::vec3 m_AABBExtents;
};
