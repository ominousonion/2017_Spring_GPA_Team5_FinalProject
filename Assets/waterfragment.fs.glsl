#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
	vec4 shadow_coord;
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

uniform sampler2D tex;

void main()
{
	float theta = max(dot(vertexData.N,vertexData.L), 0.0);
	float phi =  max(dot(vertexData.N,vertexData.H), 0.0);
	vec3 texColor = texture(tex, vertexData.texcoord).rgb;
	if(texColor == vec3(0))
	{
		discard;
	}
	fragColor = vec4(texColor, 0.7);
	
}