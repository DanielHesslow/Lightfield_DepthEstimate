#define GLEW_STATIC
#include "L:\glew\glew.h"
#include "L:\glfw\glfw3.h"
#include "imgui.h"
#include "stdio.h"

#define STB_IMAGE_IMPLEMENTATION
#include "L:\stb\stb_image.h"

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#include "L:\HandmadeMath.h"

#include "imgui_impl_glfw_gl3.h"

typedef hmm_vec2 vec2;
typedef hmm_vec3 vec3;
typedef hmm_vec4 vec4;
typedef hmm_mat4 mat4;

bool operator == (vec2 a, vec2 b) {
	return a.X == b.X && a.Y == b.Y;
}
bool operator == (vec3 a, vec3 b) {
	return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
}

bool operator == (vec4 a, vec4 b) {
	return a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
}
#include "windows.h"
#include "stdint.h"

struct ImGuiLog {
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets;        // Index to lines offset
	bool                ScrollToBottom;

	void    Clear() { Buf.clear(); LineOffsets.clear(); }

	void    AddLog(const char* fmt, ...) IM_PRINTFARGS(2) {
		int old_size = Buf.size();
		va_list args;
		va_start(args, fmt);
		Buf.appendv(fmt, args);
		vprintf(fmt, args);
		va_end(args);
		for (int new_size = Buf.size(); old_size < new_size; old_size++)
			if (Buf[old_size] == '\n')
				LineOffsets.push_back(old_size);
		ScrollToBottom = true;
	}

	void    Draw(const char* title, bool* p_open = NULL) {
		if (ImGui::Button("Clear")) Clear();
		ImGui::SameLine();
		ImGui::SameLine();
		Filter.Draw("Filter", -100.0f);
		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		if (Filter.IsActive()) {
			const char* buf_begin = Buf.begin();
			const char* line = buf_begin;
			for (int line_no = 0; line != NULL; line_no++) {
				const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
				if (Filter.PassFilter(line, line_end))
					ImGui::TextUnformatted(line, line_end);
				line = line_end && line_end[1] ? line_end + 1 : NULL;
			}
		} else {
			ImGui::TextUnformatted(Buf.begin());
		}

		if (ScrollToBottom)
			ImGui::SetScrollHere(1.0f);
		ScrollToBottom = false;
		ImGui::EndChild();
	}
};

ImGuiLog application_log;

GLFWwindow* window;

struct LF_Image {
	void *data;
	vec2 p;
	int x, y;
	int h, w;
};

struct Shader {
	uint64_t modified_time_frag;
	uint64_t modified_time_vert;
	char *frag;
	char *vert;
	GLuint program;
	bool is_valid;
};



// Trying to keep things simple,
// However for getting the modified time of a file
// there's no cross platform way without pulling in something exessive like boost
// While I prefer to use the win32 calls 
// For simplicity we'll use the ones that look the most alike. Neither very interesting nor complicated 
// It just is what it is. 
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
uint64_t last_mod_time(char *file_path) {
	struct _stat result;
	if (_stat(file_path, &result) == 0) {
		return result.st_mtime;
	}
	assert(false);
}
#else 
uint64_t last_mod_time(char *file_path) {
	struct stat result;
	if (stat(file_path, &result) == 0) {
		return result.st_mtim;
	}
	assert(false);
}
#endif

char *copy_file_to_memory(char *file_path) {
	FILE *f = fopen(file_path, "rb");
	if (!f)return NULL;
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *mem = (char *)malloc(fsize + 1);
	fread(mem, 1, fsize, f);
	mem[fsize] = '\0';
	fclose(f);
	return mem;
}

