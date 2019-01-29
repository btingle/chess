#version 330

layout (location = 0) in vec3 pos_in;

uniform mat4 light_view;
uniform mat4 light_proj;
uniform mat4 model;

void main()
{
	gl_Position = light_proj * light_view * model * vec4(pos_in, 1.0);
}