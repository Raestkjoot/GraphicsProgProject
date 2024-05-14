#include <ituGL/scene/SceneModel.h>

#include <ituGL/geometry/Model.h>
#include <ituGL/geometry/Mesh.h>
#include <ituGL/scene/Transform.h>
#include <ituGL/scene/SceneVisitor.h>
#include <cassert>

SceneModel::SceneModel(const std::string& name, std::shared_ptr<Model> model) : SceneNode(name), m_model(model)
{
}

SceneModel::SceneModel(const std::string& name, std::shared_ptr<Model> model, glm::vec3 aabbBoundsMin, glm::vec3 aabbBoundsMax) 
    : SceneNode(name, aabbBoundsMin, aabbBoundsMax), m_model(model)
{
}

SceneModel::SceneModel(const std::string& name, std::shared_ptr<Model> model, std::shared_ptr<Transform> transform, glm::vec3 aabbBoundsMin, glm::vec3 aabbBoundsMax) 
    : SceneNode(name, aabbBoundsMin, aabbBoundsMax), m_model(model)
{
}

std::shared_ptr<Model> SceneModel::GetModel() const
{
    return m_model;
}

void SceneModel::SetModel(std::shared_ptr<Model> model)
{
    m_model = model;
}

/*glm::mat4 SceneModel::GetWorldMatrix() const
{
    return m_transform ? m_transform->GetTransformMatrix() : glm::mat4(1.0f);
}

int SceneModel::GetDrawcallCount() const
{
    return m_model ? m_model->GetMesh().GetSubmeshCount() : 0;
}

const Drawcall& SceneModel::GetDrawcall(int index, const VertexArrayObject*& vao, const Material* &material) const
{
    assert(m_model);
    material = &m_model->GetMaterial(index);
    const Mesh& mesh = m_model->GetMesh();
    vao = &mesh.GetSubmeshVertexArray(index);
    return mesh.GetSubmeshDrawcall(index);
}*/

SphereBounds SceneModel::GetSphereBounds() const
{
    return SphereBounds(GetBoxBounds());
}

AabbBounds SceneModel::GetAabbBounds() const
{
    return AabbBounds(GetBoxBounds());
}

BoxBounds SceneModel::GetBoxBounds() const
{
    assert(m_transform);
    assert(m_model);

    // Setting the AABB extents manually is a hacky way of doing this in lieu of m_model->GetSize().
    return BoxBounds(m_AABB_center, m_transform->GetRotationMatrix(), m_transform->GetScale() * m_AABB_extents);
}

glm::vec3 SceneModel::GetAabbExtents() const
{
    return m_AABB_extents;
}

void SceneModel::AcceptVisitor(SceneVisitor& visitor)
{
    visitor.VisitModel(*this);
    //visitor.VisitRenderable(*this);
}

void SceneModel::AcceptVisitor(SceneVisitor& visitor) const
{
    visitor.VisitModel(*this);
    //visitor.VisitRenderable(*this);
}