GLuint LoadShaders(char *vertex_file_path, char *fragment_file_path, bool *success) {

	*success = true;
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// could ignore the copy by memory mapping the file
	// but that is more complicated and we'd have to use OS-specific stuff
	// and this isn't really perf critical anyway. The added complexity is certainly not worth it.
	char *vertex_shader_source = copy_file_to_memory(vertex_file_path);
	if (vertex_shader_source == NULL) {
		*success = false;
		return 0;
	}
	char *fragment_shader_source = copy_file_to_memory(fragment_file_path);
	if (fragment_shader_source == NULL) {
		*success = false;
		free(vertex_shader_source);
		return 0;
	}
	GLint Result = GL_FALSE;
	int InfoLogLength;

	glShaderSource(VertexShaderID, 1, &vertex_shader_source, NULL);
	glCompileShader(VertexShaderID);

	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1) {
		char *err = (char *)alloca(InfoLogLength);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, err);
		application_log.AddLog("%s\n", err);
		*success = false;
	}

	application_log.AddLog("Compiling shader : %s\n", fragment_file_path);
	glShaderSource(FragmentShaderID, 1, &fragment_shader_source, NULL);
	glCompileShader(FragmentShaderID);

	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1) {
		*success = false;
		char *err = (char *)alloca(InfoLogLength);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, err);
		application_log.AddLog("%s\n", err);
	}

	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1) {
		*success = false;
		char *err = (char *)alloca(InfoLogLength);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, err);
		application_log.AddLog("%s\n", err);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	free(vertex_shader_source);
	free(fragment_shader_source);

	if (!*success) {
		application_log.AddLog("shader compilation failed, waiting for fix...\n");
	}
	return ProgramID;
}




Shader make_shader(char *vert, char *frag) {
	Shader ret = {};
	ret.frag = frag;
	ret.vert = vert;
	ret.program = LoadShaders(vert, frag, &ret.is_valid);
	ret.modified_time_frag = last_mod_time(frag);
	ret.modified_time_vert = last_mod_time(vert);
	return ret;
}

#include "time.h"
bool maybe_update_shader(Shader *shader) {
	uint64_t last_mod_time_frag = last_mod_time(shader->frag);
	uint64_t last_mod_time_vert = last_mod_time(shader->vert);

	if (last_mod_time_frag != shader->modified_time_frag
		|| last_mod_time_vert != shader->modified_time_vert) {
		if (shader->is_valid) glDeleteProgram(shader->program);
		shader->program = LoadShaders(shader->vert, shader->frag, &shader->is_valid);
		shader->modified_time_frag = last_mod_time_frag;
		shader->modified_time_vert = last_mod_time_vert;
		return true;
	}
	return false;
}

#include "float.h"
#pragma optimize ("",on)
GLuint lf_out, lf_in;

#define printOpenGLError() printOglError(__FILE__, __LINE__)
int printOglError(char *file, int line) {
	GLenum glErr;
	int    retCode = 0;

	while ((glErr = glGetError()) != GL_NO_ERROR) {
		application_log.AddLog("glError in file %s @ line %d: %s\n",
			file, line, gluErrorString(glErr));
		retCode = 1;
	}
	if (retCode)__debugbreak();
	return retCode;
}








LF_Image images[81];
int num_images = 0;

GLuint depth_texture;

GLuint create_texture_3D(GLint internal_format, const void *pixels) {
	GLuint tex;
	printOpenGLError();
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_3D, tex);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexImage3D(GL_TEXTURE_3D, 0, internal_format, images[0].w, images[0].h, num_images, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	printOpenGLError();
	return tex;
}
GLuint create_texture_2D(GLint internal_format, GLenum pixel_format, const void *pixels) {
	GLuint tex;
	printOpenGLError();
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, images[0].w, images[0].h, 0, pixel_format, GL_UNSIGNED_BYTE, pixels);
	printOpenGLError();
	return tex;
}




void upload_lf_textures() {
	size_t image_size = 4 * images[0].w*images[0].h;
	void *mem = malloc(image_size * num_images);

	for (int x = 0; x < 9; x++) {
		for (int y = 0; y < 9; y++) {
			memcpy((char *)mem + (x + 9 * (8 - y))*image_size, images[x + 9 * y].data, image_size);
		}
	}

	lf_in = create_texture_3D(GL_RGBA, mem);
}


#pragma optimize("",off)
void draw_grid() {
	// draw_call for disp_prop_no_mesh.vert

	// so we're drawing a 512 by 512 grid. each vert corresponding to a pixel in the input texture
	// we manage to do this in a single draw call using no mesh at all.
	// this is quite a bit quicker than using a mesh. Since that would be a large one and the chache didn't seem to like it to much 

	// each instance draws a single quad strip
	// y in range [instance_id,instance_id+1 (instnace_id + vertex_id % 2)]
	// x in range [0,512 = vertex_id/2] 

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 1024, 512);
}

