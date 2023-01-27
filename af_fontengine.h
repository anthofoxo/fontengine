/* af_fontengine.h - v0.1.5

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

Recent version history:
0.1.6 (2023-01-27)
	added a few safety checks
	added api function to get user pointer
	opengl3 implmentation included
0.1.5 (2023-01-26)
	fixed a bug in line splitting where multiple carriage returns would only output as one
	improved performance slightly in line splitting
	changed buffer behavior to buffer control
	added a codepoint iterater internally
	api error safety documented (more to do)
	horizontal alignment options
0.1.4 (2023-01-25)
	some in header documentation (to be improved)
	added buffer flush control (affe_text_draw, now takes advantage of this)
0.1.3 (2023-01-24)
	fixed some compiler warnings
	added "inline" text which ignores line breaks
	some size values are now ints instead of long long
0.1.2 (2023-01-20)
	updated copyright information
	added parameter to specify the buffer size
	added very simple newline and carriage return handling
	changed default fallback count
0.1.1 (2023-01-19)
	removed exising flags, they are default behaviour now
	added font rastization setttings
	larger default font size
	fixed flushing occuring for each quad
	changed default buffer sizing to allow full usage of the memory
	improved usage of utf8 decutting
0.1.0 (2023-01-19)
	initial release
*/

#ifndef AF_FONTENGINE_H
#define AF_FONTENGINE_H

#define AFFE_VERSION 0.1.5

#ifndef NULL
#	ifdef __cplusplus
#		define NULL 0
#	else
#		define NULL ((void*)0)
#	endif
#endif

#ifndef AFFE_API
#	ifdef AFFE_STATIC
#		define AFFE_API static
#	else
#		define AFFE_API extern
#	endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FALSE
#	define FALSE 0
#endif
#ifndef TRUE
#	define TRUE 1
#endif

	// Defines an invalid font
#define AFFE_INVALID -1

// Error states
#define AFFE_ERROR_STATES_UNDERFLOW 1
#define AFFE_ERROR_STATES_OVERFLOW 2
#define AFFE_ERROR_ATLAS_FULL 3

// Alignment
#define AFFE_ALIGN_LEFT (1 << 0)
#define AFFE_ALIGN_CENTER (1 << 1)
#define AFFE_ALIGN_RIGHT (1 << 2)

// Controls how the buffer is flushed
// Automatic: buffer never needs manually flushed
// None: buffer must be manually flushed at the end of the frame
#define AFFE_BUFFER_FLUSH_CONTROL_AUTOMATIC 0
#define AFFE_BUFFER_FLUSH_CONTROL_NONE 1

// Defined backend feature supprt, currently unused, primitive restart may be supported in the future
#define AFFE_FLAGS_NONE 0

typedef struct affe_context affe_context;

struct affe_vertex
{
	// TODO: change rgba floats to a single unsigned int, provide functions to easily convert between the two
	float x, y, s, t, r, g, b, a;
};

typedef struct affe_vertex affe_vertex;

struct affe_context_create_info
{
	// Initial size of the cache
	int width, height;

	// User functions, called while the engine operates
	void* user_ptr;
	int(*create_proc)(affe_context* ctx, void* user_ptr, int width, int height);
	void(*update_proc)(affe_context* ctx, void* user_ptr, int x, int y, int width, int height, void* pixels);
	void(*draw_proc)(affe_context* ctx, void* user_ptr, affe_vertex* verts, long long verts_count);
	void(*delete_proc)(affe_context* ctx, void* user_ptr);

	// Error function, called when an error occurs
	void* error_user_ptr;
	void(*error_proc)(affe_context* ctx, void* error_user_ptr, int error);

	// How many quads to allocate space for in the vertex buffer
	long long buffer_quad_count;

	// Currently unused
	unsigned int flags;

	// Rasterizer settings
	float edge_value;
	float size;
	int padding;
};

typedef struct affe_context_create_info affe_context_create_info;

// ----- context management -----

AFFE_API affe_context* affe_context_create(const affe_context_create_info* info);
AFFE_API void affe_context_delete(affe_context* ctx);

// ----- user ptr -----

