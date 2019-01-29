#version 330

layout (location = 0) in vec3 pos_in;
layout (location = 1) in vec3 norm_in;
layout (location = 2) in vec2 tex_in;

out vec2 tex_fs;
out vec3 norm_fs;
out vec3 frag_pos;
out vec4 frag_pos_lightspace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 light_view;
uniform mat4 light_proj;

void main()
{
	norm_fs = norm_in;
	tex_fs = tex_in;
	frag_pos = vec3(model * vec4(pos_in, 1.0));
	frag_pos_lightspace = light_proj * light_view * vec4(frag_pos, 1.0);
	gl_Position = projection * view * model * vec4(pos_in, 1.0);
}