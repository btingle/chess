#include "includes.h"
#include "obj_loader.h"
#include "shader.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 960
#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024

/**************************
 *  VARIABLE DECLARATION  *
 **************************/

int screen_width, screen_height;

// openGL render-related variables
GLuint board_tex, frame_tex, p1_tex, p2_tex, depth_map;
GLuint board_VBO, frame_VBO, pawn_VBO, bishop_VBO, knight_VBO, king_VBO, queen_VBO, rook_VBO;
GLuint board_VAO, frame_VAO, pawn_VAO, bishop_VAO, knight_VAO, king_VAO, queen_VAO, rook_VAO;
GLuint board_vert_count, frame_vert_count, pawn_vert_count, bishop_vert_count, knight_vert_count, king_vert_count, queen_vert_count, rook_vert_count;
GLuint depth_map_FBO;
GLuint shader_prog, depth_shader_prog;
mat4 model;
mat4 view;
mat4 projection;
mat4 light_view;
mat4 light_proj;
vec3 light_pos;
float view_x, view_y, view_z; // viewing position magnitudes
float near_dist, far_dist; // clipping distances
vec3 view_pos;
float angle;
const mat4 identity = 
{ { 1.f, 0.f, 0.f, 0.f },
  { 0.f, 1.f, 0.f, 0.f },
  { 0.f, 0.f, 1.f, 0.f },
  { 0.f, 0.f, 0.f, 1.f } };
const material board_mtl =
{ { 0.5f, 0.5f, 0.5f },   // ambient
  { 0.8f, 0.8f, 0.8f },   // diffuse
  { 0.6f, 0.6f, 0.6f } }; // specular
const material frame_mtl =
{ { 0.5f, 0.5f, 0.5f },   // ambient
  { 0.8f, 0.8f, 0.8f },   // diffuse
  { 0.1f, 0.1f, 0.1f } }; // specular
const material piece_mtl =
{ { 0.4f, 0.4f, 0.4f },   // ambient
  { 0.7f, 0.7f, 0.7f },   // diffuse
  { 1.0f, 1.0f, 1.0f } }; // specular
bool shadow_debug;

// board mouse hover variables
typedef float vec2[2]; // cglm doesn't have vec2 type
void glm_vec2_copy(vec4 v, vec2 dest)
{
	dest[0] = v[0];
	dest[1] = v[1];
}
vec4 board_cursor_mesh [9][9];
vec4 transformed_cursor_mesh [9][9];
int select_pos[2];
int hover_pos[2];

// animation variables
int dest_pos [2];
int orig_pos [2];
vec3 orig_mpos;
vec3 dest_mpos;
vec3 curr_mpos;
vec3 orig_cpos;
vec3 dest_cpos;
int anim_lock;
float anim_time;
float anim_stop;

// chess variables
typedef enum pieces {pawn1, bishop1, knight1, king1, queen1, rook1, pawn2, bishop2, knight2, king2, queen2, rook2, empty} piece;
enum piece_types { pawn, bishop, knight, king, queen, rook };
piece chess_board [8][8] = 
{ { rook1, knight1, bishop1, queen1, king1, bishop1, knight1, rook1 },
  { pawn1, pawn1,   pawn1,   pawn1,  pawn1, pawn1,   pawn1,   pawn1 },
  { empty, empty,   empty,   empty,  empty, empty,   empty,   empty },
  { empty, empty,   empty,   empty,  empty, empty,   empty,   empty }, 
  { empty, empty,   empty,   empty,  empty, empty,   empty,   empty },
  { empty, empty,   empty,   empty,  empty, empty,   empty,   empty },
  { pawn2, pawn2,   pawn2,   pawn2,  pawn2, pawn2,   pawn2,   pawn2 },
  { rook2, knight2, bishop2, queen2, king2, bishop2, knight2, rook2 } };
piece animated_piece;

/****************************************
 *  CHESS BOARD SQUARE HOVER FUNCTIONS  *
 ****************************************/

