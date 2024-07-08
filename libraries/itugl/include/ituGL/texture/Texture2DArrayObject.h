#pragma once

#include <ituGL/texture/TextureObject.h>
#include <ituGL/core/Data.h>

// Texture object in 3 dimensions
class Texture2DArrayObject : public TextureObjectBase<TextureObject::Texture2DArray>
{
public:
    Texture2DArrayObject();

    // Initialize the texture2D with a specific format
    void SetImage(GLint level,
        GLsizei width, GLsizei height, GLsizei depth,
        Format format, InternalFormat internalFormat);

    // Initialize the texture2D with a specific format and initial data
    template <typename T>
    void SetImage(GLint level,
        GLsizei width, GLsizei height, GLsizei depth,
        Format format, InternalFormat internalFormat,
        std::span<const T> data, Data::Type type = Data::Type::None);
};

// Set image with data in bytes
template <>
void Texture2DArrayObject::SetImage<std::byte>(GLint level, GLsizei width, GLsizei height, GLsizei depth, Format format, InternalFormat internalFormat, std::span<const std::byte> data, Data::Type type);

// Template method to set image with any kind of data
template <typename T>
inline void Texture2DArrayObject::SetImage(GLint level, GLsizei width, GLsizei height, GLsizei depth,
    Format format, InternalFormat internalFormat, std::span<const T> data, Data::Type type)
{
    if (type == Data::Type::None)
    {
        type = Data::GetType<T>();
    }
    SetImage(level, width, height, depth, format, internalFormat, Data::GetBytes(data), type);
}