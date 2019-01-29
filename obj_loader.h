#include "includes.h"

float* load_obj(const char* filename, int* data_length, int* vert_count)
{
	char   header[128];
	int    vert_alloc = 1024, norm_alloc = 1024, text_alloc = 1024, indx_alloc = 2048;
	int    vi = 0, ni = 0, ti = 0; // for vert/norm/text data indexing
	int    ii = 0; // for index indexing
	int    result, matches;
	float* vert_data = (float*)malloc(vert_alloc * sizeof(float));
	float* norm_data = (float*)malloc(norm_alloc * sizeof(float));
	float* text_data = (float*)malloc(text_alloc * sizeof(float));
	int*   indx_data = (float*)malloc(indx_alloc * sizeof(int));
	float* obj_data;
	FILE*  objf = fopen(filename, "r");

	if (!objf)
	{
		printf("Coudln't open .obj file");
		return 0;
	}

	while (true)
	{
		result = fscanf(objf, "%s", header);
		
		if(result == EOF)
			break;

		if (!strcmp(header, "v"))
		{
			fscanf(objf, "%f %f %f\n", (vert_data + vi + 0), (vert_data + vi + 1), (vert_data + vi + 2));
			vi += 3;
		}
		else if (!strcmp(header, "vn"))
		{
			fscanf(objf, "%f %f %f\n", (norm_data + ni + 0), (norm_data + ni + 1), (norm_data + ni + 2));
			ni += 3;
		}
		else if (!strcmp(header, "vt"))
		{
			fscanf(objf, "%f %f", (text_data + ti + 0), (text_data + ti + 1));
			text_data[ti + 1] = 1.f - text_data[ti + 1];
			ti += 2;
		}
		else if (!strcmp(header, "f"))
		{
			matches = fscanf(objf, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
				(indx_data + ii + 0), (indx_data + ii + 1), (indx_data + ii + 2),
				(indx_data + ii + 3), (indx_data + ii + 4), (indx_data + ii + 5),
				(indx_data + ii + 6), (indx_data + ii + 7), (indx_data + ii + 8));

			if (matches != 9)
			{
				printf("Unsupported .obj format, must include vertices, normals, and uv coordinates");
			}

			ii += 9;
		}
		else
		{
			fscanf(objf, "%*[^\n]\n", NULL);
		}

		if (vi >= vert_alloc - 4)
		{
			vert_alloc *= 2;
			vert_data = (float*)realloc(vert_data, vert_alloc * sizeof(float));
		}
		if (ni >= norm_alloc - 4)
		{
			norm_alloc *= 2;
			norm_data = (float*)realloc(norm_data, norm_alloc * sizeof(float));
		}
		if (ti >= text_alloc - 3)
		{
			text_alloc *= 2;
			text_data = (float*)realloc(text_data, text_alloc * sizeof(float));
		}
		if (ii >= indx_alloc - 9)
		{
			indx_alloc *= 2;
			indx_data = (int*)realloc(indx_data, indx_alloc * sizeof(int));
		}
	}

	fclose(objf);

	obj_data = (float*)malloc((ii / 3) * 8 * sizeof(float)); // 8 floats per piece of vertex data (3 index values = 1 vertex)
	*vert_count  = (ii / 3);
	*data_length = (ii / 3) * 8 * sizeof(float);

	int j = 0, i = 0, index;
	for (; i < ii; i += 3)
	{
		index = (indx_data[i + 0] - 1) * 3;
		obj_data[j + 0] = vert_data[index + 0];
		obj_data[j + 1] = vert_data[index + 1];
		obj_data[j + 2] = vert_data[index + 2];
		index = (indx_data[i + 1] - 1) * 2;
		obj_data[j + 6] = text_data[index + 0];
		obj_data[j + 7] = text_data[index + 1];
		index = (indx_data[i + 2] - 1) * 3;
		obj_data[j + 3] = norm_data[index + 0];
		obj_data[j + 4] = norm_data[index + 1];
		obj_data[j + 5] = norm_data[index + 2];
		j += 8;
	}

	free(vert_data);
	free(norm_data);
	free(text_data);
	free(indx_data);

	return obj_data;
}