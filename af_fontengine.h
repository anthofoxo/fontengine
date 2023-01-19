/* af_fontengine.h - v0.1.0 - GPL-3.0-or-later

Authored from 2023 by AnthoFoxo
Huge credits to Sean Barrett and the community for making this project possible.
Additional credits to Mikko Mononen memon@inside.org for fontstash, inspiring the api design

This library is a text rendering engine designed to be scalable at any font size,
with emphesis on being platform and rendering api agnostic written in ansi c.
Additionally its designed to be as easy as possible to get started.

Required libraries:
stb_truetype.h - Used to load fonts and get font metrics along with creating sdf fields.
stb_rect_pack.h - Used to pack smaller rectangles into a larger.
See:
	https://github.com/nothings/stb/blob/master/stb_truetype.h
	https://github.com/nothings/stb/blob/master/stb_rect_pack.h
The library code does *NOTHING* to ensure these are included properly.
That responsibility is placed on you.

Along with the stb libraries. The following c standard library functions are used:
malloc, realloc, free, memset, memcpy, strlen

All functions and defines in the library are prefixed affe_ and AFFE_ respectivly,
with two exceptions: NULL, TRUE, FALSE; These will only be defined if not already defined.
Code not part of the public api will have an additional underscore in the name to denote this.

Example for including
Note the stb libraries only need included when implmentation is expanded:

#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define AFFE_IMPLEMENTATION
#include "stb_rect_pack.h"
#include "stb_truetype.h"
#include "af_fontengine.h"

The library does not support any rendering by default, you must implment this yourself,
however the library will create texture information and buffer information for you.

For example usage see: opengl3.cpp

Recent version history:
0.1.0 (2023-01-19) initial release
*/

#ifndef AF_FONTENGINE_H
#define AF_FONTENGINE_H

#define AFFE_VERSION 0.1.0

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

#define AFFE_INVALID -1

#define AFFE_ERROR_STATES_UNDERFLOW 1
#define AFFE_ERROR_STATES_OVERFLOW 2
#define AFFE_ERROR_ATLAS_FULL 3

#define AFFE_FLAGS_FLIP_UVS (1 << 0)
#define AFFE_FLAGS_NORMALIZE_UVS (1 << 1)

typedef struct affe_context affe_context;

struct affe_vertex
{
	float x, y, s, t, r, g, b, a;
};

typedef struct affe_vertex affe_vertex;

struct affe_context_create_info
{
	int width, height;
	void* user_ptr;
	int(*create_proc)(void* user_ptr, int width, int height);
	void(*update_proc)(void* user_ptr, int x, int y, int width, int height, void* pixels);
	void(*draw_proc)(void* user_ptr, affe_vertex* verts, long long verts_count);
	void(*delete_proc)(void* user_ptr);

	void* error_user_ptr;
	void(*error_proc)(void* error_user_ptr, int error);

	unsigned int flags;
};

typedef struct affe_context_create_info affe_context_create_info;

AFFE_API affe_context* affe_context_create(const affe_context_create_info* info);
AFFE_API void affe_context_delete(affe_context* ctx);

AFFE_API int affe_font_add(affe_context* ctx, void* data, int index, int take_ownership);
AFFE_API int affe_font_fallback(affe_context* ctx, int base, int fallback);

AFFE_API void affe_state_push(affe_context* ctx);
AFFE_API void affe_state_pop(affe_context* ctx);
AFFE_API void affe_state_clear(affe_context* ctx);

AFFE_API void affe_set_size(affe_context* ctx, float size);
AFFE_API void affe_set_color(affe_context* ctx, float r, float g, float b, float a);
AFFE_API void affe_set_font(affe_context* ctx, int font);

AFFE_API void affe_text_draw(affe_context* ctx, float x, float y, const char* string, const char* end);

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
#ifndef AFFE_VERTEX_COUNT
#	define AFFE_VERTEX_COUNT 512
#endif
#ifndef AFFE_MAX_STATES
#	define AFFE_MAX_STATES 20
#endif
#ifndef AFFE_MAX_FALLBACKS
#	define AFFE_MAX_FALLBACKS 20
#endif

struct affe__glyph
{
	unsigned int codepoint;
	int index;
	int next;
	int size;
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
};

typedef struct affe__state affe__state;

struct affe_context
{
	affe_context_create_info info;

	affe__font** fonts;
	long long fonts_capacity;
	long long fonts_count;

