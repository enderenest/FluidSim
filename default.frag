#version 330 core

in vec2 uv;
in float speedT;
out vec4 FragColor;

uniform sampler1D speedColorMap;

void main()
{
    vec2 centreOffset = (uv - 0.5) * 2.0;
    float sqrDstFromCentre = dot(centreOffset, centreOffset);
    float delta = fwidth(sqrt(sqrDstFromCentre));
    float circleAlpha = 1.0 - smoothstep(1.0 - delta, 1.0 + delta, sqrDstFromCentre);

    vec3 color = texture(speedColorMap, speedT).rgb;
    FragColor = vec4(color, circleAlpha);
}
