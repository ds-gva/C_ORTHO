// renderer_opengl.c — Modern OpenGL batch renderer

#include <glad/glad.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h> 
#include <stdlib.h> 
#include "math_common.h"
#include "engine.h"
#include "lighting.h"
#include "profiler.h"

GLint g_texture_filter_mode = GL_LINEAR;
GLuint current_texture_id = 0; // Tracks which texture is currently active

GLuint shader_program;

static Camera current_camera = {0.0f, 0.0f, 1.0f};
static int render_mode_camera = 0;

typedef struct {
    float x, y;       // Position
    float r, g, b, a; // Color
    float u, v;       // Texture Coordinates
    float type;
} Vertex;

#define MAX_QUADS 10000                     // 10k rects per draw call (plenty for 2D)
#define MAX_VERTICES (MAX_QUADS * 4)        // 4 verts per quad
#define MAX_INDICES (MAX_QUADS * 6)         // 6 indices per quad (2 triangles)

// --- BATCH RENDERER STATE ---
Vertex vertices[MAX_VERTICES];      // CPU Buffer
int vertex_count = 0;               // How many verts used so far?

GLuint VBO; // Vertex Buffer (GPU Memory for vertices)
GLuint VAO; // Vertex Array (State configuration)
GLuint IBO; // Index Buffer (GPU Memory for indices)
GLuint white_texture; // Default 1x1 white texture for untextured rects

void set_texture_filter_mode(int mode) {
    // Mode should be 0 (Nearest/Retro) or 1 (Linear/Smooth)
    if (mode == 0) g_texture_filter_mode = GL_NEAREST;
    else g_texture_filter_mode = GL_LINEAR;
}

// Helper to compile a single shader file
GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("SHADER ERROR (%s): %s\n", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog);
    }
    return shader;
}

// Combine them into a Program
void init_shaders() {
    // Load the shader source code from files
    char* vertex_shader_src = load_file_text("shaders/basic.vert");
    char* fragment_shader_src = load_file_text("shaders/basic.frag");

    if (!vertex_shader_src || !fragment_shader_src) {
        printf("FATAL: Failed to load shaders!\n");
        // Handle error (exit or fallback)
        return;
    }
    
    // Compile the shaders
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);

    
    free(vertex_shader_src);
    free(fragment_shader_src);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vs);
    glAttachShader(shader_program, fs);
    glLinkProgram(shader_program);

    // Check Link Errors
    int success;
    char infoLog[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        printf("SHADER LINK ERROR: %s\n", infoLog);
    }

    // Cleanup (We don't need the individual parts after linking)
    glDeleteShader(vs);
    glDeleteShader(fs);
}


void init_renderer_buffers() {
    //Create the VAO (The Container)
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create the VBO (The Vertex Data)
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Reserve huge space on GPU, but don't send data yet (DYNAMIC_DRAW)
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_DYNAMIC_DRAW);

    // Define Attributes (Layout)
    // Position (Location 0, 2 floats, offset 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);

    // Color (Location 1, 4 floats, offset 8)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);

    // TexCoord (Location 2, 2 floats, offset 24)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
    glEnableVertexAttribArray(2);

    // New Attribute: Type (Location 3, 1 float)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, type));
    glEnableVertexAttribArray(3);

    // Create the IBO (Index Buffer)
    // We use indices to reuse vertices. 1 Quad = 4 verts, but 6 indices (2 triangles).
    // This part is static; we calculate the pattern once and never change it.
    static uint32_t indices[MAX_INDICES];
    int offset = 0;
    for (int i = 0; i < MAX_INDICES; i += 6) {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;

        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;

        offset += 4;
    }

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Create a 1x1 White Texture
    glGenTextures(1, &white_texture);
    glBindTexture(GL_TEXTURE_2D, white_texture);
    uint32_t white_pixel = 0xffffffff;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white_pixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_texture_filter_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_texture_filter_mode);
    current_texture_id = white_texture;
}


