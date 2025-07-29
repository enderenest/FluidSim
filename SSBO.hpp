#ifndef SSBO_CLASS_H
#define SSBO_CLASS_H

#include <vector>
#include <cstddef>
#include <glad/glad.h>
#include <cassert>

template<typename T>
class SSBO {
public:
    // Construct an SSBO for 'count' elements, with optional usage hint
    SSBO(size_t count, GLenum usage = GL_DYNAMIC_DRAW)
        : _count(count)
    {
        glGenBuffers(1, &_id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            _count * sizeof(T),
            nullptr,         // no initial data
            usage
        );
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    // Destructor: deletes the GPU buffer
    ~SSBO() {
        glDeleteBuffers(1, &_id);
    }

    void resize(size_t newCount) {
        _count = newCount;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            _count * sizeof(T),
            nullptr,
            GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    // Upload a full vector of data to the GPU buffer
    void upload(const std::vector<T>& data) {
        if (data.size() != _count) {
            _count = data.size();
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
            glBufferData(GL_SHADER_STORAGE_BUFFER, _count * sizeof(T), data.data(), GL_DYNAMIC_DRAW);
        }
        else {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, _count * sizeof(T), data.data());
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    // Bind this SSBO to the given binding point in GLSL
    void bindTo(GLuint bindingIndex) const {
        glBindBufferBase(
            GL_SHADER_STORAGE_BUFFER,
            bindingIndex,
            _id
        );
    };

    // Map the buffer into client memory for direct access
    // 'access' is a GLbitfield like GL_WRITE_ONLY or GL_READ_WRITE
    T* map(GLbitfield access = GL_WRITE_ONLY) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
        return static_cast<T*>(
            glMapBufferRange(
                GL_SHADER_STORAGE_BUFFER,
                0,
                _count * sizeof(T),
                access
            )
        );
    }

    // Unmap after mapping
    void unmap() {
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    // Get the number of elements
    size_t count() const {
        return _count;
    }

    GLuint getID() const {
        return _id;
	}

private:
    GLuint _id;   // OpenGL buffer handle
    size_t _count;    // Number of T elements
};

#endif