void gen_hover_mesh()
{
	for (int y = 0; y < 9; y++)
		for (int x = 0; x < 9; x++)
			glm_vec4_copy((vec4){-4.f + x, 0.f, -4.f + y, 1.f}, board_cursor_mesh[y][x]);
}

void update_hover_mesh()
{
	mat4 MVP;
	vec4 transformed_vert;
	glm_mat4_mulN((mat4*[]) { &projection, &view, &model }, 3, MVP);
	for (int y = 0; y < 9; y++)
		for (int x = 0; x < 9; x++)
		{
			glm_mat4_mulv(MVP, board_cursor_mesh[y][x], transformed_vert);
			glm_vec4_divs(transformed_vert, transformed_vert[3], transformed_vert);
			glm_vec4_adds(transformed_vert, 1.f, transformed_vert);
			glm_vec4_divs(transformed_vert, 2.f, transformed_vert);
			glm_vec4_copy(transformed_vert, transformed_cursor_mesh[y][x]);
		};
}

// full disclosure: I don't exactly know how sign() and in_triangle() work, I just copy pasted them (whoops)
float sign(vec2 p1, vec2 p2, vec2 p3)
{
	return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
}

bool in_triangle(vec2 pt, vec2 v1, vec2 v2, vec2 v3)
{
	float d1, d2, d3;
	bool has_neg, has_pos;

	d1 = sign(pt, v1, v2);
	d2 = sign(pt, v2, v3);
	d3 = sign(pt, v3, v1);

	has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	return !(has_neg && has_pos);
}

void check_hover(double mouseX, double mouseY)
{
	vec2 mousepos = {mouseX / (float)screen_width, 1 - (mouseY / (float)screen_height)};
	vec2 tv1, tv2, tv3, tv4;
	for (int y = 0; y < 8; y++)
		for (int x = 0; x < 8; x++)
		{
			glm_vec2_copy(transformed_cursor_mesh[y+0][x+0], tv1);
			glm_vec2_copy(transformed_cursor_mesh[y+0][x+1], tv2);
			glm_vec2_copy(transformed_cursor_mesh[y+1][x+0], tv3);
			glm_vec2_copy(transformed_cursor_mesh[y+1][x+1], tv4);
			if (in_triangle(mousepos, tv1, tv2, tv3) || in_triangle(mousepos, tv2, tv3, tv4))
			{
				hover_pos[0] = x;
				hover_pos[1] = y;
				return;
			}
		}
}

/************************************
 *  RENDER AND ANIMATION FUNCTIONS  *
 ************************************/

void render_board(GLuint shader_target, bool shadow)
{
	glm_mat4_copy(identity, model);
	set_mat4(shader_target, "model", model);
	if (!shadow)
	{
		set_material(shader_target, board_mtl);
		set_int(shader_target, "tex", 0);
	}
	glBindVertexArray(board_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, board_VBO);
	glDrawArrays(GL_TRIANGLES, 0, board_vert_count);
	if (!shadow)
	{
		set_material(shader_target, frame_mtl);
		set_int(shader_target, "tex", 1);
	}
	glBindVertexArray(frame_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, frame_VBO);
	glDrawArrays(GL_TRIANGLES, 0, frame_vert_count);
}

