#version 410

in vec3 TexCoords;

uniform samplerCube skybox;

layout(location = 0) out vec4 fragColor;

void main(void)
{	
	fragColor = texture(skybox, TexCoords);
}