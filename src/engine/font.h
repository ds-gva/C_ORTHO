// font.h — Text rendering API
#ifndef FONT_H
#define FONT_H

#include "engine.h"

// Opaque font handle
typedef struct Font Font;

// Initialize the font system (call once at startup)
void font_init(void);

// Shutdown the font system (call at cleanup)
void font_shutdown(void);

// Load a TTF font at a specific pixel size
// Returns NULL on failure
Font* font_load(const char* path, float size);

// Unload a font (frees resources)
void font_unload(Font* font);

// Draw text at screen/world position
// x, y = top-left corner of text
// color = text color
void draw_text(Font* font, const char* text, float x, float y, Color color);

// Measure text dimensions (useful for centering, UI layout)
void text_measure(Font* font, const char* text, float* out_width, float* out_height);

#endif
