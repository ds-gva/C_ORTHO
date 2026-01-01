// 1. INCLUDE GLAD FIRST (Crucial!)
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include "engine.h"
#include "input.h"

// --- LINKER SETTINGS ---
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
// -----------------------

// Global Variables
int g_screen_width = 1024;
int g_screen_height = 768;
int g_debug_draw = 1;
int running = 1;

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
        default:
            if (key >= 'A' && key <= 'Z') return KEY_A + (key - 'A');
            return -1;
    }
}

// --- CALLBACKS ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_REPEAT) return; 
    int engine_key = glfw_to_engine_key(key);
    if (engine_key != -1) {
        input_update_key(engine_key, (action == GLFW_PRESS));
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    input_update_mouse((float)xpos, (float)ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    int key = -1;
    if (button == GLFW_MOUSE_BUTTON_LEFT) key = MOUSE_LEFT;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) key = MOUSE_RIGHT;
    if (key != -1) input_update_key(key, (action == GLFW_PRESS));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    g_screen_width = width;
    g_screen_height = height;
    glViewport(0, 0, width, height);
}

// --- MAIN ENTRY POINT ---
int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        printf("Failed to init GLFW\n");
        return -1;
    }

    // 2. Configure (Compatibility Profile allows glBegin/glEnd AND Modern GL)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    // 3. Create Window
    GLFWwindow* window = glfwCreateWindow(g_screen_width, g_screen_height, "MiniC Engine (GLAD)", NULL, NULL);
    if (!window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 4. LOAD OPENGL FUNCTIONS (GLAD)
    // This must happen AFTER window creation but BEFORE any drawing
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }
    printf("OpenGL Loaded: %s\n", glGetString(GL_VERSION));

    // 5. Setup Callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 6. Engine Initialization
    init_renderer();
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
        glfwSwapBuffers(window);
    }

    close_game(&state);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}