#version 330 core
out vec4 frag_color;

in vec2 tex_fs;
in vec3 norm_fs;
in vec3 frag_pos;
in vec4 frag_pos_lightspace;

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform Material material;
uniform int selected;
uniform float time;
uniform float shininess;
uniform vec3 view_pos;
uniform vec3 light_pos;
uniform sampler2D tex;
uniform sampler2D depth_map;

float shadow()
{
	vec3 proj_coords = frag_pos_lightspace.xyz / frag_pos_lightspace.w;
	proj_coords = proj_coords * 0.5 + 0.5;
	float shadow, closest_depth;
	for(int y = -1; y <= 1; y++)
	{
		for(int x = -1; x <= 1; x++)
		{
			closest_depth = texture(depth_map, proj_coords.xy + vec2(x*0.001, y*0.001)).r ;
			shadow += (proj_coords.z - 0.002) > closest_depth ? 1.0 : 0.0;
		}
	}
	shadow /= 9;
	return shadow;
}

void main()
{
	vec3 color = texture(tex, tex_fs).rgb;
	vec3 ambient = material.ambient * color;
	vec3 normal = normalize(norm_fs);
	vec3 light_dir = normalize(light_pos - frag_pos);
	float diff = max(dot(normal, light_dir), 0);
	vec3 diffuse = material.diffuse * diff * color;
	vec3 view_dir = normalize(view_pos - frag_pos);
	vec3 reflect_dir = reflect(-light_dir, normal);
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
	vec3 specular = material.specular * spec * vec3(1.0, 1.0, 1.0); // light color
	
	frag_color = vec4(ambient + ((1.0 - shadow()) * (diffuse + specular)), 1.0);
	if(selected == 1)
	{
		frag_color = mix(frag_color, vec4(1.0, 1.0, 1.0, 1.0), time);
	}
	else if(selected == 2)
	{
		frag_color = mix(frag_color, vec4(1.0, 0, 0, 1.0), 0.5);
	}
}