#include "mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <limits>
#include <iostream>
#include <stdexcept>

Mesh::Mesh()
    : _gpuPositions(0)   // count = 0 -> GL buffer of size 0
    , _gpuNormals(0)
    , _gpuTriangles(0)
{
    // Other members (_vao, _vbo..., _minBounds, _maxBounds, vectors)
    // are already initialized by their in-class initializers / default ctors.
}

bool Mesh::loadFromFile(const std::string& filePath)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        filePath.c_str(),
        aiProcess_Triangulate
        | aiProcess_JoinIdenticalVertices
        | aiProcess_GenNormals
        | aiProcess_ImproveCacheLocality
        | aiProcess_PreTransformVertices
    );

    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0) {
        std::cerr << "Assimp error loading '" << filePath << "': "
            << importer.GetErrorString() << std::endl;
        return false;
    }

    _positions.clear();
    _normals.clear();
    _indices.clear();

    // Reserved to expected numbers to avoid frequent resize in vector
    _positions.reserve(25000);
    _normals.reserve(25000);
    _indices.reserve(50000);

    std::size_t vertexOffset = 0;

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const aiMesh* srcMesh = scene->mMeshes[meshIndex];

        // --- vertices ---
        for (unsigned int vertexIndex = 0; vertexIndex < srcMesh->mNumVertices; ++vertexIndex) {
            const aiVector3D& assimpPos = srcMesh->mVertices[vertexIndex];

            glm::vec3 position(assimpPos.x, assimpPos.y, assimpPos.z);
            _positions.push_back(position);

            glm::vec3 normal(0.0f, 0.0f, 1.0f);
            if (srcMesh->mNormals) {
                const aiVector3D& assimpNormal = srcMesh->mNormals[vertexIndex];
                normal = glm::normalize(glm::vec3(assimpNormal.x, assimpNormal.y, assimpNormal.z));
            }
            _normals.push_back(normal);
        }

        // --- indices (faces as triangles) ---
        for (unsigned int faceIndex = 0; faceIndex < srcMesh->mNumFaces; ++faceIndex) {
            const aiFace& face = srcMesh->mFaces[faceIndex];
            if (face.mNumIndices != 3) {
                continue;
            }

            _indices.push_back(static_cast<uint32_t>(vertexOffset + face.mIndices[0]));
            _indices.push_back(static_cast<uint32_t>(vertexOffset + face.mIndices[1]));
            _indices.push_back(static_cast<uint32_t>(vertexOffset + face.mIndices[2]));
        }

        vertexOffset += srcMesh->mNumVertices;
    }

    // Sizes should match
    if (_positions.size() != _normals.size()) {
        std::cerr << "Mesh warning: positions/normals size mismatch.\n";
    }

    computeBounds();

    std::cout << "Loaded mesh '" << filePath << "' with "
        << _positions.size() << " vertices, "
        << _indices.size() / 3 << " triangles.\n";

    return true;
}

void Mesh::computeBounds()
{
    if (_positions.empty()) {
        _minBounds = glm::vec3(0.0f);
        _maxBounds = glm::vec3(0.0f);
        return;
    }

    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(-std::numeric_limits<float>::max());

    for (const glm::vec3& position : _positions) {
        minBounds = glm::min(minBounds, position);
        maxBounds = glm::max(maxBounds, position);
    }

    _minBounds = minBounds;
    _maxBounds = maxBounds;
}

void Mesh::uploadToGPU()
{
    // CPU to GPU: vec3 -> vec4 for positions and normals
    std::vector<glm::vec4> gpuPositions(_positions.size());
    std::vector<glm::vec4> gpuNormals(_normals.size());

    for (std::size_t i = 0; i < _positions.size(); ++i) {
        gpuPositions[i] = glm::vec4(_positions[i], 0.0f);
    }
    for (std::size_t i = 0; i < _normals.size(); ++i) {
        gpuNormals[i] = glm::vec4(_normals[i], 0.0f);
    }

    std::vector<glm::uvec3> gpuTriangles(_indices.size() / 3);
    for (std::size_t triangleIndex = 0; triangleIndex < gpuTriangles.size(); ++triangleIndex) {
        gpuTriangles[triangleIndex] = glm::uvec3(
            _indices[3 * triangleIndex + 0],
            _indices[3 * triangleIndex + 1],
            _indices[3 * triangleIndex + 2]
        );
    }

    _gpuPositions.upload(gpuPositions);
    _gpuNormals.upload(gpuNormals);
    _gpuTriangles.upload(gpuTriangles);
}

void Mesh::scale(const glm::vec3& scale)
{
    for (glm::vec3& p : _positions) {
        p.x *= scale.x;
        p.y *= scale.y;
        p.z *= scale.z;
    }

    computeBounds();
}

void Mesh::bindForCompute(GLuint positionBinding,
    GLuint normalBinding,
    GLuint triangleBinding) const
{
    _gpuPositions.bindTo(positionBinding);
    _gpuNormals.bindTo(normalBinding);
    _gpuTriangles.bindTo(triangleBinding);
}

void Mesh::createDebugGLObjects()
{
    if (_vao != 0) return; // already created

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vboPositions);
    glGenBuffers(1, &_vboNormals);
    glGenBuffers(1, &_eboIndices);

    glBindVertexArray(_vao);

    // --- Positions ---
    glBindBuffer(GL_ARRAY_BUFFER, _vboPositions);
    glBufferData(GL_ARRAY_BUFFER,
        _positions.size() * sizeof(glm::vec3),
        _positions.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // location = 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // --- Normals ---
    glBindBuffer(GL_ARRAY_BUFFER, _vboNormals);
    glBufferData(GL_ARRAY_BUFFER,
        _normals.size() * sizeof(glm::vec3),
        _normals.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(1); // location = 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // --- Indices ---
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _eboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        _indices.size() * sizeof(uint32_t),
        _indices.data(),
        GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Mesh::drawTriangles() const
{
    if (_vao == 0) return;

    glBindVertexArray(_vao);

    // To see triangle edges: wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glDrawElements(GL_TRIANGLES,
        static_cast<GLsizei>(_indices.size()),
        GL_UNSIGNED_INT,
        (void*)0);

    // Reset if you changed polygon mode globally
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
}

void Mesh::drawVertices() const
{
    if (_vao == 0) return;

    glBindVertexArray(_vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(_positions.size()));
    glBindVertexArray(0);
}