// Errors: nofail error guarantee
AFFE_API void* affe_user_ptr(affe_context* ctx);

// ----- fonts -----

// Errors: nofail error guarantee
AFFE_API int affe_font_add(affe_context* ctx, void* data, int index, int take_ownership);

// Errors: no error guarantee
AFFE_API int affe_font_fallback(affe_context* ctx, int base, int fallback);

// ----- state management -----

// Errors: strong error guarantee
AFFE_API void affe_state_push(affe_context* ctx);

// Errors: strong error guarantee
AFFE_API void affe_state_pop(affe_context* ctx);

// Errors: nofail error guarantee
AFFE_API void affe_state_clear(affe_context* ctx);

// ----- state setting -----

AFFE_API void affe_set_size(affe_context* ctx, float size);
AFFE_API void affe_set_color(affe_context* ctx, float r, float g, float b, float a);
AFFE_API void affe_set_font(affe_context* ctx, int font);
AFFE_API void affe_set_alignment(affe_context* ctx, int alignment);

// ----- buffer control -----

// Controls the behavior of buffer flushing.
// `AFFE_BUFFER_FLUSH_CONTROL_AUTOMATIC` (default): Buffer is flushed at the end of text draws or when the buffer is filled.
// `AFFE_BUFFER_FLUSH_CONTROL_NONE`: Buffer is only flushed when filled or manually via `affe_buffer_flush`.
// 
// Notes: It is UB to use any other control value than listed above.
// Errors: nofail error guarantee
// See: `affe_buffer_flush`
AFFE_API void affe_buffer_flush_control(affe_context* ctx, int control);

// Manually flush the buffer.
// This is required at the end of each frame when buffer control is set to `AFFE_BUFFER_FLUSH_CONTROL_NONE`
// 
// Errors: nofail error guarantee
// See: `affe_buffer_flush_control`
AFFE_API void affe_buffer_flush(affe_context* ctx);

// Get the max size of the buffer in bytes
// Can be used in callbacks to allocate the buffer for the backend
// Errors:
// - nofail error guarantee
// - Returns 0 if an error occurs
AFFE_API long long affe_buffer_size(affe_context* ctx);

// ----- text drawing -----

AFFE_API void affe_text_draw(affe_context* ctx, float x, float y, const char* string, const char* end);

// Draws a string of text in a single line, ignoring all line endings
AFFE_API void affe_text_draw_inline(affe_context* ctx, float x, float y, const char* string, const char* end);

#ifdef __cplusplus
}
#endif

#endif // AF_FONTENGINE_H

#ifdef AFFE_IMPLEMENTATION

#ifndef AFFE_HASH_LUT_SIZE
#	define AFFE_HASH_LUT_SIZE 256
#endif
#ifndef AFFE_INIT_FONTS
#	define AFFE_INIT_FONTS 4
#endif
#ifndef AFFE_INIT_GLYPHS
#	define AFFE_INIT_GLYPHS 256
#endif
#ifndef AFFE_MAX_STATES
#	define AFFE_MAX_STATES 16
#endif
#ifndef AFFE_MAX_FALLBACKS
#	define AFFE_MAX_FALLBACKS 16
#endif

struct affe__glyph
{
	unsigned int codepoint;
	int index;
	int next;
	float size;
	int advance;
	int x0, y0, x1, y1;
	int s0, t0, s1, t1;
};

typedef struct affe__glyph affe__glyph;

struct affe__font
{
	stbtt_fontinfo metrics;

	void* data;
	unsigned char is_owner;

	affe__glyph* glyphs;
	int glyphs_capacity;
	int glyphs_count;

	int lut[AFFE_HASH_LUT_SIZE];

	int fallbacks[AFFE_MAX_FALLBACKS];
	int fallbacks_count;
};

typedef struct affe__font affe__font;

struct affe__state
{
	float size;
	float r, g, b, a;
	int font;
	int alignment;
};

typedef struct affe__state affe__state;

struct affe_context
{
	affe_context_create_info info;

	affe__font** fonts;
	long long fonts_capacity;
	int fonts_count;

	affe_vertex* verts;
	long long verts_count;

