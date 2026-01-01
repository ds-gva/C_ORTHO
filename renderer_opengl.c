// This is all built using OpenGL 1.1 through the standard header provided in windows (gl.h)
// we will upgrade this going forward

#define STB_IMAGE_IMPLEMENTATION

#include <windows.h>
#include <glad/glad.h>
#include <math.h>
#include "math_common.h"
#include "engine.h"
#include "stb_image.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

static Camera current_camera = {0.0f, 0.0f, 1.0f};

// Draw a rectangle //
void draw_rect(float x, float y, float w, float h, float rotation, Color color) {

    glColor4f(color.r, color.g, color.b, color.a);
    glPushMatrix();
    glTranslatef(x, y, 0.0f); // Translate the rectangle to the position

    // Rotate (Degrees)
    glRotatef(rotation, 0.0f, 0.0f, 1.0f); // Z-axis rotation

    float hw = w / 2.0f;
    float hh = h / 2.0f;
    
    // Draw the rectangle
    glBegin(GL_QUADS);
        glVertex2f(-hw, -hh); // Top Left
        glVertex2f( hw, -hh); // Top Right
        glVertex2f( hw,  hh); // Bottom Right
        glVertex2f(-hw,  hh); // Bottom Left
    glEnd();

    glPopMatrix();
}

// Draw a circle //
void draw_circle(float x, float y, float radius, float rotation, Color color) {
    glColor4f(color.r, color.g, color.b, color.a);

    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);

    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0.0f, 0.0f); // Center
        
        int segments = 64; // hard-coded, might wanna scale this based on the radius going forward
        for (int i = 0; i <= segments; i++) {
            float angle = i * TAU / segments;
            glVertex2f(cosf(angle) * radius, sinf(angle) * radius);
        }
    glEnd();

    glPopMatrix();
}

// Draw a line //
void draw_line(float x1, float y1, float x2, float y2, float thickness, Color color) {
    glColor4f(color.r, color.g, color.b, color.a);

    // Set the line thickness (in pixels)
    glLineWidth(thickness);

    glBegin(GL_LINES);
        glVertex2f(x1, y1); // Start point
        glVertex2f(x2, y2); // End point
    glEnd();
}

// these outlines will be used as boudning boxes for collision
// Draw a rectangle outline //
void draw_rect_outline(float x, float y, float w, float h, float rotation, Color color) {
    glColor4f(color.r, color.g, color.b, color.a);
    glLineWidth(2.0f);

    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);

    float hw = w / 2.0f;
    float hh = h / 2.0f;
    
    glBegin(GL_LINE_LOOP);
        glVertex2f(-hw, -hh);
        glVertex2f( hw, -hh);
        glVertex2f( hw,  hh);
        glVertex2f(-hw,  hh);
    glEnd();

    glPopMatrix();
}

// these outlines will be used as boudning boxes for collision
// Draw a circle outline //
void draw_circle_outline(float x, float y, float radius, float rotation, Color color) {
    glColor4f(color.r, color.g, color.b, color.a);
    glLineWidth(2.0f);

    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);

    glBegin(GL_LINE_LOOP);
        int segments = 32; // Don't need a perfect circle, just a rough outline ; maybe no need to scale this one
        for (int i = 0; i < segments; i++) {
            float angle = i * TAU / segments;
            glVertex2f(cosf(angle) * radius, sinf(angle) * radius);
        }
    glEnd();

    glPopMatrix();
}

// Set the camera position and zoom level//
// At the moment the camera is stored as a global in current_camera ; probably wanna change this going forward
void set_camera(float x, float y, float zoom) {
    current_camera.x = x;
    current_camera.y = y;
    current_camera.zoom = zoom;
}

// Begin the camera mode //
// This is used to push the camera matrix onto the stack ; basically we're moving the world to the camera position
// The idea is that we use squeeze all the rendering that needs to be camera-dependent between begin_camera_mode and end_camera_mode
void begin_camera_mode() {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); // Push the camera matrix onto the stack
    
    glTranslatef(g_screen_width/2.0f, g_screen_height/2.0f, 0.0f);
    
    glScalef(current_camera.zoom, current_camera.zoom, 1.0f);
    glTranslatef(-current_camera.x, -current_camera.y, 0.0f);
}

void end_camera_mode() {
    glPopMatrix(); // Pop the camera matrix off the stack
}

void destroy_texture(Texture *tex) {
    if (tex->id) {
        glDeleteTextures(1, &tex->id);
        tex->id = 0;
    }
}

void init_renderer() {
    // 1. SETUP THE VIEWPORT
    // Tell OpenGL the size of our window
    glViewport(0, 0, g_screen_width, g_screen_height);

    // 2. SETUP THE COORDINATE SYSTEM (The Matrix)
    // We want 2D logic: Top-Left is (0,0), Bottom-Right is (w,h)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // glOrtho(left, right, bottom, top, near, far)
    // Note: We flip bottom and top to make Y=0 at the top!
    glOrtho(0, g_screen_width, g_screen_height, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void clear_screen(Color color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

Texture load_texture(const char *filename) {
    Texture tex = {0};
    
    // 1. Load Image Data from Disk (CPU RAM)
    int channels;
    unsigned char *data = stbi_load(filename, &tex.width, &tex.height, &channels, 0);
    
    if (!data) {
        // Simple error handling for now
        // In a real engine, you'd log this to a console/file
        return tex; 
    }

    // 2. Generate Texture on GPU
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    // 3. Setup Filtering (Pixel Art Style)
    // GL_NEAREST = Crisp pixels (Minecraft style)
    // GL_LINEAR  = Blurry/Smooth (Standard 3D style)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Setup Wrapping (What happens if we draw outside the 0-1 range?)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 4. Upload Data to VRAM
    // Determine format based on channels (3 = RGB, 4 = RGBA)
    int format = (channels == 4) ? GL_RGBA : GL_RGB;
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, tex.width, tex.height, 0, format, GL_UNSIGNED_BYTE, data);

    // 5. Free CPU RAM (GPU has a copy now, we don't need raw bytes anymore)
    stbi_image_free(data);

    return tex;
}

void draw_texture(Texture texture, float x, float y, float w, float h, float rotation, Color tint) {
    // enable Texturing
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture.id);

    glColor4f(tint.r, tint.g, tint.b, tint.a); // We can tint sprites!

    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rotation, 0.0f, 0.0f, 1.0f);

    float hw = w / 2.0f;
    float hh = h / 2.0f;

    glBegin(GL_QUADS);
        // Top Left
        glTexCoord2f(0.0f, 0.0f); 
        glVertex2f(-hw, -hh);
        
        // Top Right
        glTexCoord2f(1.0f, 0.0f); 
        glVertex2f(hw, -hh);

        // Bottom Right
        glTexCoord2f(1.0f, 1.0f); 
        glVertex2f(hw, hh);

        // Bottom Left
        glTexCoord2f(0.0f, 1.0f); 
        glVertex2f(-hw, hh);
    glEnd();

    glPopMatrix();
    
    // Always disable textures after, or your colored rects will try to use the texture!
    glDisable(GL_TEXTURE_2D);
}