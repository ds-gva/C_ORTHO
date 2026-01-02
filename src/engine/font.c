// font.c — Text rendering using stb_truetype
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>
#include "font.h"
#include "utils.h"
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//  FONT ATLAS CONFIGURATION
#define ATLAS_WIDTH  512
#define ATLAS_HEIGHT 512
#define FIRST_CHAR   32   // Space
#define CHAR_COUNT   95   // 32-126 (printable ASCII)

// Glyph info for each character
typedef struct {
    float x0, y0, x1, y1;  // Texture coords (0-1)
    float xoff, yoff;       // Offset from cursor to top-left of glyph
    float xadvance;         // How much to advance cursor after this glyph
    float width, height;    // Pixel dimensions of the glyph
} GlyphInfo;

// Font structure
struct Font {
    Texture atlas;              // Font texture atlas
    GlyphInfo glyphs[CHAR_COUNT];
    float size;                 // Pixel size
    float ascent;               // Distance from baseline to top
    float descent;              // Distance from baseline to bottom (negative)
    float line_height;          // Recommended line spacing
};

// FONT CACHE
#define MAX_FONTS 16

typedef struct {
    char path[256];
    float size;
    Font* font;
} FontEntry;

static FontEntry font_cache[MAX_FONTS];
static int font_count = 0;

// External access to current texture for batch renderer
extern GLuint current_texture_id;
extern GLint g_texture_filter_mode;

// Forward declaration for batch flush
void flush_batch(void);

void font_init(void) {
    font_count = 0;
    memset(font_cache, 0, sizeof(font_cache));
}

void font_shutdown(void) {
    for (int i = 0; i < font_count; i++) {
        if (font_cache[i].font) {
            font_unload(font_cache[i].font);
            font_cache[i].font = NULL;
        }
    }
    font_count = 0;
}

// Find cached font
static Font* find_cached_font(const char* path, float size) {
    for (int i = 0; i < font_count; i++) {
        if (strcmp(font_cache[i].path, path) == 0 && 
            font_cache[i].size == size) {
            return font_cache[i].font;
        }
    }
    return NULL;
}

Font* font_load(const char* path, float size) {
    // Check cache
    Font* cached = find_cached_font(path, size);
    if (cached) return cached;
    
    // Load TTF file
    char* ttf_data = load_file_text(path);
    if (!ttf_data) {
        printf("ERROR: Could not load font file: %s\n", path);
        return NULL;
    }
    
    // Initialize stb_truetype
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, (unsigned char*)ttf_data, 0)) {
        printf("ERROR: Failed to parse font: %s\n", path);
        free(ttf_data);
        return NULL;
    }
    
    // Calculate scale for desired pixel size
    float scale = stbtt_ScaleForPixelHeight(&info, size);
    
    // Get font metrics
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    
    // Allocate font structure
    Font* font = (Font*)malloc(sizeof(Font));
    if (!font) {
        free(ttf_data);
        return NULL;
    }
    
    font->size = size;
    font->ascent = ascent * scale;
    font->descent = descent * scale;
    font->line_height = (ascent - descent + line_gap) * scale;
    
    // Create bitmap for atlas
    unsigned char* atlas_bitmap = (unsigned char*)calloc(ATLAS_WIDTH * ATLAS_HEIGHT, 1);
    
    // Pack glyphs into atlas
    int pen_x = 1;  // Start with 1px padding
    int pen_y = 1;
    int row_height = 0;
    
    for (int i = 0; i < CHAR_COUNT; i++) {
        int codepoint = FIRST_CHAR + i;
        
        // Get glyph metrics
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&info, codepoint, &advance, &lsb);
        
        // Get bounding box
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&info, codepoint, scale, scale, &x0, &y0, &x1, &y1);
        
        int glyph_w = x1 - x0;
        int glyph_h = y1 - y0;
        
        // Check if we need to wrap to next row
        if (pen_x + glyph_w + 1 >= ATLAS_WIDTH) {
            pen_x = 1;
            pen_y += row_height + 1;
            row_height = 0;
        }
        
        // Check if we ran out of vertical space
        if (pen_y + glyph_h + 1 >= ATLAS_HEIGHT) {
            printf("WARNING: Font atlas full, some glyphs not rendered\n");
            break;
        }
        
        // Render glyph to atlas
        if (glyph_w > 0 && glyph_h > 0) {
            stbtt_MakeCodepointBitmap(
                &info, 
                atlas_bitmap + pen_y * ATLAS_WIDTH + pen_x,
                glyph_w, glyph_h, 
                ATLAS_WIDTH,  // stride
                scale, scale,
                codepoint
            );
        }
        
        // Store glyph info
        font->glyphs[i].x0 = (float)pen_x / ATLAS_WIDTH;
        font->glyphs[i].y0 = (float)pen_y / ATLAS_HEIGHT;
        font->glyphs[i].x1 = (float)(pen_x + glyph_w) / ATLAS_WIDTH;
        font->glyphs[i].y1 = (float)(pen_y + glyph_h) / ATLAS_HEIGHT;
        font->glyphs[i].xoff = (float)x0;
        font->glyphs[i].yoff = (float)y0;
        font->glyphs[i].xadvance = advance * scale;
        font->glyphs[i].width = (float)glyph_w;
        font->glyphs[i].height = (float)glyph_h;
        
        // Advance pen
        pen_x += glyph_w + 1;
        if (glyph_h > row_height) row_height = glyph_h;
    }
    
    // Create OpenGL texture from atlas
    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    
    // Use linear filtering for smooth text
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Upload as single-channel texture (RED in OpenGL 3.3+)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS_WIDTH, ATLAS_HEIGHT, 
                 0, GL_RED, GL_UNSIGNED_BYTE, atlas_bitmap);
    
    // Set swizzle mask so RED channel appears in all RGB channels (grayscale)
    // This makes the font texture work with our color tinting
    GLint swizzle[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    
    font->atlas.id = tex_id;
    font->atlas.width = ATLAS_WIDTH;
    font->atlas.height = ATLAS_HEIGHT;
    
    // Cleanup
    free(atlas_bitmap);
    free(ttf_data);
    
    // Add to cache
    if (font_count < MAX_FONTS) {
        strncpy(font_cache[font_count].path, path, 255);
        font_cache[font_count].path[255] = '\0';
        font_cache[font_count].size = size;
        font_cache[font_count].font = font;
        font_count++;
    }
    
    printf("Loaded font: %s @ %.0fpx\n", path, size);
    return font;
}

