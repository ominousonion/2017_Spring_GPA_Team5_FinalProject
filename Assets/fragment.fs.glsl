#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

uniform sampler2D tex;
uniform float alpha;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;

void main()
{
	float theta = max(dot(vertexData.N,vertexData.L), 0.0);
	float phi =  max(dot(vertexData.N,vertexData.H), 0.0);
	vec3 texColor = texture(tex, vertexData.texcoord).rgb;
	vec3 ambient = texColor * Ka;
	vec3 diffuse = texColor * Kd * theta;
	vec3 specular = vec3(1,1,1) * Ks * pow(phi, 1000);
	fragColor = vec4(ambient + diffuse + specular, alpha);
}