	affe__state states[AFFE_MAX_STATES];
	long long states_count;

	stbrp_context packer;
	stbrp_node* packer_nodes;
	int packer_nodes_count;

	int buffer_flush_control;
};

static void affe__font__free(affe__font* font)
{
	if (font == NULL) return;
	if (font->glyphs) free(font->glyphs);
	if (font->is_owner && font->data) free(font->data);
	free(font);
}

static int affe__font__alloc(affe_context* ctx)
{
	if (ctx->fonts_count + 1 > ctx->fonts_capacity)
	{
		ctx->fonts_capacity = ctx->fonts_capacity == 0 ? AFFE_INIT_FONTS : ctx->fonts_capacity * 2;
		affe__font** new_fonts = (affe__font**)realloc(ctx->fonts, ctx->fonts_capacity * sizeof(affe__font*));
		if (new_fonts == NULL) return AFFE_INVALID;
		ctx->fonts = new_fonts;
	}

	affe__font* font = (affe__font*)malloc(sizeof(affe__font));
	if (font == NULL) goto error;
	memset(font, 0, sizeof(affe__font));

	font->glyphs = (affe__glyph*)malloc(AFFE_INIT_GLYPHS * sizeof(affe__glyph));
	if (font->glyphs == NULL) goto error;
	font->glyphs_capacity = AFFE_INIT_GLYPHS;
	font->glyphs_count = 0;

	ctx->fonts[ctx->fonts_count] = font;
	return ctx->fonts_count++;

error:
	affe__font__free(font);

	return AFFE_INVALID;
}

int affe_font_add(affe_context* ctx, void* data, int index, int take_ownership)
{
	if (!ctx) return AFFE_INVALID;

	int font_index = affe__font__alloc(ctx);
	if (font_index == AFFE_INVALID) return AFFE_INVALID;

	affe__font* font = ctx->fonts[font_index];

	for (int i = 0; i < AFFE_HASH_LUT_SIZE; ++i)
		font->lut[i] = -1;

	font->data = data;
	font->is_owner = (unsigned char)take_ownership;

	if (!stbtt_InitFont(&font->metrics, (const unsigned char*)font->data, stbtt_GetFontOffsetForIndex((const unsigned char*)font->data, index)))
		goto error;

	return font_index;

error:
	affe__font__free(font);
	--ctx->fonts_count;
	return AFFE_INVALID;
}

int affe_font_fallback(affe_context* ctx, int base, int fallback)
{
	if (!ctx) return FALSE;

	affe__font* font_base = ctx->fonts[base];

	if (font_base->fallbacks_count < AFFE_MAX_FALLBACKS)
	{
		font_base->fallbacks[font_base->fallbacks_count++] = fallback;
		return TRUE;
	}

	return FALSE;
}

static affe__state* affe__state__get(affe_context* ctx)
{
	if (!ctx) return NULL;
	return &ctx->states[ctx->states_count - 1];
}

void* affe_user_ptr(affe_context* ctx)
{
	if (!ctx) return NULL;
	return ctx->info.user_ptr;
}

void affe_set_size(affe_context* ctx, float size)
{
	if (!ctx) return;
	affe__state__get(ctx)->size = size;
}

void affe_set_color(affe_context* ctx, float r, float g, float b, float a)
{
	if (!ctx) return;
	affe__state* state = affe__state__get(ctx);
	state->r = r;
	state->g = g;
	state->b = b;
	state->a = a;
}

void affe_set_font(affe_context* ctx, int font)
{
	if (!ctx) return;
	affe__state__get(ctx)->font = font;
}

void affe_set_alignment(affe_context* ctx, int alignment)
{
	if (!ctx) return;
	affe__state__get(ctx)->alignment = alignment;
}

void affe_buffer_flush_control(affe_context* ctx, int control)
{
	if (!ctx) return;
	ctx->buffer_flush_control = control;
}

