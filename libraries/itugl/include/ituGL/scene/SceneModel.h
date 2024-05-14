#pragma once

#include <ituGL/scene/SceneNode.h>
//#include <ituGL/renderer/Renderable.h>

class Model;

class SceneModel : public SceneNode//, public Renderable
{
public:
    SceneModel(const std::string& name, std::shared_ptr<Model> model);
    SceneModel(const std::string& name, std::shared_ptr<Model> model, glm::vec3 aabbBoundsMin, glm::vec3 aabbBoundsMax);
    SceneModel(const std::string& name, std::shared_ptr<Model> model, std::shared_ptr<Transform> transform, glm::vec3 aabbBoundsMin, glm::vec3 aabbBoundsMax);

    std::shared_ptr<Model> GetModel() const;
    void SetModel(std::shared_ptr<Model> model);

    //glm::mat4 GetWorldMatrix() const override;
    //int GetDrawcallCount() const override;
    //const Drawcall& GetDrawcall(int index, const VertexArrayObject*& vao, const Material*& material) const override;

    SphereBounds GetSphereBounds() const override;
    AabbBounds GetAabbBounds() const override;
    BoxBounds GetBoxBounds() const override;
    glm::vec3 GetAabbExtents() const override;

    void AcceptVisitor(SceneVisitor& visitor) override;
    void AcceptVisitor(SceneVisitor& visitor) const override;

private:
    std::shared_ptr<Model> m_model;
};
