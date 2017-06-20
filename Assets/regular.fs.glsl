#version 410

layout(location = 0) out vec4 fragColor;

uniform sampler2D tex;

in VS_OUT
{                                                             
	vec2 texcoord;                                              
} fs_in; 

void main()
{
    //float depthValue = texture(tex, fs_in.texcoord).r;
    //fragColor = vec4(vec3(depthValue), 1.0);
	vec3 tex_color = texture(tex, fs_in.texcoord).rgb;
	fragColor = vec4(tex_color, 1.0);
}