void affe_state_push(affe_context* ctx)
{
	if (!ctx) return;

	if (ctx->states_count >= AFFE_MAX_STATES)
	{
		if (ctx->info.error_proc)
			ctx->info.error_proc(ctx, ctx->info.error_user_ptr, AFFE_ERROR_STATES_OVERFLOW);
		return;
	}
	if (ctx->states_count > 0)
		memcpy(&ctx->states[ctx->states_count], &ctx->states[ctx->states_count - 1], sizeof(affe__state));
	++ctx->states_count;
}

void affe_state_pop(affe_context* ctx)
{
	if (!ctx) return;

	if (ctx->states_count <= 1)
	{
		if (ctx->info.error_proc)
			ctx->info.error_proc(ctx, ctx->info.error_user_ptr, AFFE_ERROR_STATES_UNDERFLOW);
		return;
	}
	--ctx->states_count;
}

void affe_state_clear(affe_context* ctx)
{
	if (!ctx) return;

	affe__state* state = affe__state__get(ctx);
	state->size = 16.0f;
	state->r = 1.0f;
	state->g = 1.0f;
	state->b = 1.0f;
	state->a = 1.0f;
	state->font = AFFE_INVALID;
	state->alignment = AFFE_ALIGN_LEFT;
}

void affe_context_delete(affe_context* ctx)
{
	if (!ctx) return;

	if (ctx->info.delete_proc)
		ctx->info.delete_proc(ctx, ctx->info.user_ptr);

	for (int i = 0; i < ctx->fonts_count; ++i)
		affe__font__free(ctx->fonts[i]);

	if (ctx->verts) free(ctx->verts);
	if (ctx->packer_nodes) free(ctx->packer_nodes);
	if (ctx->fonts) free(ctx->fonts);
	free(ctx);
}

affe_context* affe_context_create(const affe_context_create_info* info)
{
	// Allocate context
	affe_context* ctx = (affe_context*)malloc(sizeof(affe_context));
	if (ctx == NULL) goto error;
	memset(ctx, 0, sizeof(affe_context));

	ctx->info = *info;

	// Setup rectangle packer
	ctx->packer_nodes_count = ctx->info.width;
	ctx->packer_nodes = (stbrp_node*)malloc(ctx->packer_nodes_count * sizeof(stbrp_node));
	if (ctx->packer_nodes == NULL) goto error;
	stbrp_init_target(&ctx->packer, ctx->info.width, ctx->info.height, ctx->packer_nodes, ctx->packer_nodes_count);

	// Invoke user create function
	if (ctx->info.create_proc != NULL)
	{
		if (ctx->info.create_proc(ctx, ctx->info.user_ptr, ctx->info.width, ctx->info.height) == FALSE)
			goto error;
	}

	// Allocate font
	ctx->fonts = (affe__font**)malloc(AFFE_INIT_FONTS * sizeof(affe__font*));
	if (ctx->fonts == NULL) goto error;
	memset(ctx->fonts, 0, AFFE_INIT_FONTS * sizeof(affe__font*));
	ctx->fonts_capacity = AFFE_INIT_FONTS;
	ctx->fonts_count = 0;

	// Allocate vertex buffer
	ctx->buffer_flush_control = AFFE_BUFFER_FLUSH_CONTROL_AUTOMATIC;
	ctx->verts = (affe_vertex*)malloc(ctx->info.buffer_quad_count * 6 * sizeof(affe_vertex));
	if (!ctx->verts) goto error;

	// Setup initial state
	affe_state_push(ctx);
	affe_state_clear(ctx);

	return ctx;
error:
	affe_context_delete(ctx);
	return NULL;
}

// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#define AFFE_UTF8_ACCEPT 0
#define AFFE_UTF8_REJECT 12

static unsigned int affe__decut(unsigned int* state, unsigned int* codep, unsigned int byte)
{
	static const unsigned char utf8d[] = {
		// The first part of the table maps bytes to character classes that
		// to reduce the size of the transition table and create bitmasks.
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

		// The second part is a transition table that maps a combination
		// of a state of the automaton and a character class to a state.
		0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
		12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
		12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
		12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
		12,36,12,12,12,12,12,12,12,12,12,12,
	};

	unsigned int type = utf8d[byte];

	*codep = (*state != AFFE_UTF8_ACCEPT) ?
		(byte & 0x3fu) | (*codep << 6) :
		(0xff >> type) & (byte);

	*state = utf8d[256 + *state + type];
	return *state;
}