void get_ortho_matrix(float *mat, float w, float h) {
    // Identity
    for(int i=0; i<16; i++) mat[i] = 0.0f;
    mat[0] = 2.0f / w;
    mat[5] = -2.0f / h; // Negative for Y-down
    mat[10] = -1.0f;
    mat[15] = 1.0f;
    mat[12] = -1.0f;
    mat[13] = 1.0f;
}

void flush_batch() {
    if (vertex_count == 0) return;
    
    // Record stats for profiler
    profiler_record_draw_call(vertex_count / 4);

    glUseProgram(shader_program);
    
    // Apply lighting uniforms
    lighting_apply();

    // Camera Logic
    // We construct a simple 2D View Matrix manually:
    // M = Translate(ScreenCenter) * Scale(Zoom) * Translate(-CamPos)
    float view[16] = {0};
    // Identity diagonal
    view[0] = 1.0f; view[5] = 1.0f; view[10] = 1.0f; view[15] = 1.0f;

    if (render_mode_camera) {
            // Scale (Zoom)
            view[0] = current_camera.zoom;
            view[5] = current_camera.zoom;
            
            // Translation
            float sw = (float)g_screen_width / 2.0f;
            float sh = (float)g_screen_height / 2.0f;
            
            view[12] = -current_camera.x * current_camera.zoom + sw;
            view[13] = -current_camera.y * current_camera.zoom + sh;
        }

    GLint locView = glGetUniformLocation(shader_program, "uView");
    glUniformMatrix4fv(locView, 1, GL_FALSE, view);

    // Projection Logic
    float ortho[16];
    get_ortho_matrix(ortho, (float)g_screen_width, (float)g_screen_height);
    GLint locProj = glGetUniformLocation(shader_program, "uProjection");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, ortho);

    // Texture Logic
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, current_texture_id);

    // Draw
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * sizeof(Vertex), vertices);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (vertex_count / 4) * 6, GL_UNSIGNED_INT, 0);

    vertex_count = 0;
}


// Draw a rectangle //
void draw_rect(float x, float y, float w, float h, float rotation, Color color, int hollow) {

    // Check for Texture Switch
    // If we were drawing sprites, and now want a solid rect, we must flush.
    if (current_texture_id != white_texture) {
        profiler_record_texture_switch();
        flush_batch();
        current_texture_id = white_texture;
    }

    // 2. Check for Buffer Overflow
    if (vertex_count + 4 >= MAX_VERTICES) {
        flush_batch();
    }

    // Rotation setup
    float hw = w / 2.0f;
    float hh = h / 2.0f;
    float c = cosf(rotation * DEG2RAD);
    float s = sinf(rotation * DEG2RAD);

    // Local offsets for 4 corners (TL, TR, BR, BL)
    float lx[] = {-hw,  hw,  hw, -hw};
    float ly[] = {-hh, -hh,  hh,  hh};

    // Texture Coords (Full Quad)
    float u[] = {0.0f, 1.0f, 1.0f, 0.0f};
    float v[] = {0.0f, 0.0f, 1.0f, 1.0f};

    // Fill Buffer
    for (int i = 0; i < 4; i++) {
        Vertex *vert = &vertices[vertex_count + i];
        
        // Rotate: x' = x*cos - y*sin
        //         y' = x*sin + y*cos
        float rx = lx[i] * c - ly[i] * s;
        float ry = lx[i] * s + ly[i] * c;

        // Translate (World Position)
        vert->x = x + rx;
        vert->y = y + ry;
        
        vert->r = color.r;
        vert->g = color.g;
        vert->b = color.b;
        vert->a = color.a;

        vert->u = u[i];
        vert->v = v[i];
        if (hollow == 0) {
            vert->type = 0.0f;
        }
        else vert->type = 3.0f;
    }

    vertex_count += 4;
}

