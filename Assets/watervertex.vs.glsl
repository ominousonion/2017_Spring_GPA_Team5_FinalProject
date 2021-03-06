#version 410

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform mat4 um4shadow;
uniform vec3 uv3lightpos;

out VertexData
{
	vec4 shadow_coord;
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

void main()
{
	vec4 P = um4mv * vec4(iv3vertex, 1.0);

	gl_Position = um4p * um4mv * vec4(iv3vertex, 1.0);
    vertexData.texcoord = iv2tex_coord;

	vec3 V = -P.xyz;
	vertexData.N = normalize(mat3(um4mv) * iv3normal);
	vertexData.L = normalize(uv3lightpos + V);
	vertexData.H = normalize(vertexData.L + V);
	vertexData.shadow_coord = um4shadow * vec4(iv3vertex, 1.0);
}