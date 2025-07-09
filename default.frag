#version 330 core

in vec2 texCoord;
in vec3 velocity;

out vec4 FragColor;

vec3 velocityToColor(float t)
{
    // t in [0, 1]
    if (t < 0.25)
        return mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), t / 0.25);        // Blue ? Cyan
    else if (t < 0.5)
        return mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), (t - 0.25) / 0.25); // Cyan ? Green
    else if (t < 0.75)
        return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (t - 0.5) / 0.25);  // Green ? Yellow
    else
        return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), (t - 0.75) / 0.25); // Yellow ? Red
}

void main()
{
    // Calculate soft circle mask
    vec2 centreOffset = (texCoord - vec2(0.5)) * 2.0;
    float distSquared = dot(centreOffset, centreOffset);
    float delta = fwidth(sqrt(distSquared));
    float circleAlpha = 1.0 - smoothstep(1.0 - delta, 1.0 + delta, distSquared);

    // Map velocity to speed-based color
    float speed = length(velocity);
    float t = clamp(speed / 3.0, 0.0, 1.0);  // Adjust divisor (3.0) based on your expected max speed
    vec3 color = velocityToColor(t);

    FragColor = vec4(color, circleAlpha);
}