void affe_buffer_flush(affe_context* ctx)
{
	if (!ctx) return;
	if (ctx->verts_count <= 0) return;

	if (ctx->info.draw_proc != NULL)
		ctx->info.draw_proc(ctx, ctx->info.user_ptr, ctx->verts, ctx->verts_count);

	ctx->verts_count = 0;
}

long long affe_buffer_size(affe_context* ctx)
{
	if (!ctx) return 0;
	return ctx->info.buffer_quad_count * 6 * sizeof(affe_vertex);
}

static unsigned int affe__hash(unsigned int a)
{
	a += ~(a << 15);
	a ^= (a >> 10);
	a += (a << 3);
	a ^= (a >> 6);
	a += ~(a << 11);
	a ^= (a >> 16);
	return a;
}

static affe__glyph* affe__glyph__alloc(affe__font* font)
{
	if (font->glyphs_count + 1 > font->glyphs_capacity)
	{
		font->glyphs_capacity = font->glyphs_capacity == 0 ? AFFE_INIT_GLYPHS : font->glyphs_capacity * 2;
		affe__glyph* new_alloc = (affe__glyph*)realloc(font->glyphs, font->glyphs_capacity * sizeof(affe__glyph));
		if (new_alloc == NULL) return NULL;
		font->glyphs = new_alloc;
	}

	return &font->glyphs[font->glyphs_count++];
}

static affe__glyph* affe__glyph__get(affe_context* ctx, affe__font* font, int codepoint, float size, int padding)
{
	affe__font* font_render = font;

	int hash = affe__hash(codepoint) & (AFFE_HASH_LUT_SIZE - 1);
	int i = font->lut[hash];
	while (i != -1)
	{
		if (font->glyphs[i].codepoint == codepoint && font->glyphs[i].size == size)
			return &font->glyphs[i];
		i = font->glyphs[i].next;
	}

	int glyph_index = stbtt_FindGlyphIndex(&font->metrics, codepoint);

	if (glyph_index == 0)
	{
		for (i = 0; i < font->fallbacks_count; ++i)
		{
			affe__font* font_fallback = ctx->fonts[font->fallbacks[i]];
			int fallback_index = stbtt_FindGlyphIndex(&font_fallback->metrics, codepoint);

			if (fallback_index != 0)
			{
				glyph_index = fallback_index;
				font_render = font_fallback;
				break;
			}
		}
	}

	float scale = stbtt_ScaleForPixelHeight(&font_render->metrics, size);

	int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	stbtt_GetGlyphBox(&font_render->metrics, glyph_index, &x0, &y0, &x1, &y1);

	stbrp_rect rect;
	memset(&rect, 0, sizeof(stbrp_rect));

	unsigned char* pixels = stbtt_GetGlyphSDF(&font_render->metrics, scale, glyph_index, padding, (unsigned char)(ctx->info.edge_value * 255.0f), 255.0f / (float)padding, &rect.w, &rect.h, NULL, NULL);

	if (pixels)
	{
		if (!stbrp_pack_rects(&ctx->packer, &rect, 1))
		{
			if (ctx->info.error_proc)
				ctx->info.error_proc(ctx, ctx->info.error_user_ptr, AFFE_ERROR_ATLAS_FULL);
			if (!stbrp_pack_rects(&ctx->packer, &rect, 1)) return NULL;
		}

		if (ctx->info.update_proc)
			ctx->info.update_proc(ctx, ctx->info.user_ptr, rect.x, rect.y, rect.w, rect.h, pixels);

		stbtt_FreeSDF(pixels, NULL);
	}

	int advance;
	stbtt_GetGlyphHMetrics(&font_render->metrics, glyph_index, &advance, NULL);

	affe__glyph* glyph = affe__glyph__alloc(font);
	if (glyph == NULL) return NULL;

	glyph->s0 = rect.x;
	glyph->t0 = rect.y + rect.h;
	glyph->s1 = rect.x + rect.w;
	glyph->t1 = rect.y;

	glyph->x0 = (int)(x0 - (float)padding / scale);
	glyph->y0 = (int)(y0 - (float)padding / scale);
	glyph->x1 = (int)(x1 + (float)padding / scale);
	glyph->y1 = (int)(y1 + (float)padding / scale);

	glyph->codepoint = codepoint;
	glyph->size = size;
	glyph->index = glyph_index;
	glyph->advance = advance;

	glyph->next = font->lut[hash];
	font->lut[hash] = font->glyphs_count - 1;

	return glyph;
}