void render_piece(GLuint shader_target, bool shadow, piece p)
{
	GLuint vert_count;
	switch (p < 6)
	{
	case true:
		if (!shadow)
			set_int(shader_target, "tex", 2);
		break;
	case false:
		if (!shadow)
			set_int(shader_target, "tex", 3);
		break;
	}
	switch (p % 6)
	{
	case pawn:
		glBindVertexArray(pawn_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, pawn_VBO);
		vert_count = pawn_vert_count;
		break;
	case bishop:
		glBindVertexArray(bishop_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, bishop_VBO);
		vert_count = bishop_vert_count;
		break;
	case knight:
		glBindVertexArray(knight_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, knight_VBO);
		vert_count = knight_vert_count;
		break;
	case queen:
		glBindVertexArray(queen_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, queen_VBO);
		vert_count = queen_vert_count;
		break;
	case king:
		glBindVertexArray(king_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, king_VBO);
		vert_count = king_vert_count;
		break;
	case rook:
		glBindVertexArray(rook_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, rook_VBO);
		vert_count = rook_vert_count;
		break;
	}
	set_mat4(shader_target, "model", model);
	glDrawArrays(GL_TRIANGLES, 0, vert_count);
	if (!shadow)
		set_int(shader_target, "selected", 0);
}

void render_pieces(GLuint shader_target, bool shadow)
{
	vec3 ppos;
	vec4 temp;
	if (!shadow)
	{
		set_int(shader_target, "tex", 1);
		set_material(shader_target, piece_mtl);
	}
	for (int y = 0; y < 8; y++)
		for (int x = 0; x < 8; x++)
			if (chess_board[y][x] != empty)
			{
				if (!shadow && x == hover_pos[0] && y == hover_pos[1])
					set_int(shader_target, "selected", 1);
				if (!shadow && x == select_pos[0] && y == select_pos[1])
					set_int(shader_target, "selected", 2);
				glm_mat4_copy(identity, model);
				glm_vec4_add(board_cursor_mesh[y + 0][x + 0], board_cursor_mesh[y + 1][x + 1], temp);
				glm_vec4_divs(temp, 2.f, temp);
				glm_vec3(temp, ppos);
				glm_translate(model, ppos);

				render_piece(shader_target, shadow, chess_board[y][x]);
			}
	glm_mat4_copy(identity, model);
}

void init_move_anim() // called by move_piece()
{
	vec4 temp;
	// both pieces are replaced with an empty square, but the piece that is moving is stored so it can be replaced after the animation
	animated_piece = chess_board[orig_pos[1]][orig_pos[0]];
	chess_board[orig_pos[1]][orig_pos[0]] = empty;
	chess_board[dest_pos[1]][dest_pos[0]] = empty;

	// gets the world coordinates for the centers of each of the tiles the pieces were standing on
	glm_vec4_add(board_cursor_mesh[orig_pos[1]][orig_pos[0]], board_cursor_mesh[orig_pos[1] + 1][orig_pos[0] + 1], temp);
	glm_vec4_divs(temp, 2.f, temp);
	glm_vec3(temp, orig_mpos);

	glm_vec4_add(board_cursor_mesh[dest_pos[1]][dest_pos[0]], board_cursor_mesh[dest_pos[1] + 1][dest_pos[0] + 1], temp);
	glm_vec4_divs(temp, 2.f, temp);
	glm_vec3(temp, dest_mpos);

	// curr_mpos holds the current world coordinates for the piece that is to be animated, at the start of the animation this is the piece's original position
	glm_vec3_copy(orig_mpos, curr_mpos);

	// camera animation has two stages, one where it pans in from origin to the piece, and a second where it pans from the piece to the origin
	glm_vec3_copy(orig_mpos, dest_cpos);
	glm_vec3_copy((vec3) { 0.f, 0.f, 0.f }, orig_cpos);

	// disables hover & select highlights for the duration of the animation
	hover_pos[0] = -1;
	hover_pos[1] = -1;
	select_pos[0] = -1;
	select_pos[1] = -1;

	anim_time = 0.f;
	// the time it takes for the piece to move should be dependent on the distance it travels
	anim_stop = glm_vec3_distance(orig_mpos, dest_mpos);
	anim_lock = 1;
}

