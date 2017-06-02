#version 410

layout (location = 0) in vec3 position;

out vec3 TexCoords;

uniform mat4 vp_matrix;
uniform vec3 eye;

void main(void)
{
	vec4 tc = inverse(vp_matrix) * vec4(position, 1.0);
    gl_Position = vec4(position, 1.0);
    TexCoords = tc.xyz/tc.w - eye;
}