void draw() {
	// draw_call for base_no_mesh.vert

	// also using no mesh, pretty nice :) 

	// draws full screen triangle. faster than full screen quad. 
	// Apparently about 8% faster according to some blogpost I read, more than I would have though.
	// although probably less in our case. Since we're very bottlenecked by pixelshader the cache shouldn't be such a big deal.
	// however a minimum of 1/512th faster cause we will calculate the diagonal twice I believe.
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
}



struct DepthPassData {
	Shader shader;
	GLuint frame_buffer_id;
	GLuint textures[10];
	GLenum internal_format;
	int num_textures;
	GLuint texture;
	float progress;
	uint32_t flags;
	void(*cpu_pass)(uint8_t *prev, vec3 *color);
};

uint32_t depth_pass_flag_set_fracts = 1 << 0;
uint32_t depth_pass_flag_disp_prop = 1 << 1;
uint32_t depth_pass_flag_only_center = 1 << 2;
uint32_t depth_pass_flag_slow = 1 << 4;


DepthPassData depth_passes[100];
int num_depth_passes = 0;



#include <stdarg.h>

void draw_texture_to_screen(GLuint texture_2d) {
	static Shader pass_trough_shader = make_shader("base_no_mesh.vert", "pass_through.frag");
	printOpenGLError();
	glUseProgram(pass_trough_shader.program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_2d);
	draw();
}


GLuint add_depth_pass(void *shader, GLint internal_texture_format, uint32_t flags, int num_deps, ...) {
	DepthPassData data = {};
	{
		if (flags & depth_pass_flag_disp_prop)
			data.shader = make_shader("disp_prop_no_mesh.vert", (char *)shader);
		else
			data.shader = make_shader("base_no_mesh.vert", (char *)shader);
	}

	data.internal_format = internal_texture_format;
	{//make the fbo
		glGenFramebuffers(1, &data.frame_buffer_id);
		glBindFramebuffer(GL_FRAMEBUFFER, data.frame_buffer_id);

		//create and attach draw buffer
		GLenum DrawBuffers = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &DrawBuffers);

		if (depth_pass_flag_disp_prop) {
			GLuint depth_buffer;
			glGenRenderbuffers(1, &depth_buffer);
			glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 512, 512);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
		}
	}
	data.flags = flags;
	data.num_textures = num_deps;
	data.progress = 0.0;
	va_list va;
	va_start(va, num_deps);
	for (int i = 0; i < num_deps; i++) {
		data.textures[i] = va_arg(va, GLuint);
	}
	va_end(va);
	data.texture = create_texture_3D(internal_texture_format, NULL);
	depth_passes[num_depth_passes++] = data;
	return data.texture;
}



enum constant_uniform_locations {
	uniform_location_camera = 0,
	uniform_location_fractions = 2,
	uniform_location_active_camera = 1,
	uniform_location_num_fractions = 1,
};


