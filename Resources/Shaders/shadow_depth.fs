#version 330

out vec4 finalColor;

void main()
{
    float depth = gl_FragCoord.z;
    finalColor = vec4(vec3(depth), 1.0);
}