struct affe__quad
{
	float x0, y0, x1, y1, s0, t0, s1, t1, r, g, b, a;
};

typedef struct affe__quad affe__quad;

int affe__text__line(const char* string, const char* end, const char** line_end, const char** next_start)
{
	if (!string) return FALSE;
	if (!end) end = string + strlen(string);

	// since null(\0) needs to be checked to split the final line, add one to ensure the last line gets output
	++end;

	int checking_crlf = FALSE;

	for (; string != end; ++string)
	{
		unsigned int codepoint = *string;

		if (checking_crlf)
		{
			if (codepoint == '\n')
			{
				*line_end = string - 1;
				*next_start = string + 1;
				return TRUE;
			}

			*line_end = string - 1;
			*next_start = string;
			return TRUE;
		}

		if (codepoint == '\r')
		{
			checking_crlf = TRUE;
			continue;
		}
		else if (codepoint == '\n')
		{
			*line_end = string;
			*next_start = string + 1;
			return TRUE;
		}
		else if (codepoint == '\0')
		{
			*line_end = string;
			*next_start = NULL;
			return TRUE;
		}
	}

	return FALSE;
}

void affe_text_draw(affe_context* ctx, float x, float y, const char* string, const char* end)
{
	if (!ctx) return;
	if (!end) end = string + strlen(string);

	affe__state* state = affe__state__get(ctx);
	if (state->font < 0 || state->font >= ctx->fonts_count) return;

	affe__font* font = ctx->fonts[state->font];
	if (font->data == NULL) return;

	const int prev_flush_control = ctx->buffer_flush_control;

	if (prev_flush_control == AFFE_BUFFER_FLUSH_CONTROL_AUTOMATIC)
		affe_buffer_flush_control(ctx, AFFE_BUFFER_FLUSH_CONTROL_NONE);

	// TODO: Store this information inside the font
	int ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&font->metrics, &ascent, &descent, &line_gap);
	int line_height = ascent + line_gap - descent;
	float scale = stbtt_ScaleForPixelHeight(&font->metrics, state->size);

	const char* line_end, * next_start;
	while (affe__text__line(string, end, &line_end, &next_start))
	{
		affe_text_draw_inline(ctx, x, y, string, line_end);
		y -= (float)line_height * scale;
		string = next_start;
	}

	if (prev_flush_control == AFFE_BUFFER_FLUSH_CONTROL_AUTOMATIC)
	{
		affe_buffer_flush(ctx);
		affe_buffer_flush_control(ctx, AFFE_BUFFER_FLUSH_CONTROL_AUTOMATIC);
	}
}

static int affe__codepoint_iterator(const char** string, const char* end)
{
	unsigned int codepoint = 0;
	unsigned int utf8state = AFFE_UTF8_ACCEPT;

	for (; (*string) != end; ++(*string))
	{
		unsigned int ret = affe__decut(&utf8state, &codepoint, *(const unsigned char*)(*string));
		if (ret == AFFE_UTF8_REJECT)
		{
			utf8state = AFFE_UTF8_ACCEPT;
			codepoint = 0;
		}
		if (ret != AFFE_UTF8_ACCEPT) continue;
		
		++(*string);
		return codepoint;
	}

	return FALSE;
}