#include "fractions.h"
void calc_depth_gpu_step() {
	static bool first = true;
	if (first) {

		GLuint prev;
		prev = add_depth_pass("calc_depth_all_opt.frag", GL_RGBA16, depth_pass_flag_set_fracts|depth_pass_flag_slow, 1, lf_in);
		prev = add_depth_pass("mark_reliable.frag", GL_RGBA16, 0, 2, lf_in, prev);
		prev = add_depth_pass("disp_propagation.frag", GL_RGBA16, depth_pass_flag_disp_prop, 2, lf_in, prev);
		prev = add_depth_pass("normalize.frag", GL_RGBA16, 0, 1, prev);
		prev = add_depth_pass("calc_depth_max_ontop.frag", GL_RGBA16, depth_pass_flag_slow, 2, lf_in, prev);
		prev = add_depth_pass("disp_propagation_2.frag", GL_RGBA16, depth_pass_flag_disp_prop| depth_pass_flag_slow, 2, lf_in, prev);
		prev = add_depth_pass("normalize.frag", GL_RGBA16, 0, 1, prev);		
		prev = add_depth_pass("finalize.frag", GL_RGBA16, 0, 2, lf_in, prev);
		lf_out = prev;
		first = false;
	}

	printOpenGLError();

	static int depth_pass = 0;
	static int cam_x = 0;
	static int cam_y = 0;
	static int scissor_index=0;

	static time_t last_check_time;
	time_t t; time(&t);
	if (t - last_check_time > 1) { // only check for updates if we havn't for more than a sec.  
		last_check_time = t;
		for (int i = 0; i < num_depth_passes; i++) {
			if (maybe_update_shader(&depth_passes[i].shader)) {
				if (i <= depth_pass) {
					depth_pass = i;
					cam_x = cam_y = 0;
					for (int j = i; j < num_depth_passes; j++) depth_passes[j].progress = 0;
				}
			}
		}
	}
	printOpenGLError();

	int scissor_size = 128;
	int scissor_divs = 512 / scissor_size;

	if (!depth_passes[depth_pass].shader.is_valid)return;
	printOpenGLError();
	if (depth_pass < num_depth_passes) {
		GLint64 vp[4]; glGetInteger64v(GL_VIEWPORT, vp);
		glViewport(0, 0, 512, 512);
		DepthPassData data = depth_passes[depth_pass];
		if((data.flags & depth_pass_flag_slow)) glEnable(GL_SCISSOR_TEST);
		// so on windows ogl auto times out after 2 seconds of computation.
		// we need more during computation phase :/ (atleast for the edges on my laptop) 
		// so we just go at it a bit at a time.
		// however for the depth propagation vertex shader is the slow part anyway. (so there's ~no gain)

		int sx = (scissor_index % scissor_divs)*scissor_size;
		int sy = (scissor_index / scissor_divs)*scissor_size;
		glScissor(sx, sy, scissor_size, scissor_size);
		glBindFramebuffer(GL_FRAMEBUFFER, data.frame_buffer_id);
		glUseProgram(data.shader.program);
		for (int tex = 0; tex < data.num_textures; tex++) {
			glActiveTexture(GL_TEXTURE0 + tex);
			glBindTexture(GL_TEXTURE_3D, data.textures[tex]);
		}
		printOpenGLError();

		if (depth_passes[depth_pass].flags & depth_pass_flag_only_center) {
			cam_x = 4;
			cam_y = 4;
		}

		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, data.texture, 0, cam_x + cam_y * 9);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		{
			if (data.flags & depth_pass_flag_set_fracts) {
				int num_fracts;
				float *fractions = get_fractions(max(max(8-cam_x,cam_x),max(8-cam_y,cam_y)), &num_fracts);
				glUniform1fv(uniform_location_fractions, num_fracts, fractions);
				glUniform1i(uniform_location_num_fractions, num_fracts);
			}
			printOpenGLError();
			glUniform2i(uniform_location_camera, cam_x, cam_y);
			if (data.flags  & depth_pass_flag_disp_prop) {
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				//glEnable(GL_DEPTH_TEST);
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				//bind_grid();
				for (int cam_xx = 0; cam_xx < 9; cam_xx++)
				for (int cam_yy = 0; cam_yy < 9; cam_yy++) {
						glClear(GL_DEPTH_BUFFER_BIT);
						glUniform2i(uniform_location_active_camera, cam_xx, cam_yy);
						draw_grid();
				}
				//glDisable(GL_DEPTH_TEST);
			} else draw();
		}
		printOpenGLError();
		++scissor_index;
		printf("sc=%d\n", scissor_index);
		if (scissor_index == scissor_divs*scissor_divs||~data.flags & depth_pass_flag_slow) {
			scissor_index = 0;
			if (depth_passes[depth_pass].flags & depth_pass_flag_only_center) {
				depth_passes[depth_pass].progress = 1.0f;
				++depth_pass;
				cam_x = cam_y = 0;
			} else {
				depth_passes[depth_pass].progress = (cam_x * 9 + cam_y) / 80.0;
				++cam_y;
				if (cam_y == 9) {
					cam_y = 0;
					++cam_x;
					if (cam_x == 9) {
						cam_x = 0;
						++depth_pass;
					}
				}
			}
		}

		// This doens't really seem to be needed anymore
		// but I'll keep it in anyway. Atleast in my driver we need to have finnished the last draw
		// before we call glFramebufferTextureLayer again
		// however since we go through the drawing the ui inbetween, on my cpu this is always the case
		// but removing it seems like an annoying bug waiting to happen. 
		// for example if we decide to only draw the ui if we havn't in the last 30 ms or something 
		// this would prob crash on my cpu. yada yada, it's staying.
		glFinish();

		printOpenGLError();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport((GLsizei)vp[0], (GLsizei)vp[1], (GLsizei)vp[2], (GLsizei)vp[3]);
		glDisable(GL_SCISSOR_TEST);
		printOpenGLError();
	}
	printOpenGLError();
}


