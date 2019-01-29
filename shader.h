#pragma once
#include "includes.h"

bool check_compile_errors(GLuint shader, const char* type)
{
	GLint success;
	GLchar info_log[1024];
	if (type != "PROGRAM")
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 1024, NULL, info_log);
			printf("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n -- -----------------------------------------------------\n", type, info_log);
			return false;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shader, 1024, NULL, info_log);
			printf("ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n -- -----------------------------------------------------\n", type, info_log);
			return false;
		}
	}
	return true;
}

GLuint compile_and_link_shader(const char* vert_path, const char* frag_path)
{
	FILE* vertf = fopen(vert_path, "rb");
	FILE* fragf = fopen(frag_path, "rb");
	int vertfsize, fragfsize;
	char *vert_code, *frag_code;
	GLuint vert, frag, shader_id;
	bool compile_error = false;

	fseek(vertf, 0, SEEK_END);
	vertfsize = ftell(vertf);
	rewind(vertf);

	fseek(fragf, 0, SEEK_END);
	fragfsize = ftell(fragf);
	rewind(fragf);

	vert_code = (char*)malloc((vertfsize + 1) * sizeof(char));
	frag_code = (char*)malloc((fragfsize + 1) * sizeof(char));

	if (!fread(vert_code, 1, vertfsize, vertf))
		return 0;
	if (!fread(frag_code, 1, fragfsize, fragf))
		return 0;
	fclose(vertf);
	fclose(fragf);

	vert_code[vertfsize] = '\0';
	frag_code[fragfsize] = '\0';

	vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vert_code, NULL);
	glCompileShader(vert);
	if (!check_compile_errors(vert, "VERTEX"))
		compile_error = true;

	frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &frag_code, NULL);
	glCompileShader(frag);
	if (!check_compile_errors(frag, "FRAGMENT"))
		compile_error = true;

	shader_id = glCreateProgram();
	glAttachShader(shader_id, vert);
	glAttachShader(shader_id, frag);
	glLinkProgram(shader_id);
	if (!check_compile_errors(shader_id, "PROGRAM"))
		compile_error = true;

	glDeleteShader(vert);
	glDeleteShader(frag);

	if (compile_error)
		return 0;

	free(vert_code);
	free(frag_code);

	return shader_id; 
}

GLuint get_uniform_loc(GLuint prog, const char* uniform_string)
{
	return glGetUniformLocation(prog, uniform_string);
}

void set_mat4(GLuint prog, const char* uniform_string, mat4 arg)
{
	GLuint uniform_loc = glGetUniformLocation(prog, uniform_string);
	glUniformMatrix4fv(uniform_loc, 1, GL_FALSE, arg[0]);
}

void set_int(GLuint prog, const char* uniform_string, int arg)
{
	GLuint uniform_loc = glGetUniformLocation(prog, uniform_string);
	glUniform1i(uniform_loc, arg);
}

void set_float(GLuint prog, const char* uniform_string, float arg)
{
	GLuint uniform_loc = glGetUniformLocation(prog, uniform_string);
	glUniform1f(uniform_loc, arg);
}

void set_vec3(GLuint prog, const char* uniform_string, vec3 arg)
{
	GLuint uniform_loc = glGetUniformLocation(prog, uniform_string);
	glUniform3fv(uniform_loc, 1, arg);
}

void set_material(GLuint prog, material arg)
{
	glUniform3fv(glGetUniformLocation(prog, "material.ambient"), 1, arg.ambient);
	glUniform3fv(glGetUniformLocation(prog, "material.diffuse"), 1, arg.diffuse);
	glUniform3fv(glGetUniformLocation(prog, "material.specular"), 1, arg.specular);
}