static void affe__text_width(affe_context* ctx, const char* string, const char* end, int* left, int* right)
{
	if (!ctx) return;

	affe__state* state = affe__state__get(ctx);
	if (state->font < 0 || state->font >= ctx->fonts_count) return;

	affe__font* font = ctx->fonts[state->font];
	if (!font->data) return;

	if (!end) end = string + strlen(string);

	int lhs = INT_MAX;
	int rhs = INT_MIN;
	int cursor = 0;

	while (int codepoint = affe__codepoint_iterator(&string, end))
	{
		affe__glyph* glyph = affe__glyph__get(ctx, font, codepoint, ctx->info.size, ctx->info.padding);

		if (glyph)
		{
			int glyph_left = cursor + glyph->x0;
			int glyph_right = cursor + glyph->x1;

			if (glyph_left < lhs)
			{
				lhs = glyph_left;
			}
			if (glyph_right > rhs) rhs = glyph_right;

			cursor += glyph->advance;
		}
	}

	*left = lhs;
	*right = rhs;
}

void affe_text_draw_inline(affe_context* ctx, float x, float y, const char* string, const char* end)
{
	if (!ctx) return;

	affe__state* state = affe__state__get(ctx);
	if (state->font < 0 || state->font >= ctx->fonts_count) return;

	affe__font* font = ctx->fonts[state->font];
	if (!font->data) return;

	if (!end) end = string + strlen(string);

	float scale = stbtt_ScaleForPixelHeight(&font->metrics, state->size);

	// calculate alignment
	{
		int left, right;
		affe__text_width(ctx, string, end, &left, &right);

		float width = (float)(right - left) * scale;

		x -= (float)left * scale;

		if (state->alignment & AFFE_ALIGN_CENTER)
			x -= width * 0.5f;
		else if (state->alignment & AFFE_ALIGN_RIGHT)
			x -= width;
	}

	int prev_glyph_index = -1;

	while (int codepoint = affe__codepoint_iterator(&string, end))
	{
		affe__glyph* glyph = affe__glyph__get(ctx, font, codepoint, ctx->info.size, ctx->info.padding);

		if (glyph)
		{
			if (glyph->s0 != glyph->s1 && glyph->t0 != glyph->t1)
			{
				if (ctx->verts_count + 6 > ctx->info.buffer_quad_count * 6) affe_buffer_flush(ctx);

				affe__quad quad;

				quad.x0 = x + (float)glyph->x0 * scale;
				quad.y0 = y + (float)glyph->y0 * scale;
				quad.x1 = x + (float)glyph->x1 * scale;
				quad.y1 = y + (float)glyph->y1 * scale;

				quad.s0 = (float)glyph->s0 / (float)ctx->info.width;
				quad.t0 = (float)glyph->t0 / (float)ctx->info.height;
				quad.s1 = (float)glyph->s1 / (float)ctx->info.width;
				quad.t1 = (float)glyph->t1 / (float)ctx->info.height;

				quad.r = state->r;
				quad.g = state->g;
				quad.b = state->b;
				quad.a = state->a;

				ctx->verts[ctx->verts_count++] = affe_vertex(quad.x0, quad.y1, quad.s0, quad.t1, quad.r, quad.g, quad.b, quad.a);
				ctx->verts[ctx->verts_count++] = affe_vertex(quad.x0, quad.y0, quad.s0, quad.t0, quad.r, quad.g, quad.b, quad.a);
				ctx->verts[ctx->verts_count++] = affe_vertex(quad.x1, quad.y1, quad.s1, quad.t1, quad.r, quad.g, quad.b, quad.a);

				ctx->verts[ctx->verts_count++] = affe_vertex(quad.x1, quad.y1, quad.s1, quad.t1, quad.r, quad.g, quad.b, quad.a);
				ctx->verts[ctx->verts_count++] = affe_vertex(quad.x0, quad.y0, quad.s0, quad.t0, quad.r, quad.g, quad.b, quad.a);
				ctx->verts[ctx->verts_count++] = affe_vertex(quad.x1, quad.y0, quad.s1, quad.t0, quad.r, quad.g, quad.b, quad.a);
			}

			x += (float)glyph->advance * scale;
		}

		prev_glyph_index = glyph != NULL ? glyph->index : -1;
	}

	if (ctx->buffer_flush_control == AFFE_BUFFER_FLUSH_CONTROL_AUTOMATIC)
		affe_buffer_flush(ctx);
}

#endif // AFFE_IMPLEMENTATION