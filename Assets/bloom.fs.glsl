#version 410

layout(location = 0) out vec4 fragColor;

uniform sampler2D tex;
uniform float bar;

in VS_OUT
{                                                             
	vec2 texcoord;                                              
} fs_in; 

void main()
{
		float weight = 1.5;
		int half_size = 2;
		int sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);

		vec3 color_sum = vec3(0);

		for (int i = -half_size; i <= half_size ; ++i){
			for (int j = -half_size; j <= half_size ; ++j){
				ivec2 coord = ivec2(gl_FragCoord.xy) + ivec2(i, j);

				vec3 color_sum_first = vec3(0);

				for(int x = -half_size; x <= half_size ; ++x){
					for(int y = -half_size; y <= half_size ; ++y){
						ivec2 coord_first = coord + ivec2(x, y);

						color_sum_first += texelFetch(tex, coord_first, 0).rgb;
					}
				}
				color_sum_first /= sample_count;

				color_sum += color_sum_first;
			}
		}
	
		vec3 color = color_sum / sample_count;

		vec3 tex_color = texelFetch(tex, ivec2(gl_FragCoord.xy), 0).rgb;
	
		fragColor = vec4(color * tex_color * weight, 1.0);	
}