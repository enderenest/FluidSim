#ifndef MESH_CLASS_H  

#define MESH_CLASS_H  

#include <vector>
#include <string>
#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glad/glad.h>

// Include your SSBO wrapper header
#include "SSBO.hpp"

class Mesh {
public:
    Mesh();

    // Load mesh data from file using Assimp.
    // Returns true on success, false on failure.
    bool loadFromFile(const std::string& path);

    // Upload current CPU data to GPU SSBOs.
    void uploadToGPU();

    // Bind SSBOs to given binding points (for compute shaders).
    void bindForCompute(GLuint posBinding,
        GLuint normBinding,
        GLuint triBinding) const;

    // --- Getters ---

    const std::vector<glm::vec3>& positions() const { return _positions; }
    const std::vector<glm::vec3>& normals() const { return _normals; }
    const std::vector<uint32_t>& indices() const { return _indices; }

    glm::vec3 minBounds() const { return _minBounds; }
    glm::vec3 maxBounds() const { return _maxBounds; }
    glm::vec3 center() const { return 0.5f * (_minBounds + _maxBounds); }

    std::size_t vertexCount() const { return _positions.size(); }
    std::size_t triangleCount() const { return _indices.size() / 3; }

    void scale(const glm::vec3& scale);

    void createDebugGLObjects();   // create VAO/VBO/EBO for rendering
    void drawTriangles() const;    // draw filled or wireframe triangles
    void drawVertices() const;     // draw points at each vertex

private:
    GLuint _vao = 0;
    GLuint _vboPositions = 0;
    GLuint _vboNormals = 0;
    GLuint _eboIndices = 0;

    void computeBounds();

    // CPU-side data
    std::vector<glm::vec3> _positions;
    std::vector<glm::vec3> _normals;
    std::vector<uint32_t>  _indices;

    glm::vec3 _minBounds{ 0.0f };
    glm::vec3 _maxBounds{ 0.0f };

    // GPU-side SSBOs
    SSBO<glm::vec4>  _gpuPositions;
    SSBO<glm::vec4>  _gpuNormals;
    SSBO<glm::uvec3> _gpuTriangles;
};


#endif // MESH_CLASS_H