#define MOVE_SPEED 0.07f
// interpolates between animation start and end points based on time, then moves animated piece to interpolated position
bool move_animated_piece()
{
	vec3 temp;
	bool final_pass = false;
	anim_time += MOVE_SPEED;
	if (anim_time >= anim_stop)
	{
		final_pass = true;
		anim_time = anim_stop;
	}
	glm_vec3_sub(dest_mpos, orig_mpos, temp);
	glm_vec3_muls(temp, anim_time / anim_stop, temp);
	glm_vec3_add(orig_mpos, temp, curr_mpos);
	glm_translate_to(identity, curr_mpos, model);

	// translates piece according to current position and focuses camera on it
	set_mat4(shader_prog, "model", model);
	glm_lookat(view_pos, curr_mpos, (vec3) { 0.f, 1.f, 0.f }, view);
	set_mat4(shader_prog, "view", view);
	return final_pass;
}

#define CAM_SPEED 0.03f
#define MAX_ZOOM 40.f
// pans and zooms in on animated piece over time
bool camera_slow_pan(bool in_out)
{
	vec3 temp;
	bool final_pass = false;
	anim_time += in_out ? CAM_SPEED : -CAM_SPEED;
	if (anim_time > 1.f || anim_time < 0.f)
	{
		final_pass = true;
		anim_time = in_out ? 1.f : 0.f;
	}
	glm_vec3_sub(dest_cpos, orig_cpos, temp);
	glm_vec3_muls(temp, sin((anim_time / 1.f) * M_PI_2), temp); // gives a funky camera acceleration
	glm_vec3_add(orig_cpos, temp, temp);

	glm_lookat(view_pos, temp, (vec3) { 0.f, 1.f, 0.f }, view);
	set_mat4(shader_prog, "view", view);
	glm_perspective(glm_rad(MAX_ZOOM + (45.f - MAX_ZOOM) * (1 - (anim_time / 1.f))), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, near_dist, far_dist, projection);
	set_mat4(shader_prog, "projection", projection);
	return final_pass;
}

void render_shadows()
{
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glUseProgram(depth_shader_prog);
	if (!shadow_debug)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, depth_map_FBO);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	render_board(depth_shader_prog, true);
	render_pieces(depth_shader_prog, true);
	if (anim_lock)
	{
		// depth shader doesn't have to do anything special for animation except for rendering the shadow of the moving piece
		glm_translate_to(identity, curr_mpos, model);
		set_mat4(depth_shader_prog, "model", model);
		render_piece(depth_shader_prog, true, animated_piece);
	}
}

void render_scene()
{
	glViewport(0, 0, screen_width, screen_height);
	glUseProgram(shader_prog);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm_vec3_copy((vec3) { sin(angle) * view_x, view_y, -cos(angle) * view_z }, view_pos);
	set_vec3(shader_prog, "view_pos", view_pos);
	angle += 0.002f;

	if (anim_lock)
	{
		// rendering of animated piece takes place before any change in position so that shadows line up
		glm_translate_to(identity, curr_mpos, model);
		set_mat4(shader_prog, "model", model);
		render_piece(shader_prog, false, animated_piece);
	}

	render_board(shader_prog, false);
	render_pieces(shader_prog, false);

	switch (anim_lock)
	{
	case 0 :
		// if no animation is happening, look at the center of the board and update time
		glm_lookat(view_pos, (vec3) { 0.f, 0.f, 0.f }, (vec3) { 0.f, 1.f, 0.f }, view);
		set_mat4(shader_prog, "view", view);
		set_float(shader_prog, "time", (cos(glfwGetTime() * 3) + 1.f) / 3.f + 0.2f); // gives a pulsating effect to selected pieces
		break;
	case 1 : // first step of move animation; see init_move_anim()
		if (camera_slow_pan(true))
		{
			anim_time = 0.f;
			anim_lock = 2;
		}
		break;
	case 2 :
		if (move_animated_piece()) 
		{
			anim_time = 1.f;
			anim_lock = 3; 
			// when camera_slow_pan is set to false, it runs in reverse, from destination to origin
			// so the destination var is set to the starting point of animation, and the origin var is already set to the end point of animation (0, 0, 0)
			glm_vec3_copy(dest_mpos, dest_cpos);

			glm_vec3_copy(dest_mpos, curr_mpos);
		}
		break;
	case 3 :
		if (camera_slow_pan(false))
		{
			anim_lock = 4;
		}
		break;
	case 4 : // end of move animation
		chess_board[dest_pos[1]][dest_pos[0]] = animated_piece;
		animated_piece = empty;
		anim_lock = 0;
		break;
	}
}

