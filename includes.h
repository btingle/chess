#pragma once

#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif

#define _CRT_SECURE_NO_WARNINGS

#include "glad.h"
#include "wglext.h"
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cglm/cglm.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
}material;