void load_light_images() {
#define test "sideboard"
	//#define test "medieval"
	//#define test "cotton"
	//#define test "rosemary"
	//#define test "pillows"
	//#define test "vinyl"
	//#define test "table"

	for (int i = 0; i < 81; i++) {
		int comp;
		LF_Image image;
		char buffer[500];
		sprintf(buffer, test"/input_Cam%.3d.png", i);
		image.data = stbi_load(buffer, &image.w, &image.h, &comp, STBI_rgb_alpha);
		assert(image.data);
		images[num_images++] = image;
	}

	upload_lf_textures();

	{
		// parsing the simple pfm format
		FILE *file = fopen(test "/gt_disp_lowres.pfm", "rb");
		fscanf(file, "Pf\n");
		int w, h, scale;
		fscanf(file, "%d %d\n", &w, &h);
		fscanf(file, "%d\n", &scale);
		// assert we have one setting that we do handle
		assert(w == 512 && h == 512 && scale == -1);
		float *mem = (float *)malloc(512 * 512 * 4);
		fread(mem, 1, 4 * 512 * 512, file);


		fclose(file);
		glGenTextures(1, &depth_texture);

		glBindTexture(GL_TEXTURE_2D, depth_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexImage2D(GL_TEXTURE_2D, 0, 0x8815, w, h, 0, GL_RED, GL_FLOAT, mem);
		free(mem);
	}
}


void update(float *data, float speed, float delta_time, int key_up, int key_down) {
	if (glfwGetKey(window, key_up) == GLFW_PRESS) {
		*data += speed*delta_time;
	}
	if (glfwGetKey(window, key_down) == GLFW_PRESS) {
		*data -= speed*delta_time;
	}
}

void update_mul(float *data, float speed, float delta_time, int key_up, int key_down) {
	//yea i know it's not right. but meh.
	if (glfwGetKey(window, key_up) == GLFW_PRESS) {
		*data *= 1.0 + speed*delta_time;
	}
	if (glfwGetKey(window, key_down) == GLFW_PRESS) {
		*data *= 1.0 - speed*delta_time;
	}
}

#include <chrono>
void maybe_update() {
	auto time = std::chrono::system_clock::now();
	auto static last_check_time = std::chrono::system_clock::now();
	glFinish();
	if (std::chrono::duration_cast<std::chrono::milliseconds>((time - last_check_time)).count() < 15)return;
	last_check_time = time;
	ImGui_ImplGlfwGL3_NewFrame();
	glfwPollEvents();

	static vec3 p = { 0.5,0.5,0.0 };
	static vec3 d = { 0,0,1 };
	static float focus_plane = 50.0;
	static float aparture = 0.00;
	static int render_mode = 0;

	static Shader s = make_shader("base_no_mesh.vert", "lf_renderer_2pp.frag");

	maybe_update_shader(&s);
	printOpenGLError();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (s.is_valid) {
		printOpenGLError();
		glUseProgram(s.program);
		static GLuint texture_id = glGetUniformLocation(s.program, "texture");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, lf_out);
		glUniform1i(texture_id, 0);

		static GLuint depth_id = glGetUniformLocation(s.program, "depth");
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depth_texture);
		glUniform1i(depth_id, 1);

		glClear(GL_COLOR_BUFFER_BIT);
		vec4 right = { 1, 0, 0, 0 };
		vec4 up = { 0, 1, 0, 0 };
		vec4 dir = { 0, 0, 1, 0 };
		vec4 pos = { 0, 0, 0, 1 };


		float speed = 3.0;

		static float last_time = glfwGetTime();
		float time = glfwGetTime();
		float delta_time = time - last_time;
		last_time = time;
		auto io = ImGui::GetIO();
		if (!io.WantCaptureKeyboard) {
			update(&d.X, speed, delta_time, GLFW_KEY_RIGHT, GLFW_KEY_LEFT);
			update(&d.Y, speed, delta_time, GLFW_KEY_UP, GLFW_KEY_DOWN);
			update(&p.X, speed / 2.0, delta_time, GLFW_KEY_D, GLFW_KEY_A);
			update(&p.Y, speed / 2.0, delta_time, GLFW_KEY_W, GLFW_KEY_S);
			update(&p.Z, speed / 2.0, delta_time, GLFW_KEY_E, GLFW_KEY_Q);
			update_mul(&focus_plane, 0.3, delta_time, GLFW_KEY_Z, GLFW_KEY_X);
			update(&aparture, speed*0.1, delta_time, GLFW_KEY_C, GLFW_KEY_V);
		}
		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
			render_mode = 0;
		}
		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
			render_mode = 1;
		}
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
			render_mode = 2;
		}
		if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
			render_mode = 3;
		}

		vec3 ly = up.XYZ;
		vec3 lx = right.XYZ;
		vec3 lz = HMM_Normalize(d);
		lx = HMM_Cross(ly, lz);
		ly = HMM_Cross(lz, lx);
		mat4 l =
		{
			lx.X,lx.Y,lx.Z,0,
			ly.X,ly.Y,ly.Z,0,
			lz.X,lz.Y,lz.Z,0,
			0, 0, 0, 1
		};
		l = l * HMM_Translate(p);

		right = l*right;
		dir = l*dir;
		pos = l*pos;

		printf("delta time:%f\n", delta_time);

		static GLuint time_id = glGetUniformLocation(s.program, "time");
		static GLuint right_id = glGetUniformLocation(s.program, "right");
		static GLuint up_id = glGetUniformLocation(s.program, "up");
		static GLuint pos_id = glGetUniformLocation(s.program, "ppos");
		static GLuint dir_id = glGetUniformLocation(s.program, "dir");
		static GLuint focus_plane_id = glGetUniformLocation(s.program, "focus_plane");
		static GLuint apparture_id = glGetUniformLocation(s.program, "aparture");
		static GLuint render_mode_id = glGetUniformLocation(s.program, "render_mode");

		glUniform1f(time_id, glfwGetTime());
		glUniform3fv(right_id, 1, right.Elements);
		glUniform3fv(up_id, 1, up.Elements);
		glUniform3fv(pos_id, 1, p.Elements);
		glUniform3fv(dir_id, 1, dir.Elements);
		glUniform1f(focus_plane_id, focus_plane);
		glUniform1f(apparture_id, aparture);
		glUniform1i(render_mode_id, render_mode);
		draw();
		printOpenGLError();
	}
	{
		printOpenGLError();
		ImGui::SliderFloat("focus_plane", &focus_plane, 0.0f, 500.0f, "%.3f", 2.0);
		ImGui::SliderFloat("apparture", &aparture, 0.0f, 1.0f);
		ImGui::SliderFloat3("origin", p.Elements, -1.0f, 1.0f);
		ImGui::SliderFloat3("dir", d.Elements, -1.0f, 1.0f);
		ImGui::Combo("RenderMode", &render_mode, "Depth\0Epi\0DepthBadPix\0Color\0");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		for (int depth_pass = 0; depth_pass < num_depth_passes; depth_pass++) {
			ImGui::PushID(depth_pass);
			if (ImGui::RadioButton("", depth_passes[depth_pass].texture == lf_out)) {
				lf_out = depth_passes[depth_pass].texture;
			}
			ImGui::SameLine();
			ImGui::ProgressBar(depth_passes[depth_pass].progress, ImVec2(-1, 0), depth_passes[depth_pass].shader.frag);
		}
		printOpenGLError();
		application_log.Draw("log");
		printOpenGLError();
		ImGui::Render();
		printOpenGLError();
	}
	glfwSwapBuffers(window);
	printOpenGLError();
}