/*****************
 *  CHESS LOGIC  *
 *****************/

bool valid_move(int px, int py, int ex, int ey)
{
	// currently the only requirement for a valid move is that the selected piece isn't empty (or invalid), and that the destination square is valid
	return (px != -1 && ex != -1 && chess_board[py][px] != empty);
}

void check_checkmate();

void move_piece()
{
	int px = select_pos[0];
	int py = select_pos[1];
	int ex = hover_pos[0];
	int ey = hover_pos[1];
	if (valid_move(px, py, ex, ey))
	{
		orig_pos[0] = px;
		orig_pos[1] = py;
		dest_pos[0] = ex;
		dest_pos[1] = ey;
		init_move_anim();
	}
}

/*******************************
 *  WINDOW CALLBACK FUNCTIONS  *
 *******************************/

void reshape_window_callback(GLFWwindow* window, int width, int height) 
{
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	glm_perspective(glm_rad(45.f), (float)width / (float)height, near_dist, far_dist, projection);
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (!anim_lock)
	{
		int old_hover[2] = { hover_pos[0], hover_pos[1] };
		check_hover(xpos, ypos);
		if (select_pos[0] == hover_pos[0] && select_pos[1] == hover_pos[1])
		{
			hover_pos[0] = -1;
			hover_pos[1] = -1;
		}
		if (old_hover[0] != hover_pos[0] || old_hover[1] != hover_pos[1])
			glfwSetTime(0);
	}
}

void mouse_click_callback(GLFWwindow* window, int button, int action, int mods) 
{
	if(action == GLFW_PRESS && !anim_lock && !shadow_debug)
		switch (button)
		{
		case GLFW_MOUSE_BUTTON_LEFT : 
			move_piece();
			select_pos[0] = hover_pos[0];
			select_pos[1] = hover_pos[1];
			break;
		case GLFW_MOUSE_BUTTON_RIGHT :
			select_pos[0] = -1;
			select_pos[1] = -1;
			break;
		}
}

void key_press_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_D : 
			if (shadow_debug)
				glClearColor(0.7f, 1.f, 1.f, 1.f);
			else
				glClearColor(0.f, 0.f, 0.f, 1.f);
			shadow_debug = !shadow_debug;
			break;
		}
	}
}

/*************************
 *  OPENGL INIT & MAIN   *
 *************************/

int WGLExtensionSupported(const char *extension_name)
{
	// this is pointer to function which returns pointer to string with list of all wgl extensions
	PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;

	// determine pointer to wglGetExtensionsStringEXT function
	_wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");

	if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
	{
		// string was not found
		return 0;
	}

	// extension is supported
	return 1;
}