void font_unload(Font* font) {
    if (!font) return;
    
    glDeleteTextures(1, &font->atlas.id);
    free(font);
}

// Internal: draw a single textured quad for a glyph
static void draw_glyph(Texture atlas, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1, Color color);

void draw_text(Font* font, const char* text, float x, float y, Color color) {
    if (!font || !text) return;
    
    // Flush if texture changes
    if (current_texture_id != font->atlas.id) {
        flush_batch();
        glBindTexture(GL_TEXTURE_2D, font->atlas.id);
        current_texture_id = font->atlas.id;
    }
    
    // Round starting position to avoid subpixel rendering artifacts
    float cursor_x = floorf(x + 0.5f);
    float cursor_y = floorf(y + font->ascent + 0.5f);
    
    for (const char* p = text; *p; p++) {
        char c = *p;
        
        // Handle newline
        if (c == '\n') {
            cursor_x = floorf(x + 0.5f);
            cursor_y += font->line_height;
            continue;
        }
        
        // Skip non-printable characters
        if (c < FIRST_CHAR || c >= FIRST_CHAR + CHAR_COUNT) {
            continue;
        }
        
        int idx = c - FIRST_CHAR;
        GlyphInfo* g = &font->glyphs[idx];
        
        // Calculate glyph position (relative to baseline)
        // Round to integer to get crisp rendering
        float gx = floorf(cursor_x + g->xoff + 0.5f);
        float gy = floorf(cursor_y + g->yoff + 0.5f);
        
        // Draw glyph quad
        if (g->width > 0 && g->height > 0) {
            draw_glyph(font->atlas, gx, gy, g->width, g->height,
                      g->x0, g->y0, g->x1, g->y1, color);
        }
        
        cursor_x += g->xadvance;
    }
}

void text_measure(Font* font, const char* text, float* out_width, float* out_height) {
    if (!font || !text) {
        if (out_width) *out_width = 0;
        if (out_height) *out_height = 0;
        return;
    }
    
    float width = 0;
    float max_width = 0;
    int lines = 1;
    
    for (const char* p = text; *p; p++) {
        char c = *p;
        
        if (c == '\n') {
            if (width > max_width) max_width = width;
            width = 0;
            lines++;
            continue;
        }
        
        if (c < FIRST_CHAR || c >= FIRST_CHAR + CHAR_COUNT) {
            continue;
        }
        
        int idx = c - FIRST_CHAR;
        width += font->glyphs[idx].xadvance;
    }
    
    if (width > max_width) max_width = width;
    
    if (out_width) *out_width = max_width;
    if (out_height) *out_height = lines * font->line_height;
}

// INTERNAL: Draw glyph using batch renderer
// We need to add vertices directly to the batch, similar to draw_texture but with custom UVs

// Access to the vertex buffer from renderer_opengl.c
typedef struct {
    float x, y;
    float r, g, b, a;
    float u, v;
    float type;
} Vertex;

extern Vertex vertices[];
extern int vertex_count;

#define MAX_VERTICES (10000 * 4)

static void draw_glyph(Texture atlas, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1, Color color) {
    // Buffer overflow check
    if (vertex_count + 4 >= MAX_VERTICES) {
        flush_batch();
        glBindTexture(GL_TEXTURE_2D, atlas.id);
        current_texture_id = atlas.id;
    }
    
    // No rotation for text - simple axis-aligned quad
    // Top-left, Top-right, Bottom-right, Bottom-left
    float positions[4][2] = {
        {x,     y},
        {x + w, y},
        {x + w, y + h},
        {x,     y + h}
    };
    
    float uvs[4][2] = {
        {u0, v0},
        {u1, v0},
        {u1, v1},
        {u0, v1}
    };
    
    for (int i = 0; i < 4; i++) {
        Vertex* v = &vertices[vertex_count + i];
        v->x = positions[i][0];
        v->y = positions[i][1];
        v->r = color.r;
        v->g = color.g;
        v->b = color.b;
        v->a = color.a;
        v->u = uvs[i][0];
        v->v = uvs[i][1];
        v->type = 0.0f;  // Regular textured quad
    }
    
    vertex_count += 4;
}