int main() {
	stbi_set_flip_vertically_on_load(true);

	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, true);

	window = glfwCreateWindow(512, 512, "light field renerer", NULL, NULL);
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	// Open a window and create its OpenGL context
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. \n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	glGetError();
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwPollEvents();

	printf("OpenGL-version: %s\n", glGetString(GL_VERSION));

	ImGui_ImplGlfwGL3_Init(window, true);

	GLuint vao_id;
	glGenVertexArrays(1, &vao_id);
	glBindVertexArray(vao_id);

	load_light_images();
	maybe_update();
	while (!glfwWindowShouldClose(window)) {
		calc_depth_gpu_step();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		maybe_update();
		printOpenGLError();
	}
	ImGui_ImplGlfwGL3_Shutdown();
	return 0;
}


















/*
NOT IN USE CURRENTLY

Filling in regions without any information, currently this is not in use. I should be able to come up with a better way
that taks the light field in concideration and hopefully we can do it on the gpu as well.

Algorithm:
flow fill if similar color
if we hit a new neighbour draw line (using bresenham) between set all to origins (ie distance 0 from start)
seems actuall somewhat fast. however wont handle long lerps well but really, meh.

if all neighbors are set fill no matter what (to avg?) currently we do not neccessarily fill everything.

*/


float lerp(float a, float b, float t) {
	return a *(1.0f - t) + b * t;
}