int init()
{
	// texture loading
	const char * tex_filenames[] = { "chess_items/board/board_tex.jpg", "chess_items/board/frame_tex.jpg", "chess_items/pieces/p1_marble_tex.jpg", "chess_items/pieces/p2_marble_tex_1.jpg" };
	unsigned int * tex_ids[] = { &board_tex, &frame_tex, &p1_tex, &p2_tex };

	for (int i = 0; i < 4; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glGenTextures(1, tex_ids[i]);
		glBindTexture(GL_TEXTURE_2D, *tex_ids[i]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		int tex_w, tex_h, nr_channels;
		unsigned char * tex_data = stbi_load(tex_filenames[i], &tex_w, &tex_h, &nr_channels, 0);
		if (tex_data)
		{
			if (nr_channels == 3)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_data);
				glGenerateMipmap(GL_TEXTURE_2D);
			}
			else if (nr_channels == 4)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
				glGenerateMipmap(GL_TEXTURE_2D);
			}
			else
			{
				printf("invalid number of channels\n");
				return 0;
			}
			printf("loaded texture from: %s\n", tex_filenames[i]);
		}
		else
		{
			printf("failed to load texture data from %s\n", tex_filenames[i]);
			return 0;
		}
		stbi_image_free(tex_data);
	}

	// model vertex loading
	const char*   obj_filenames[] = { "chess_items/board/board.obj",   "chess_items/board/board_frame.obj", "chess_items/pieces/pawn.obj", "chess_items/pieces/bishop.obj", 
									  "chess_items/pieces/knight.obj", "chess_items/pieces/king.obj", "chess_items/pieces/queen.obj",  "chess_items/pieces/rook.obj" };
	unsigned int* obj_VAOs [] = { &board_VAO, &frame_VAO, &pawn_VAO, &bishop_VAO, &knight_VAO, &king_VAO, &queen_VAO, &rook_VAO };
	unsigned int* obj_VBOs [] = { &board_VBO, &frame_VBO, &pawn_VBO, &bishop_VBO, &knight_VBO, &king_VBO, &queen_VBO, &rook_VBO };
	unsigned int* obj_vertct [] = { &board_vert_count, &frame_vert_count, &pawn_vert_count, &bishop_vert_count, &knight_vert_count, &king_vert_count, &queen_vert_count, &rook_vert_count };
	float*        obj_buffer;
	unsigned int  data_length, vert_count;

	for (int i = 0; i < 8; i++)
	{
		obj_buffer = load_obj(obj_filenames[i], &data_length, &vert_count);
		if (obj_buffer)
		{
			glGenBuffers(1, obj_VBOs[i]);
			glGenVertexArrays(1, obj_VAOs[i]);
			glBindBuffer(GL_ARRAY_BUFFER, *obj_VBOs[i]);
			glBufferData(GL_ARRAY_BUFFER, data_length, obj_buffer, GL_STATIC_DRAW);
			*(obj_vertct[i]) = vert_count;
			glBindVertexArray(*obj_VAOs[i]);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // position attrib
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal attrib
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // texture attrib
			glEnableVertexAttribArray(2);
			printf("loaded model from: %s\n", obj_filenames[i]);
			free(obj_buffer);
		}
		else
		{
			printf("failed to load: %s\n", obj_filenames[i]);
			return 0;
		}
	}

	// shader compilation and linking
	const char *vertfn = "vert.vs", *fragfn = "frag.fs";
	shader_prog = compile_and_link_shader(vertfn, fragfn);
	if (!shader_prog)
	{
		printf("failed to create shader program from %s and %s\n", vertfn, fragfn);
		return 0;
	}
	printf("successfully compiled shader program from %s and %s\n", vertfn, fragfn);

	const char *depth_vertfn = "depth.vs", *depth_fragfn = "depth.fs";
	depth_shader_prog = compile_and_link_shader(depth_vertfn, depth_fragfn);
	if (!depth_shader_prog)
	{
		printf("failed to create shader program from %s and %s\n", depth_vertfn, depth_fragfn);
		return 0;
	}
	printf("successfully compiled shader program from %s and %s\n", depth_vertfn, depth_fragfn);

	glEnable(GL_DEPTH_TEST);

	// shadow depth buffer init
	glActiveTexture(GL_TEXTURE4);
	glGenFramebuffers(1, &depth_map_FBO);
	glGenTextures(1, &depth_map);
	glBindTexture(GL_TEXTURE_2D, depth_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (vec4) { 1.f, 1.f, 1.f, 1.f });
	glBindFramebuffer(GL_FRAMEBUFFER, depth_map_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// initialize shader uniform variables
	near_dist = 1.f, far_dist = 20.f;
	view_x = 9.f, view_y = 9.f, view_z = 9.f;
	glUseProgram(shader_prog);
	glm_mat4_copy(identity, model);
	glm_vec3_copy((vec3) { view_x, view_y, view_z }, view_pos);
	glm_vec3_copy((vec3) { -5.f, 5.f, 1.f }, light_pos);
	glm_lookat(view_pos, (vec3) { 0.f, 0.f, 0.f }, (vec3) { 0.f, 1.f, 0.f }, view);
	glm_perspective(glm_rad(45.f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, near_dist, far_dist, projection);
	//glm_ortho(-6, 6, -6, 6, near_dist, far_dist, light_proj); // for a directional light
	glm_lookat(light_pos, (vec3) { 0.f, 0.f, 0.f }, (vec3) { 0.f, 1.f, 0.f }, light_view);
	glm_perspective(glm_rad(90.f), 1.f, near_dist, far_dist, light_proj);
	set_mat4(shader_prog, "model", model);
	set_mat4(shader_prog, "view", view);
	set_mat4(shader_prog, "projection", projection);
	set_int(shader_prog, "tex", p1_tex);
	set_vec3(shader_prog, "view_pos", view_pos);
	set_vec3(shader_prog, "light_pos", light_pos);
	set_float(shader_prog, "shininess", 32.f);
	set_mat4(shader_prog, "light_view", light_view);
	set_mat4(shader_prog, "light_proj", light_proj);
	set_int(shader_prog, "depth_map", 4);

	glUseProgram(depth_shader_prog);
	set_mat4(depth_shader_prog, "light_view", light_view);
	set_mat4(depth_shader_prog, "light_proj", light_proj);

	glUseProgram(shader_prog);
	glClearColor(0.7f, 1.f, 1.f, 1.f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	screen_width = SCREEN_WIDTH;
	screen_height = SCREEN_HEIGHT;
	angle = 0.f;
	gen_hover_mesh();
	select_pos[0] = -1;
	select_pos[1] = -1;
	hover_pos[0] = -1;
	hover_pos[1] = -1;
	anim_lock = 0;
	shadow_debug = false;

	return 1;
}

int main()
{
	// super boilerplate stuff
	GLFWwindow* window;

	if (!glfwInit())
	{
		printf("Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Checkers", NULL, NULL);
	if (!window)
	{
		printf("Failed to open window\n");
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, reshape_window_callback);
	glfwSetMouseButtonCallback(window, mouse_click_callback);
	glfwSetCursorPosCallback(window, mouse_move_callback);
	glfwSetKeyCallback(window, key_press_callback);

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD\n");
		return -1;
	}

	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
	PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT = NULL;

	if (WGLExtensionSupported("WGL_EXT_swap_control"))
	{
		// Extension is supported, init pointers.
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

		// this is another function from WGL_EXT_swap_control extension
		wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

		// enables VSYNC and limits FPS to refresh rate
		wglSwapIntervalEXT(1);
	}
	// end super boilerplate stuff

	printf("initializing...\n");
	if (!init())
	{
		printf("init failed...\n");
		return -1;
	}

	while (!glfwWindowShouldClose(window))
	{
		render_shadows();

		if (!shadow_debug)
			render_scene();

		update_hover_mesh();

		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(8, (GLuint[8]){board_VAO, frame_VAO, pawn_VAO, bishop_VAO, knight_VAO, queen_VAO, queen_VAO, rook_VAO});
	glDeleteBuffers     (8, (GLuint[8]){board_VBO, frame_VBO, pawn_VBO, bishop_VBO, knight_VBO, queen_VBO, queen_VBO, rook_VBO});
	glDeleteTextures    (5, (GLuint[5]){board_tex, frame_tex, p1_tex, p2_tex, depth_map});
	glDeleteFramebuffers(1, &depth_map_FBO);
	glDeleteShader(shader_prog);
	glDeleteShader(depth_shader_prog);

	glfwTerminate();

	return 0;
}