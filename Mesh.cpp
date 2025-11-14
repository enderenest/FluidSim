#include "mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <limits>
#include <iostream>
#include <stdexcept>

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

void Mesh::bindForCompute(GLuint positionBinding,
    GLuint normalBinding,
    GLuint triangleBinding) const
{
    _gpuPositions.bindTo(positionBinding);
    _gpuNormals.bindTo(normalBinding);
    _gpuTriangles.bindTo(triangleBinding);
}