#include <set>
#include <queue>
void cpu_fill(uint8_t *disp_data, vec3 *color_data) {

	struct ivec2 {
		int16_t x, y;
		float distance_to(ivec2 o) {
			float dx = x - o.x;
			float dy = y - o.y;
			return sqrt(dx*dx + dy*dy);
		}
	};

	struct tmp_data {
		int16_t distance;
		ivec2 origin;
	};

	static tmp_data data[512][512];

	// yea we don't need all of these, we could probably get away with like 4 but meh.
	// really doubt this'll be the bottleneck anyway.
	static std::queue<ivec2> points[256];

#define index_of(p) ((p.x)+(p.y)*512)
#define color_of(p) color_data[index_of(p)]
#define disp_of(p) disp_data[index_of(p)]

	ivec2 neighbours[4] = { { 0,-1 },{ 0,1 },{ -1, 0 },{ 1, 0 } };
	for (int x = 0; x < 512; x++) {
		for (int y = 0; y < 512; y++) {
			ivec2 p = { x,y };
			data[x][y] = { (short)-2, p };
			if (disp_of(p)) {
				data[x][y].distance = 0;

				bool push = false;
				for (int i = 0; i < 4; i++) {
					for (int i = 0; i < 4; i++) {
						ivec2 n = { p.x + neighbours[i].x,p.y + neighbours[i].y };
						if (!disp_of(n)) {
							push = true;
							break;
						}
					}
				}
				if (push) points[0].push(p);
			}
		}
	}

	bool clear_dist;
	for (int dist = 0; dist < 256; dist++) {
		printf("dist:%d\n", dist);
		while (points[dist].size() > 0) {
			ivec2 p = points[dist].front();
			points[dist].pop();
			if (data[p.x][p.y].distance < 0)__debugbreak();
			uint64_t num_p = points[0].size();
			for (int i = 0; i < 4; i++) {
				ivec2 n = { p.x + neighbours[i].x,p.y + neighbours[i].y };
				if (!(n.x >= 0 && n.y >= 0 && n.x < 512 && n.y < 512)) continue;

				int d = data[n.x][n.y].distance;
				// plus one needed in second pass.
				if (d == dist - 1 || d == dist || d == dist + 1) {
					// draw line from our origin to neighbouring origin using bresenham  
					ivec2 start = data[n.x][n.y].origin;
					ivec2 end = data[p.x][p.y].origin;
					int d = abs(start.x - end.x) + abs(start.y - end.y);
					if (d >= 2) {
						int dx = abs(end.x - start.x);
						int dy = -abs(end.y - start.y);
						int sx = start.x < end.x ? 1 : -1;
						int sy = start.y < end.y ? 1 : -1;

						// draw line
						int err = dx + dy;
						ivec2 curr = start;
						for (;;) {
							int e2 = 2 * err;
							if (e2 >= dy) { err += dy; curr.x += sx; }
							if (e2 <= dx) { err += dx; curr.y += sy; }
							if (curr.x == end.x && curr.y == end.y) break;
							{// setting the pixel
								if (data[curr.x][curr.y].distance != 0) {
									data[curr.x][curr.y] = { 0, curr };
									points[0].push(curr);
									float t = curr.distance_to(start) / (curr.distance_to(start) + curr.distance_to(end));
									disp_of(curr) = lerp(disp_of(start), disp_of(end), t); // uses float lerp, fixme.
								}
							}
						}
						clear_dist = true; // NOTE: yes this is needed. we need to set other neighbours before clearing the dist.
					}
				} else {
					if (data[n.x][n.y].distance > data[p.x][p.y].distance + 1 || data[n.x][n.y].distance == -2) {
						//data[n.x][n.y] = { (short)(data[p.x][p.y].distance + 1), data[p.x][p.y].origin };
						data[n.x][n.y] = { (short)(dist + 1), data[p.x][p.y].origin };
						points[dist + 1].push(n);
					}
				}
			}
			if (clear_dist) dist = 0;
		}
	}
}
