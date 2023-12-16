/* af_fontengine_impl_ogl3.h Last Updated: v0.1.8

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
AFFE_API void affe_ogl3_context_delete(affe_context* ctx);

#ifdef __cplusplus
}
#endif

#endif // AF_FONTENGINE_OGL3_H

#ifdef AFFE_OGL3_IMPLEMENTATION

// Use c++20 std::bit_cast if available
#if __cpp_lib_bit_cast == 201806L
#	include <bit>
#else
#	include <type_traits>
namespace std // Yes I know this is illegal, sue me :3
{
	template<class To, class From>
	std::enable_if_t<sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From>&& std::is_trivially_copyable_v<To>, To>
		// constexpr support needs compiler magic
		bit_cast(const From& src) noexcept {
		static_assert(std::is_trivially_constructible_v<To>, "This implementation additionally requires destination type to be trivially constructible");
		To dst;
		std::memcpy(&dst, &src, sizeof(To));
		return dst;
	}
}
#endif

struct affe__ogl
{
	GLuint program;
	GLuint texture;
	GLuint vao, vbo;
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

	int prev_unpack_alignment; glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack_alignment);
	int prev_texture_binding; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_texture_binding);

	glBindTexture(GL_TEXTURE_2D, ptr->texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_UNSIGNED_BYTE, pixels);

	glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack_alignment);
	glBindTexture(GL_TEXTURE_2D, std::bit_cast<unsigned int>(prev_texture_binding));
}

float af_linear_remap(float value, float min1, float max1, float min2, float max2) {
	return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

static void draw(affe_context* ctx, void* user_ptr, affe_vertex* verts, long long verts_count)
{
	affe__ogl* ptr = ((affe__ogl*)affe_user_ptr(ctx));

	for (long long i = 0; i < verts_count; ++i)
	{
		verts[i].x = af_linear_remap(verts[i].x, 0, (float)ctx->canvas_width, -1.0f, 1.0f);
		verts[i].y = af_linear_remap(verts[i].y, 0, (float)ctx->canvas_height, -1.0f, 1.0f);
	}

	// Update buffer contents
	{
		int prev_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_array_buffer);

		glBindBuffer(GL_ARRAY_BUFFER, ptr->vbo);
		glBufferData(GL_ARRAY_BUFFER, affe_buffer_size(ctx), NULL, GL_STREAM_DRAW); // invalidation
		glBufferSubData(GL_ARRAY_BUFFER, 0, verts_count * sizeof(affe_vertex), verts);

		glBindBuffer(GL_ARRAY_BUFFER, prev_array_buffer);
	}
	
	GLint prev_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vertex_array);
	GLint prev_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_texture);
	GLint prev_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_texture);
	GLboolean prev_blend_enabled; glGetBooleanv(GL_BLEND, &prev_blend_enabled);
	GLint prev_blend_src; glGetIntegerv(GL_BLEND_SRC_ALPHA, &prev_blend_src);
	GLint prev_blend_dst; glGetIntegerv(GL_BLEND_DST_ALPHA, &prev_blend_dst);
	GLint prev_program; glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);
	GLint prev_depthmask; glGetIntegerv(GL_DEPTH_WRITEMASK, &prev_depthmask);
	GLint prev_depthtest; glGetIntegerv(GL_DEPTH_TEST, &prev_depthtest);

	glBindVertexArray(ptr->vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ptr->texture);
	glUseProgram(ptr->program);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDrawArrays(GL_TRIANGLES, 0, verts_count);

	glBindVertexArray(std::bit_cast<GLuint>(prev_vertex_array));
	glUseProgram(std::bit_cast<GLuint>(prev_program));
	glActiveTexture(std::bit_cast<GLenum>(prev_active_texture));
	glBindTexture(GL_TEXTURE_2D, std::bit_cast<GLuint>(prev_texture));

	if (prev_depthmask) glDepthMask(GL_TRUE);
	if (prev_blend_enabled) glEnable(GL_BLEND);
	if (prev_depthtest) glEnable(GL_DEPTH_TEST);
	glBlendFunc(prev_blend_src, prev_blend_dst);
}

static void destroy(affe_context* ctx, void* user_ptr)
{
	affe__ogl* ptr = ((affe__ogl*)affe_user_ptr(ctx));
	glDeleteVertexArrays(1, &ptr->vao);
	glDeleteBuffers(1, &ptr->vbo);
	glDeleteProgram(ptr->program);
	glDeleteTextures(1, &ptr->texture);
}

static void error_proc(affe_context* ctx, void* error_ptr, int error)
{
	if (error == AFFE_ERROR_ATLAS_FULL) affe_cache_invalidate(ctx);
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
	info.error_proc = &error_proc;
	
	info.buffer_quad_count = quads;
	info.edge_value = 0.8f;
	info.padding = padding;
	info.size = (float)size;

	return affe_context_create(&info);
}

void affe_ogl3_context_delete(affe_context* ctx)
{
	void* user_ptr = affe_user_ptr(ctx);
	affe_context_delete(ctx);
	free(user_ptr);
}

#endif // AFFE_OGL3_IMPLEMENTATION