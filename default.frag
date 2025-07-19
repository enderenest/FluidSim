#version 430 core

in vec2 texCoord;
in vec3 velocity;

out vec4 FragColor;

vec3 velocityToColor(float t)
{
    if (t < 0.25)
        return mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), smoothstep(0.0, 0.25, t));        
    else if (t < 0.5)
        return mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), smoothstep(0.25, 0.5, t)); 
    else if (t < 0.75)
        return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), smoothstep(0.5, 0.75, t));  
    else
        return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), smoothstep(0.75, 1.0, t)); 
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