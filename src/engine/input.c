#include "engine.h"
#include <string.h>

// Internal state (updated by platform layer)
static struct {
    int current[KEY_COUNT];
    int previous[KEY_COUNT];

    int just_pressed[KEY_COUNT];
    int just_released[KEY_COUNT];

    float mouse_x, mouse_y;
} input_state = {0};

void input_begin_frame(void) {
    // ACCUMULATE edges: only SET flags, don't clear them
    for (int i = 0; i < KEY_COUNT; i++) {
        if (input_state.current[i] && !input_state.previous[i]) {
            input_state.just_pressed[i] = 1;
        }
        if (!input_state.current[i] && input_state.previous[i]) {
            input_state.just_released[i] = 1;
        }
    }
    memcpy(input_state.previous, input_state.current, sizeof(input_state.current));
}

// Called by platform layer each frame
void input_update_key(EngineKey key, int is_down) {
    if (key < KEY_COUNT) {
        input_state.current[key] = is_down;
    }
}

void input_update_mouse(float x, float y) {
    input_state.mouse_x = x;
    input_state.mouse_y = y;
}

void input_end_frame(void) {
    // Clear any unconsumed edge flags at end of frame
    memset(input_state.just_pressed, 0, sizeof(input_state.just_pressed));
    memset(input_state.just_released, 0, sizeof(input_state.just_released));
}


// User-facing API
int is_key_down(EngineKey key) {
    return key < KEY_COUNT && input_state.current[key];
}

int is_key_pressed(EngineKey key) {
    if (key < KEY_COUNT && input_state.just_pressed[key]) {
        input_state.just_pressed[key] = 0;  // Consume!
        return 1;
    }
    return 0;
}

int is_key_released(EngineKey key) {
    if (key < KEY_COUNT && input_state.just_released[key]) {
        input_state.just_released[key] = 0;  // Consume!
        return 1;
    }
    return 0;
}

void get_move_input(float *out_x, float *out_y) {
    *out_x = 0.0f;
    *out_y = 0.0f;
    
    // WASD + Arrows
    if (is_key_down(KEY_LEFT)  || is_key_down(KEY_A)) *out_x -= 1.0f;
    if (is_key_down(KEY_RIGHT) || is_key_down(KEY_D)) *out_x += 1.0f;
    if (is_key_down(KEY_UP)    || is_key_down(KEY_W)) *out_y -= 1.0f;
    if (is_key_down(KEY_DOWN)  || is_key_down(KEY_S)) *out_y += 1.0f;
}

void get_mouse_pos(float *out_x, float *out_y) {
    *out_x = input_state.mouse_x;
    *out_y = input_state.mouse_y;
}

void get_world_mouse_pos(GameState *state, float *out_x, float *out_y) {
   
    float offset_x = (input_state.mouse_x - g_screen_width / 2.0f) / state->camera.zoom;
    float offset_y = (input_state.mouse_y - g_screen_height / 2.0f) / state->camera.zoom;
    *out_x = state->camera.x + offset_x;
    *out_y = state->camera.y + offset_y;
}