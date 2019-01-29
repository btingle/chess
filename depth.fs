#version 330 core

out vec4 frag_color;

void main()
{
	float depth = gl_FragCoord.z;
	frag_color = vec4(depth, depth, depth, 1.0);
}