	affe_vertex verts[AFFE_VERTEX_COUNT];
	long long verts_count;

	affe__state states[AFFE_MAX_STATES];
	long long states_count;

	stbrp_context packer;
	stbrp_node* packer_nodes;
	long long packer_nodes_count;
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
	return &ctx->states[ctx->states_count - 1];
}

void affe_set_size(affe_context* ctx, float size)
{
	affe__state__get(ctx)->size = size;
}

void affe_set_color(affe_context* ctx, float r, float g, float b, float a)
{
	affe__state* state = affe__state__get(ctx);
	state->r = r;
	state->g = g;
	state->b = b;
	state->a = a;
}

void affe_set_font(affe_context* ctx, int font)
{
	affe__state__get(ctx)->font = font;
}

void affe_state_push(affe_context* ctx)
{
	if (ctx->states_count >= AFFE_MAX_STATES)
	{
		if (ctx->info.error_proc)
			ctx->info.error_proc(ctx->info.error_user_ptr, AFFE_ERROR_STATES_OVERFLOW);
		return;
	}
	if (ctx->states_count > 0)
		memcpy(&ctx->states[ctx->states_count], &ctx->states[ctx->states_count - 1], sizeof(affe__state));
	++ctx->states_count;
}

void affe_state_pop(affe_context* ctx)
{
	if (ctx->states_count <= 1)
	{
		if (ctx->info.error_proc)
			ctx->info.error_proc(ctx->info.error_user_ptr, AFFE_ERROR_STATES_UNDERFLOW);
		return;
	}
	--ctx->states_count;
}

void affe_state_clear(affe_context* ctx)
{
	affe__state* state = affe__state__get(ctx);
	state->size = 12.0f;
	state->r = 1.0f;
	state->g = 1.0f;
	state->b = 1.0f;
	state->a = 1.0f;
	state->font = AFFE_INVALID;
}

void affe_context_delete(affe_context* ctx)
{
	if (ctx == NULL) return;

	if (ctx->info.delete_proc)
		ctx->info.delete_proc(ctx->info.user_ptr);

	for (int i = 0; i < ctx->fonts_count; ++i)
		affe__font__free(ctx->fonts[i]);

	if (ctx->packer_nodes) free(ctx->packer_nodes);
	if (ctx->fonts) free(ctx->fonts);
	free(ctx);
}

