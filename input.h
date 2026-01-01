#ifndef INPUT_H
#define INPUT_H

#include "engine.h"

// Platform layer calls these
void input_update_key(EngineKey key, int is_down);
void input_update_mouse(float x, float y);
void input_end_frame(void);
void input_begin_frame(void);

// User-facing API
int is_key_down(EngineKey key);
int is_key_pressed(EngineKey key);   // Just pressed this frame
int is_key_released(EngineKey key);  // Just released this frame

void get_move_input(float *out_x, float *out_y);  // WASD + Arrows combined
void get_mouse_pos(float *out_x, float *out_y);
void get_world_mouse_pos(GameState *state, float *out_x, float *out_y);

#endif