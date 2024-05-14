#include <ituGL/texture/Texture2DArrayObject.h>

#include <cassert>

Texture2DArrayObject::Texture2DArrayObject()
{
}

template <>
void Texture2DArrayObject::SetImage<std::byte>(GLint level, GLsizei width, GLsizei height, GLsizei depth, Format format, InternalFormat internalFormat, std::span<const std::byte> data, Data::Type type)
{
    assert(IsBound());
    assert(data.empty() || type != Data::Type::None);
    assert(IsValidFormat(format, internalFormat));
    assert(data.empty() || data.size_bytes() == width * height * depth * GetDataComponentCount(internalFormat) * Data::GetTypeSize(type));
    glTexImage3D(GetTarget(), level, internalFormat, width, height, depth, 0, format, static_cast<GLenum>(type), data.data());
}

void Texture2DArrayObject::SetImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, Format format, InternalFormat internalFormat)
{
    SetImage<float>(level, width, height, depth, format, internalFormat, std::span<float>());
}
