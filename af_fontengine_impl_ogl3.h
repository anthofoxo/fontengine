/* af_fontengine_impl_ogl3.h Last Updated: v0.1.6

Authored from 2023 by AnthoFoxo

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Credits to Sean Barrett, Mikko Mononen, Bjoern Hoehrmann, and the community for making this project possible.

Visit the github page for updates and documentation: https://github.com/anthofoxo/fontengine

Contributor list
AnthoFoxo
*/
#ifndef AF_FONTENGINE_OGL3_H
#define AF_FONTENGINE_OGL3_H

// Include an OpenGL header before including this file
// #define AFFE_OGL3_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

AFFE_API affe_context* affe_ogl3_context_create(int width, int height, int quads, int padding, int size);
AFFE_API void affe_ogl3_viewport(affe_context* ctx, int width, int height);
AFFE_API void affe_ogl3_context_delete(affe_context* ctx);

#ifdef __cplusplus
}
#endif

#endif // AF_FONTENGINE_OGL3_H

#ifdef AFFE_OGL3_IMPLEMENTATION

struct affe__ogl
{
	GLuint program;
	GLuint texture;
	GLuint vao, vbo;
	int w, h;
};

static unsigned int affe__ogl__make_shader(unsigned int type, const char* source)
{
	unsigned int shader = glCreateShader(type);

	if (!shader) return 0;

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	int status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (!status)
	{
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static int create(affe_context* ctx, void* user_ptr, int w, int h)
{;
	unsigned int vsh = 0;
	unsigned int fsh = 0;

	const char* vsh_source = "#version 330 core\n\nlayout(location = 0) in vec2 vert_pos;\nlayout(location = 1) in vec2 vert_tex;\nlayout(location = 2) in vec4 vert_col;\n\nout vec2 frag_tex;\n\nout vec4 frag_col;\n\nvoid main(void)\n{\n\tgl_Position = vec4(vert_pos, 0.0, 1.0);\n\tfrag_tex = vert_tex;\n\tfrag_col = vert_col;\n}";
	const char* fsh_source = "#version 330 core\n\nin vec2 frag_tex;\nin vec4 frag_col;\n\nlayout(location = 0) out vec4 out_col;\n\nuniform sampler2D u_sampler;\n\nvoid main(void)\n{\n\tout_col = frag_col;\n\tfloat dist = texture(u_sampler, frag_tex).r;\n\tfloat w = fwidth(dist);\n\tout_col.a *= smoothstep(0.8 - w, 0.8 + w, dist);\n}";

	affe__ogl* ptr = (affe__ogl*)user_ptr;

	glGenVertexArrays(1, &ptr->vao);
	glGenBuffers(1, &ptr->vbo);
	glGenTextures(1, &ptr->texture);
	ptr->program = glCreateProgram();

	if (!ptr->vao) goto error;
	if (!ptr->vbo) goto error;
	if (!ptr->texture) goto error;
	if (!ptr->program) goto error;

	vsh = affe__ogl__make_shader(GL_VERTEX_SHADER, vsh_source);
	if (!vsh) goto error;

	fsh = affe__ogl__make_shader(GL_FRAGMENT_SHADER, fsh_source);
	if (!fsh) goto error;

	glAttachShader(ptr->program, vsh);
	glAttachShader(ptr->program, fsh);

	glLinkProgram(ptr->program);

	int status;
	glGetProgramiv(ptr->program, GL_LINK_STATUS, &status);

	if (!status) goto error;

	glDetachShader(ptr->program, vsh);
	glDetachShader(ptr->program, fsh);

	glDeleteShader(vsh);
	glDeleteShader(fsh);

	glBindBuffer(GL_ARRAY_BUFFER, ptr->vbo);
	glBufferData(GL_ARRAY_BUFFER, affe_buffer_size(ctx), NULL, GL_STREAM_DRAW);

	glBindVertexArray(ptr->vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(affe_vertex), (const void*)offsetof(affe_vertex, x));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(affe_vertex), (const void*)offsetof(affe_vertex, s));
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(affe_vertex), (const void*)offsetof(affe_vertex, r));
	
	glBindTexture(GL_TEXTURE_2D, ptr->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	return TRUE;
error:
	glDeleteVertexArrays(1, &ptr->vao);
	glDeleteBuffers(1, &ptr->vbo);
	glDeleteProgram(ptr->program);
	glDeleteTextures(1, &ptr->texture);
	glDeleteShader(vsh);
	glDeleteShader(fsh);

	return FALSE;
}

static void update(affe_context* ctx, void* user_ptr, int x, int y, int w, int h, void* pixels)
{
	affe__ogl* ptr = ((affe__ogl*)affe_user_ptr(ctx));
	glBindTexture(GL_TEXTURE_2D, ptr->texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_UNSIGNED_BYTE, pixels);
}

float af_linear_remap(float value, float min1, float max1, float min2, float max2) {
	return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

static void draw(affe_context* ctx, void* user_ptr, affe_vertex* verts, long long verts_count)
{
	affe__ogl* ptr = ((affe__ogl*)affe_user_ptr(ctx));

	for (long long i = 0; i < verts_count; ++i)
	{
		verts[i].x = af_linear_remap<float>(verts[i].x, 0, (float)ptr->w, -1.0f, 1.0f);
		verts[i].y = af_linear_remap<float>(verts[i].y, 0, (float)ptr->h, -1.0f, 1.0f);
	}

	glBindBuffer(GL_ARRAY_BUFFER, ptr->vbo);
	glBufferData(GL_ARRAY_BUFFER, affe_buffer_size(ctx), NULL, GL_STREAM_DRAW); // invalidation
	glBufferSubData(GL_ARRAY_BUFFER, 0, verts_count * sizeof(affe_vertex), verts);

	glBindVertexArray(ptr->vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ptr->texture);
	glUseProgram(ptr->program);
	glDrawArrays(GL_TRIANGLES, 0, verts_count);
}

static void destroy(affe_context* ctx, void* user_ptr)
{
	affe__ogl* ptr = ((affe__ogl*)affe_user_ptr(ctx));
	glDeleteVertexArrays(1, &ptr->vao);
	glDeleteBuffers(1, &ptr->vbo);
	glDeleteProgram(ptr->program);
	glDeleteTextures(1, &ptr->texture);
}

affe_context* affe_ogl3_context_create(int width, int height, int quads, int padding, int size)
{
	affe__ogl* impl;
	impl = (affe__ogl*)malloc(sizeof(affe__ogl));
	if (!impl) return NULL;
	memset(impl, 0, sizeof(affe__ogl));

	affe_context_create_info info;
	memset(&info, 0, sizeof(affe_context_create_info));

	info.width = width;
	info.height = height;
	info.user_ptr = impl;
	info.create_proc = &create;
	info.update_proc = &update;
	info.draw_proc = &draw;
	info.delete_proc = &destroy;
	
	info.buffer_quad_count = quads;
	info.edge_value = 0.8f;
	info.padding = padding;
	info.size = size;

	return affe_context_create(&info);
}

void affe_ogl3_context_delete(affe_context* ctx)
{
	void* user_ptr = affe_user_ptr(ctx);
	affe_context_delete(ctx);
	free(user_ptr);
}

void affe_ogl3_viewport(affe_context* ctx, int width, int height)
{
	((affe__ogl*)affe_user_ptr(ctx))->w = width;
	((affe__ogl*)affe_user_ptr(ctx))->h = height;
}

#endif // AFFE_OGL3_IMPLEMENTATION