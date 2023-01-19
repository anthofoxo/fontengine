#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define AFFE_IMPLEMENTATION
#include "vendor/stb_rect_pack.h"
#include "vendor/stb_truetype.h"
#include "af_fontengine.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <fstream>
#include <cstdio>

struct user_data
{
	unsigned int program;
	unsigned int texture;
	unsigned int vao, vbo;
};

typedef struct user_data user_data;

static unsigned char* load(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if (!f) return NULL;

	long long len, pos;
	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, pos, SEEK_SET);
	
	unsigned char* buffer = (unsigned char*)malloc(len + 1);

	if (!buffer)
	{
		fclose(f);
		return NULL;
	}

	fread(buffer, 1, len, f);
    buffer[len] = 0;
	fclose(f);
	return buffer;
}

unsigned int make_shader(unsigned int type, const char* resource)
{
	unsigned char* src = load(resource);

	const char* src_c = &src;
	const char* const* src_cc = &src_c;

	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, src_cc, NULL);
	glCompileShader(shader);

	int status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (!status)
	{
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &status);

		std::unique_ptr<GLchar[]> buffer = std::make_unique<GLchar[]>(status);

		glGetShaderInfoLog(shader, status, nullptr, buffer.get());

		printf("Opengl shader error: %s\n", buffer.get());
	}

	return shader;
}

static int create(void* user_ptr, int w, int h)
{	
	glGenVertexArrays(1, &((user_data*)user_ptr)->vao);
	glGenBuffers(1, &((user_data*)user_ptr)->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ((user_data*)user_ptr)->vbo);
	glBufferData(GL_ARRAY_BUFFER, AFFE_VERTEX_COUNT * sizeof(affe_vertex), NULL, GL_STREAM_DRAW);
	glBindVertexArray(((user_data*)user_ptr)->vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(affe_vertex), (const void*)offsetof(affe_vertex, x));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(affe_vertex), (const void*)offsetof(affe_vertex, s));
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(affe_vertex), (const void*)offsetof(affe_vertex, r));

	glGenTextures(1, &((user_data*)user_ptr)->texture);
	glBindTexture(GL_TEXTURE_2D, ((user_data*)user_ptr)->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

    unsigned int vsh, fsh;
	((user_data*)user_ptr)->program = glCreateProgram();
	vsh = make_shader(GL_VERTEX_SHADER, "text.vert.glsl");
	fsh = make_shader(GL_FRAGMENT_SHADER, "text.frag.glsl");

	glAttachShader(((user_data*)user_ptr)->program, vsh);
	glAttachShader(((user_data*)user_ptr)->program, fsh);

	glLinkProgram(((user_data*)user_ptr)->program);

	glDetachShader(((user_data*)user_ptr)->program, vsh);
	glDetachShader(((user_data*)user_ptr)->program, fsh);

	glDeleteShader(vsh);
	glDeleteShader(fsh);

	return TRUE;
}

static void update(void* user_ptr, int x, int y, int w, int h, void* pixels)
{
	glBindTexture(GL_TEXTURE_2D, ((user_data*)user_ptr)->texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_UNSIGNED_BYTE, pixels);
}

static void draw(void* user_ptr, affe_vertex* verts, long long verts_count)
{
	glBindBuffer(GL_ARRAY_BUFFER, ((user_data*)user_ptr)->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, verts_count * sizeof(affe_vertex), verts);

	glBindVertexArray(((user_data*)user_ptr)->vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ((user_data*)user_ptr)->texture);
	glUseProgram(((user_data*)user_ptr)->program);
	glDrawArrays(GL_TRIANGLES, 0, verts_count);	
}

static void destroy(void* user_ptr)
{
	glDeleteVertexArrays(1, &((user_data*)user_ptr)->vao);
	glDeleteBuffers(1, &((user_data*)user_ptr)->vbo);
	glDeleteProgram(((user_data*)user_ptr)->vbo);
	glDeleteTextures(1, &((user_data*)user_ptr)->texture);
}

int main(int argc, char** argv)
{
	unsigned char* filedata = load("NotoSans-Medium.ttf");

	if (!filedata) return 1;

	glfwInit();
	GLFWwindow* window = glfwCreateWindow(1280, 720, "af_fontengine.h opengl3.cpp", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGL(&glfwGetProcAddress);

	user_data data;
	memset(&data, 0, sizeof(user_data));

	affe_context_create_info info;
	memset(&info, 0, sizeof(affe_context_create_info));
	info.width = 512;
	info.height = 512;
	info.user_ptr = &data;
	info.create_proc = &create;
	info.update_proc = &update;
	info.draw_proc = &draw;
	info.delete_proc = &destroy;
	info.flags = AFFE_FLAGS_FLIP_UVS | AFFE_FLAGS_NORMALIZE_UVS;

	affe_context* ctx = affe_context_create(&info);
	affe_set_font(ctx, affe_font_add(ctx, filedata, 0, TRUE));
	affe_set_size(ctx, 32.0f);

	while (!glfwWindowShouldClose(window))
	{
		affe_text_draw(ctx, 100, 100, "Hello World!", NULL);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	affe_context_delete(ctx);

	glfwMakeContextCurrent(NULL);
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}