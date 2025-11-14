#version 430 core

const float IMPACT_SCALE      = 1000.0;
const float INV_IMPACT_SCALE  = 1.0 / IMPACT_SCALE;
const float WALL_AMPLITUDE    = 0.0000001;   // tweak for how “soft” walls are
const float PI                = 3.14159265359;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

// From C++: center and half extents of the cube (after scaling)
uniform vec3 boundaryCenter;
uniform vec3 boundaryHalf;

// Accumulated impacts from compute shader
layout(std430, binding = 9) buffer WallImpacts
{
    uint wallImpact[4]; // 0:+X, 1:-X, 2:+Z, 3:-Z
};

void main()
{
    // Work in object space (same space as aPos)
    vec3 pos = aPos;

    // Compute rel position wrt cube center in object space
    vec3 rel = pos - boundaryCenter;

    // Decide which side wall this vertex belongs to (like we did for particles)
    float ax = abs(rel.x) / boundaryHalf.x;
    float ay = abs(rel.y) / boundaryHalf.y;
    float az = abs(rel.z) / boundaryHalf.z;

    int face = -1;
    vec3 wallNormal = vec3(0.0);

    if (ax >= ay && ax >= az) {
        // X walls
        if (rel.x > 0.0) {
            face = 0;                // +X
            wallNormal = vec3(1,0,0);
        } else {
            face = 1;                // -X
            wallNormal = vec3(-1,0,0);
        }
    } else if (az >= ax && az >= ay) {
        // Z walls
        if (rel.z > 0.0) {
            face = 2;                // +Z
            wallNormal = vec3(0,0,1);
        } else {
            face = 3;                // -Z
            wallNormal = vec3(0,0,-1);
        }
    }

    // Default: no displacement
    float displacement = 0.0;

    if (face >= 0) {
        // Map this vertex to (u,v) on its wall for smooth shape
        float u, v;
        if (face == 0 || face == 1) {
            // ±X walls: use (z,y)
            u = (pos.z - (boundaryCenter.z - boundaryHalf.z)) / (2.0 * boundaryHalf.z);
            v = (pos.y - (boundaryCenter.y - boundaryHalf.y)) / (2.0 * boundaryHalf.y);
        } else {
            // ±Z walls: use (x,y)
            u = (pos.x - (boundaryCenter.x - boundaryHalf.x)) / (2.0 * boundaryHalf.x);
            v = (pos.y - (boundaryCenter.y - boundaryHalf.y)) / (2.0 * boundaryHalf.y);
        }

        u = clamp(u, 0.0, 1.0);
        v = clamp(v, 0.0, 1.0);

        float amp = float(wallImpact[face]) * INV_IMPACT_SCALE;

        // optional damping so it doesn't explode visually
        amp = amp * WALL_AMPLITUDE;  // tweak this constant

        // Smooth "bump" shape on the wall: peak in center, zero at edges
        float s = sin(PI * u) * sin(PI * v);

        // This s is effectively your "smoothed by neighbors" effect
        displacement = amp * s;
    }

    // Move vertex along its normal. We can choose:
    // - aNormal (geometry normal)
    // - wallNormal (face normal)
    // Using wallNormal gives cleaner result for flat walls.
    vec3 N = normalize(wallNormal); // or normalize(aNormal);
    vec3 displacedPos = pos + N * displacement;

    gl_Position = projection * view * model * vec4(displacedPos, 1.0);
}