affe_context* affe_context_create(const affe_context_create_info* info)
{
	affe_context* ctx = (affe_context*)malloc(sizeof(affe_context));
	if (ctx == NULL) goto error;
	memset(ctx, 0, sizeof(affe_context));

	ctx->info = *info;

	ctx->packer_nodes_count = ctx->info.width;

	ctx->packer_nodes = (stbrp_node*)malloc(ctx->packer_nodes_count * sizeof(stbrp_node));
	if (ctx->packer_nodes == NULL) goto error;

	stbrp_init_target(&ctx->packer, ctx->info.width, ctx->info.height, ctx->packer_nodes, ctx->packer_nodes_count);

	if (ctx->info.create_proc != NULL)
	{
		if (ctx->info.create_proc(ctx->info.user_ptr, ctx->info.width, ctx->info.height) == FALSE)
			goto error;
	}

	ctx->fonts = (affe__font**)malloc(AFFE_INIT_FONTS * sizeof(affe__font*));
	if (ctx->fonts == NULL) goto error;
	memset(ctx->fonts, 0, AFFE_INIT_FONTS * sizeof(affe__font*));
	ctx->fonts_capacity = AFFE_INIT_FONTS;
	ctx->fonts_count = 0;

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

static void affe__flush(affe_context* ctx)
{
	if (ctx->verts_count <= 0) return;

	if (ctx->info.draw_proc != NULL)
		ctx->info.draw_proc(ctx->info.user_ptr, ctx->verts, ctx->verts_count);

	ctx->verts_count = 0;
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

static affe__glyph* affe__glyph__get(affe_context* ctx, affe__font* font, int codepoint, int size, int padding)
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

	int x0, y0, x1, y1;
	stbtt_GetGlyphBox(&font_render->metrics, glyph_index, &x0, &y0, &x1, &y1);
	
	stbrp_rect rect;
	memset(&rect, 0, sizeof(stbrp_rect));

	unsigned char* pixels = stbtt_GetGlyphSDF(&font_render->metrics, scale, glyph_index, padding, 204, 32, &rect.w, &rect.h, NULL, NULL);

	if (pixels)
	{
		if (!stbrp_pack_rects(&ctx->packer, &rect, 1))
		{
			ctx->info.error_proc(ctx->info.error_user_ptr, AFFE_ERROR_ATLAS_FULL);
			if (!stbrp_pack_rects(&ctx->packer, &rect, 1)) return NULL;
		}

		if (ctx->info.update_proc)
			ctx->info.update_proc(ctx->info.user_ptr, rect.x, rect.y, rect.w, rect.h, pixels);

		stbtt_FreeSDF(pixels, NULL);
	}

	int advance;
	stbtt_GetGlyphHMetrics(&font_render->metrics, glyph_index, &advance, NULL);

	affe__glyph* glyph = affe__glyph__alloc(font);
	if (glyph == NULL) return NULL;

	glyph->s0 = rect.x;
	glyph->t0 = rect.y;
	glyph->s1 = rect.x + rect.w;
	glyph->t1 = rect.y + rect.h;
	
	glyph->x0 = x0 - (float)padding / scale;
	glyph->y0 = y0 - (float)padding / scale;
	glyph->x1 = x1 + (float)padding / scale;
	glyph->y1 = y1 + (float)padding / scale;

	glyph->codepoint = codepoint;
	glyph->size = size;
	glyph->index = glyph_index;
	glyph->advance = advance;

	glyph->next = font->lut[hash];
	font->lut[hash] = font->glyphs_count - 1;

	return glyph;
}

void affe_text_draw(affe_context* ctx, float x, float y, const char* string, const char* end)
{
	if (ctx == NULL) return;

	affe__state* state = affe__state__get(ctx);
	if (state->font < 0 || state->font >= ctx->fonts_count) return;

	affe__font* font = ctx->fonts[state->font];
	if (font->data == NULL) return;

	if (end == NULL) end = string + strlen(string);
	
	float scale = stbtt_ScaleForPixelHeight(&font->metrics, state->size);

	int prev_glyph_index = -1;
	unsigned int codepoint;
	unsigned int utf8state = 0;

	for (; string != end; ++string)
	{
		if (affe__decut(&utf8state, &codepoint, *(const unsigned char*)string))
			continue;

		affe__glyph* glyph = affe__glyph__get(ctx, font, codepoint, 32, 4);

		if (glyph != NULL)
		{
			if (glyph->s0 != glyph->s1 && glyph->t0 != glyph->t1)
			{
				if (ctx->verts_count + 6 > AFFE_VERTEX_COUNT) affe__flush(ctx);

				float x0 = x + (float)glyph->x0 * scale;
				float y0 = y + (float)glyph->y0 * scale;
				float x1 = x + (float)glyph->x1 * scale;
				float y1 = y + (float)glyph->y1 * scale;

				float s0 = glyph->s0;
				float t0 = glyph->t0;
				float s1 = glyph->s1;
				float t1 = glyph->t1;

				if (ctx->info.flags & AFFE_FLAGS_NORMALIZE_UVS)
				{
					s0 /= (float)ctx->info.width;
					t0 /= (float)ctx->info.height;
					s1 /= (float)ctx->info.width;
					t1 /= (float)ctx->info.height;
				}

				if (ctx->info.flags & AFFE_FLAGS_FLIP_UVS)
				{
					float temp = t1;
					t1 = t0;
					t0 = temp;
				}

				float r = state->r;
				float g = state->g;
				float b = state->b;
				float a = state->a;

				ctx->verts[ctx->verts_count++] = affe_vertex(x0, y1, s0, t1, r, g, b, a);
				ctx->verts[ctx->verts_count++] = affe_vertex(x0, y0, s0, t0, r, g, b, a);
				ctx->verts[ctx->verts_count++] = affe_vertex(x1, y1, s1, t1, r, g, b, a);

				ctx->verts[ctx->verts_count++] = affe_vertex(x1, y1, s1, t1, r, g, b, a);
				ctx->verts[ctx->verts_count++] = affe_vertex(x0, y0, s0, t0, r, g, b, a);
				ctx->verts[ctx->verts_count++] = affe_vertex(x1, y0, s1, t0, r, g, b, a);

				affe__flush(ctx);
			}

			x += (float)glyph->advance * scale;
		}
		prev_glyph_index = glyph != NULL ? glyph->index : -1;
	}	
}

#endif // AFFE_IMPLEMENTATION