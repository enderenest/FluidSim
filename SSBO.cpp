#include "SSBO.h"
#include <cassert>

// Constructor: generate and allocate GPU buffer storage
template<typename T>
SSBO<T>::SSBO(size_t count, GLenum usage)
    : _count(count)
{
    glGenBuffers(1, &_handle);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _handle);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        _count * sizeof(T),
        nullptr,         // no initial data
        usage
    );
}

// Destructor: delete the GPU buffer
template<typename T>
SSBO<T>::~SSBO() {
    glDeleteBuffers(1, &_handle);
}

// Upload data via glBufferSubData
template<typename T>
void SSBO<T>::upload(const std::vector<T>& data) {
    assert(data.size() == _count);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _handle);
    glBufferSubData(
        GL_SHADER_STORAGE_BUFFER,
        0,
        _count * sizeof(T),
        data.data()
    );
}

// Bind buffer to a shader storage binding point
template<typename T>
void SSBO<T>::bindTo(GLuint bindingIndex) const {
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        bindingIndex,
        _handle
    );
}

// Map buffer range for direct access
template<typename T>
T* SSBO<T>::map(GLbitfield access) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _handle);
    return static_cast<T*>(
        glMapBufferRange(
            GL_SHADER_STORAGE_BUFFER,
            0,
            _count * sizeof(T),
            access
        )
        );
}

// Unmap buffer after mapping
template<typename T>
void SSBO<T>::unmap() {
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

// Return element count
template<typename T>
size_t SSBO<T>::count() const {
    return _count;
}