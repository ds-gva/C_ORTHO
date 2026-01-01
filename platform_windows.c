// Our platform-specific code ; currently runs on windows
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "engine.h"
#include "input.h"

// Link libraries
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")

// Global platform variables
int running = 1;
int g_screen_width = 1024;
int g_screen_height = 768;
int g_debug_draw = 1;

// Map Windows VK codes to engine keys
static int vk_to_engine_key(int vk) {
    switch (vk) {
        case VK_LEFT:   return KEY_LEFT;
        case VK_RIGHT:  return KEY_RIGHT;
        case VK_UP:     return KEY_UP;
        case VK_DOWN:   return KEY_DOWN;
        case 'A':       return KEY_A;
        case 'B':       return KEY_B;
        case 'C':       return KEY_C;
        case 'D':       return KEY_D;
        case 'E':       return KEY_E;
        case 'F':       return KEY_F;
        case 'G':       return KEY_G;
        case 'H':       return KEY_H;
        case 'I':       return KEY_I;
        case 'J':       return KEY_J;
        case 'K':       return KEY_K;
        case 'L':       return KEY_L;
        case 'M':       return KEY_M;
        case 'N':       return KEY_N;
        case 'O':       return KEY_O;
        case 'P':       return KEY_P;
        case 'Q':       return KEY_Q;
        case 'R':       return KEY_R;
        case 'S':       return KEY_S;
        case 'T':       return KEY_T;
        case 'U':       return KEY_U;
        case 'V':       return KEY_V;
        case 'W':       return KEY_W;
        case 'X':       return KEY_X;
        case 'Y':       return KEY_Y;
        case 'Z':       return KEY_Z;
        case VK_SPACE:  return KEY_SPACE;
        case VK_ESCAPE: return KEY_ESCAPE;
        case VK_RETURN: return KEY_ENTER;
        case VK_SHIFT:  return KEY_SHIFT;
        case VK_CONTROL:return KEY_CTRL;
        case VK_F1:     return KEY_F1;
        case VK_F2:     return KEY_F2;
        case VK_F3:     return KEY_F3;
        case VK_F4:     return KEY_F4;
        default:        return -1;
    }
}

// Boilerplate for windows //
// This is the window procedure for windows
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE || uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Main function for windows //
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {

    AllocConsole(); // Allocate a console for the program
    freopen("CONOUT$", "w", stdout); // Redirect the stdout to the console - this is for easier debugging ; probably wanna create a debug mode for this

    // WINDOW SETUP
    WNDCLASS wc = {0}; // Create a window class
    wc.lpfnWndProc = WindowProc; // Set the window procedure
    wc.hInstance = hInstance; // Set the instance handle
    wc.lpszClassName = "EngineHost"; // Set the class name
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // Set the style

    RegisterClass(&wc); // Register the window class
    RECT wr = {0, 0, g_screen_width, g_screen_height}; // Create a rectangle
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE); // Adjust the window rectangle
    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, "MiniC Engine", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top, 0, 0, hInstance, 0); // Create a window
    HDC hdc = GetDC(hwnd); // Get the device context

    // MEMORY SETUP
    PIXELFORMATDESCRIPTOR pfd = {0}; // Create a pixel format descriptor
    pfd.nSize = sizeof(pfd); // Set the size of the pixel format descriptor
    pfd.nVersion = 1; // Set the version of the pixel format descriptor
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; // Double Buffered is KEY
    pfd.iPixelType = PFD_TYPE_RGBA; // Set the pixel type
    pfd.cColorBits = 32; // Set the color bits

    int pixel_format_index = ChoosePixelFormat(hdc, &pfd); // Choose the pixel format
    SetPixelFormat(hdc, pixel_format_index, &pfd); // Set the pixel format

    // Create the "Render Context" to connect the GPU to the window
    HGLRC hglrc = wglCreateContext(hdc); // Create the render context
    wglMakeCurrent(hdc, hglrc); // Make the render context current

    ///////////////////////////////////////
    // This is where our engine comes in //
    ///////////////////////////////////////


    // Initialize the renderer
    init_renderer(); // Initialize the renderer

    // Initialize the game state
    static GameState state; // Create a game state
    init_game(&state); // Initialize the game state

    // Setup the timer
    LARGE_INTEGER perf_freq, last_counter, current_counter; // Create a timer
    QueryPerformanceFrequency(&perf_freq); // Get the performance frequency
    QueryPerformanceCounter(&last_counter); // Get the last counter
    float accumulator = 0.0f; // Create an accumulator
    const float FIXED_DT = 1.0f / 60.0f; // Create a fixed time step
    
    // Map the Windows VK codes to the engine keys
    int vk_codes[] = {VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, 
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    VK_SPACE, VK_ESCAPE, VK_RETURN, VK_SHIFT, VK_CONTROL,
    VK_F1, VK_F2, VK_F3, VK_F4};

    // This is the main loop for windows
    while (running) {

        // Check for messages
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // Update the keys
        for (int i = 0; i < sizeof(vk_codes)/sizeof(vk_codes[0]); i++) {
            int engine_key = vk_to_engine_key(vk_codes[i]);

            if (engine_key >= 0) {
                int is_down = (GetAsyncKeyState(vk_codes[i]) & 0x8000) != 0;
                input_update_key(engine_key, is_down);
            }
        }

        // Update the mouse
        input_update_key(MOUSE_LEFT, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
        input_update_key(MOUSE_RIGHT, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);

        // Get the mouse position
        POINT mouse_pos;
        GetCursorPos(&mouse_pos);
        ScreenToClient(hwnd, &mouse_pos);
        input_update_mouse((float)mouse_pos.x, (float)mouse_pos.y);

        // Process the input at the start of the frame
        input_begin_frame();

        // Debug toggle (F1)
        if (is_key_pressed(KEY_F1)) {
        g_debug_draw = !g_debug_draw;
        }

        // Update the timer
        QueryPerformanceCounter(&current_counter); // Get the current counter
        int64_t elapsed = current_counter.QuadPart - last_counter.QuadPart;        // Calculate the time elapsed since the last frame
        last_counter = current_counter; // Update the last counter
        float dt = (float)((double)elapsed / (double)perf_freq.QuadPart); // Calculate the delta time

        // Cap the delta time at 0.1 seconds
        if (dt > 0.1f) dt = 0.1f;
        accumulator += dt;

        // Update the game physics and game logic once the accumulator is greater than the fixed time step
        while (accumulator >= FIXED_DT) {
            engine_update(&state, FIXED_DT);           // AUTO: Physics
            update_game(&state, FIXED_DT);     // USER: Custom logic
            accumulator -= FIXED_DT;
        }

        engine_render(&state);   // Draw all entities
        render_game(&state);     // Custom rendering
        SwapBuffers(hdc);       // Swap the buffers
    }

    // Clean up
    close_game(&state); // Clean up the game state

    wglMakeCurrent(NULL, NULL);    // Clean up the OpenGL context
    wglDeleteContext(hglrc);       // Clean up the OpenGL context
    ReleaseDC(hwnd, hdc);          // Clean up the device context

    return 0; // Exit the program
}