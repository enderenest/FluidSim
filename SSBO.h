#ifndef SSBO_CLASS_H
#define SSBO_CLASS_H

#include <vector>
#include <cstddef>
#include <glad/glad.h>

template<typename T>
class SSBO {
public:
    // Construct an SSBO for 'count' elements, with optional usage hint
    SSBO(size_t count, GLenum usage = GL_DYNAMIC_DRAW);

    // Destructor: deletes the GPU buffer
    ~SSBO();

    // Upload a full vector of data to the GPU buffer
    void upload(const std::vector<T>& data);

    // Bind this SSBO to the given binding point in GLSL
    void bindTo(GLuint bindingIndex) const;

    // Map the buffer into client memory for direct access
    // 'access' is a GLbitfield like GL_WRITE_ONLY or GL_READ_WRITE
    T* map(GLbitfield access = GL_WRITE_ONLY);

    // Unmap after mapping
    void unmap();

    // Get the number of elements
    size_t count() const;

private:
    GLuint _handle;   // OpenGL buffer handle
    size_t _count;    // Number of T elements
};

#endif