// Draw a circle //
void draw_circle(float x, float y, float radius, float rotation, Color color, int hollow) {
    // Texture Switch (Circles use the white texture)
    if (current_texture_id != white_texture) {
        profiler_record_texture_switch();
        flush_batch();
        current_texture_id = white_texture;
    }

    if (vertex_count + 4 >= MAX_VERTICES) flush_batch();

    // Math (Same as rect we are drawing a square bounding box)
    // The "radius" is half the width/height
    float r = radius; 
    float c = cosf(rotation * DEG2RAD);
    float s = sinf(rotation * DEG2RAD);

    // Local offsets for a square centered at 0
    float lx[] = {-r,  r,  r, -r};
    float ly[] = {-r, -r,  r,  r};
    
    float u[] = {0.0f, 1.0f, 1.0f, 0.0f};
    float v[] = {0.0f, 0.0f, 1.0f, 1.0f};

    // 3. Fill Buffer
    for (int i = 0; i < 4; i++) {
        Vertex *vert = &vertices[vertex_count + i];
        
        float rx = lx[i] * c - ly[i] * s;
        float ry = lx[i] * s + ly[i] * c;

        vert->x = x + rx;
        vert->y = y + ry;
        
        vert->r = color.r;
        vert->g = color.g;
        vert->b = color.b;
        vert->a = color.a;

        vert->u = u[i];
        vert->v = v[i];
        
        if (hollow == 0) {
            vert->type = 1.0f;
        }
        else vert->type = 2.0f;
        
    }

    vertex_count += 4;
}

void draw_texture(Texture texture, float x, float y, float w, float h, float rotation, Color tint) {
    // Texture Switching Logic
    // If the requested texture is different from the active one, we must draw what we have so far
    // and then switch textures.
    if (texture.id != current_texture_id) {
        profiler_record_texture_switch();
        flush_batch();
        current_texture_id = texture.id;
    }

    // Buffer Check
    if (vertex_count + 4 >= MAX_VERTICES) {
        flush_batch();
    }

    // Math (Same as draw_rect)
    float hw = w / 2.0f;
    float hh = h / 2.0f;
    float c = cosf(rotation * DEG2RAD);
    float s = sinf(rotation * DEG2RAD);

    float lx[] = {-hw,  hw,  hw, -hw};
    float ly[] = {-hh, -hh,  hh,  hh};
    
    // UV Coordinates (Top-Left is 0,0)
    float u[] = {0.0f, 1.0f, 1.0f, 0.0f};
    float v[] = {0.0f, 0.0f, 1.0f, 1.0f};

    // Fill Vertices
    for (int i = 0; i < 4; i++) {
        Vertex *vert = &vertices[vertex_count + i];
        
        float rx = lx[i] * c - ly[i] * s;
        float ry = lx[i] * s + ly[i] * c;

        vert->x = x + rx;
        vert->y = y + ry;
        
        vert->r = tint.r;
        vert->g = tint.g;
        vert->b = tint.b;
        vert->a = tint.a;

        vert->u = u[i]; // <--- Actual UVs here
        vert->v = v[i];
        vert->type = 0.0f;
    }

    vertex_count += 4;
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
    // If we were already in camera mode, do nothing
    if (render_mode_camera == 1) return;

    // Flush whatever UI/Background was queued up
    flush_batch(); 

    // Set flag so next batch uses the camera matrix
    render_mode_camera = 1;
}

// End the Camera mode//
// Anything outside of this will not be affected by the camera
void end_camera_mode() {
    // If we were already in UI mode, do nothing
    if (render_mode_camera == 0) return;

    // Flush whatever World entities were queued up
    flush_batch();

    // Set flag so next batch uses the Identity matrix
    render_mode_camera = 0;
}


void init_renderer() {
    // SETUP THE VIEWPORT
    // Tell OpenGL the size of our window
    glViewport(0, 0, g_screen_width, g_screen_height);

    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_SCISSOR_TEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    init_shaders();
    init_renderer_buffers();
    
    // Initialize lighting with safe defaults (shadows off, neutral ambient)
    init_lighting();
}


void clear_screen() {
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void clear_game_area(Color color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void enable_scissor_test() {
    glEnable(GL_SCISSOR_TEST);
}
