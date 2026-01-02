// 1. INCLUDE GLAD FIRST (Crucial!)
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include "../engine/engine.h"
#include "../engine/input.h"
#include "../engine/resources.h"
#include "../engine/font.h"

// --- LINKER SETTINGS ---
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
// -----------------------

// --- GLOBALS ---
const int LOGICAL_WIDTH = 1024;
const int LOGICAL_HEIGHT = 768;

// Physical Window Size (Updated by GLFW)
int g_window_width = 1024;
int g_window_height = 768;

// Render Viewport (Calculated for Letterboxing)
struct { float x, y, w, h, scale; } viewport = {0};

// Engine Globals
int g_screen_width = 1024;
int g_screen_height = 768;
int g_debug_draw = 0;
int running = 1;

int is_fullscreen = 0;
int saved_x, saved_y, saved_w, saved_h; // To restore window after fullscreen

// --- INPUT MAPPING ---
static int glfw_to_engine_key(int key) {
    switch (key) {
        case GLFW_KEY_LEFT:     return KEY_LEFT;
        case GLFW_KEY_RIGHT:    return KEY_RIGHT;
        case GLFW_KEY_UP:       return KEY_UP;
        case GLFW_KEY_DOWN:     return KEY_DOWN;
        case GLFW_KEY_SPACE:    return KEY_SPACE;
        case GLFW_KEY_ESCAPE:   return KEY_ESCAPE;
        case GLFW_KEY_ENTER:    return KEY_ENTER;
        case GLFW_KEY_LEFT_SHIFT: return KEY_SHIFT;
        case GLFW_KEY_LEFT_CONTROL: return KEY_CTRL;
        case GLFW_KEY_F1:       return KEY_F1;
        case GLFW_KEY_TAB:      return KEY_TAB;
        case GLFW_KEY_BACKSPACE: return KEY_BACKSPACE;
        default:
            // Letters A-Z
            if (key >= 'A' && key <= 'Z') return KEY_A + (key - 'A');
            // Numbers 0-9
            if (key >= '0' && key <= '9') return KEY_0 + (key - '0');
            
            return -1;
    }
}

void update_viewport() {
    float target_aspect = (float)LOGICAL_WIDTH / (float)LOGICAL_HEIGHT;
    float window_aspect = (float)g_window_width / (float)g_window_height;
    
    if (window_aspect > target_aspect) {
        // Window is too wide (black bars on sides)
        viewport.h = (float)g_window_height;
        viewport.w = viewport.h * target_aspect;
        viewport.y = 0;
        viewport.x = (g_window_width - viewport.w) / 2.0f;
    } else {
        // Window is too tall (black bars on top/bottom)
        viewport.w = (float)g_window_width;
        viewport.h = viewport.w / target_aspect;
        viewport.x = 0;
        viewport.y = (g_window_height - viewport.h) / 2.0f;
    }
    
    viewport.scale = viewport.w / (float)LOGICAL_WIDTH;
    
    // Update OpenGL Viewport (Scissor Test clamps drawing to this area)
    glViewport((GLint)viewport.x, (GLint)viewport.y, (GLint)viewport.w, (GLint)viewport.h);
    glScissor((GLint)viewport.x, g_window_height - ((GLint)viewport.y + (GLint)viewport.h), (GLint)viewport.w, (GLint)viewport.h);
}


void toggle_fullscreen(GLFWwindow* window) {
    if (!is_fullscreen) {
        // SAVE current state
        glfwGetWindowPos(window, &saved_x, &saved_y);
        glfwGetWindowSize(window, &saved_w, &saved_h);
        
        // Switch to Fullscreen (Native Monitor Resolution)
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        // RESTORE windowed state
        glfwSetWindowMonitor(window, NULL, saved_x, saved_y, saved_w, saved_h, 0);
    }
    is_fullscreen = !is_fullscreen;
}


// --- CALLBACKS ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_F11) {
        toggle_fullscreen(window);
        return; // Don't pass F11 to game
    }
    if (action == GLFW_REPEAT) return; 
    
    int engine_key = glfw_to_engine_key(key);
    if (engine_key != -1) {
        input_update_key(engine_key, (action == GLFW_PRESS));
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // Convert Physical Mouse -> Logical Mouse
    // Subtract Black Bar offset
    float mx = (float)xpos - viewport.x;
    float my = (float)ypos - viewport.y;
    
    // Un-scale
    mx /= viewport.scale;
    my /= viewport.scale;
    
    // Clamp (Optional: prevent inputs outside the game area)
    // if (mx < 0) mx = 0; if (mx > LOGICAL_WIDTH) mx = LOGICAL_WIDTH; ...

    input_update_mouse(mx, my);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    int key = -1;
    if (button == GLFW_MOUSE_BUTTON_LEFT) key = MOUSE_LEFT;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) key = MOUSE_RIGHT;
    if (key != -1) input_update_key(key, (action == GLFW_PRESS));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    g_window_width = width;
    g_window_height = height;
    update_viewport();
    
    // Force a re-render immediately so we don't see artifacts during drag
    // (Optional, requires exposing render loop or triggering an event)
}


// --- MAIN ENTRY POINT ---
int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        printf("Failed to init GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    // Set correct initial size
    g_screen_width = LOGICAL_WIDTH;
    g_screen_height = LOGICAL_HEIGHT;
    // Create Window
    GLFWwindow* window = glfwCreateWindow(g_screen_width, g_screen_height, "C_ORTHO2D Mini Game Engine", NULL, NULL);
    if (!window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }


    glfwMakeContextCurrent(window);
    // LOAD OPENGL FUNCTIONS (GLAD)
    // This must happen AFTER window creation but BEFORE any drawing
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }
    printf("OpenGL Loaded: %s\n", glGetString(GL_VERSION));
    glfwSwapInterval(1); // Enable V-Sync

    
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    
    // Manually trigger the callback to calculate scale/black bars
    framebuffer_size_callback(window, w, h);

    // Setup Callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Engine Initialization
    init_renderer();
    font_init();
    GameState state = {0};
    init_game(&state);

    float last_frame_time = 0.0f;
    float accumulator = 0.0f;
    const float FIXED_DT = 1.0f / 60.0f;

    // --- GAME LOOP ---
    while (!glfwWindowShouldClose(window)) {
        float current_time = (float)glfwGetTime();
        float dt = current_time - last_frame_time;
        last_frame_time = current_time;
        if (dt > 0.1f) dt = 0.1f;
        accumulator += dt;

        input_begin_frame();
        glfwPollEvents();

        if (is_key_pressed(KEY_F1)) g_debug_draw = !g_debug_draw;
        if (is_key_pressed(KEY_ESCAPE)) glfwSetWindowShouldClose(window, 1);

        while (accumulator >= FIXED_DT) {
            engine_update(&state, FIXED_DT);
            update_game(&state, FIXED_DT);
            accumulator -= FIXED_DT;
        }

        engine_render(&state);
        render_game(&state);
        flush_batch();
        glfwSwapBuffers(window);
    }

    close_game(&state);
    font_shutdown();
    resources_shutdown();  // Free all cached resources
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}