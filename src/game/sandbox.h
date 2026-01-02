// sandbox.h — Quality-of-life helpers for top-down game demos
// Provides: time of day, movement controllers, camera helpers, debug controls

#ifndef SANDBOX_H
#define SANDBOX_H

#include "../engine/engine.h"

// ============================================================================
// TIME OF DAY SYSTEM
// ============================================================================
// Automatically cycles lighting through day/night with smooth interpolation

// Initialize with starting hour (0-24) and real seconds per game hour
void time_of_day_init(float starting_hour, float seconds_per_hour);

// Update the cycle (call every frame with delta time)
void time_of_day_update(float dt);

// Pause/resume the cycle
void time_of_day_pause(int paused);
int time_of_day_is_paused(void);

// Get/set current hour (0-24, wraps automatically)
float time_of_day_get_hour(void);
void time_of_day_set_hour(float hour);

// Get time as a string for debug display (e.g., "14:30")
const char* time_of_day_get_string(void);

// ============================================================================
// MOVEMENT CONTROLLERS
// ============================================================================
// Different movement styles for top-down games

// Movement mode for entity controllers
typedef enum {
    MOVE_MODE_8DIR,      // 8-directional, entity faces movement direction
    MOVE_MODE_4DIR,      // 4-directional (no diagonals)
    MOVE_MODE_TANK,      // Forward/back + rotation (vehicle-style)
    MOVE_MODE_STRAFE,    // 8-directional, entity faces mouse/target
    MOVE_MODE_CLICK,     // Click-to-move, faces movement direction
    MOVE_MODE_CLICK_LOOK // Click-to-move, always faces mouse
} MovementMode;

// Apply movement to entity based on mode
// - WASD/Arrow input is read automatically
// - For STRAFE mode, entity looks at mouse
// - Returns 1 if entity is moving, 0 if idle
int movement_apply(Entity *e, GameState *state, MovementMode mode, float dt);

// Manual movement helpers (if you need more control)
void movement_8dir(Entity *e, float dt);           // Standard 8-way movement
void movement_4dir(Entity *e, float dt);           // Cardinal directions only
void movement_tank(Entity *e, float dt);           // Vehicle-style (forward/back + rotate)
void movement_strafe(Entity *e, GameState *state, float dt);  // Move freely, face mouse
void movement_click(Entity *e, GameState *state, float dt, int look_at_mouse);  // Click-to-move

// Click-to-move helpers
void movement_click_set_target(float x, float y);  // Set target manually
void movement_click_clear(void);                   // Stop moving to target
int movement_click_has_target(void);               // Check if currently moving to target
void movement_click_get_target(float *x, float *y); // Get current target position

// ============================================================================
// CAMERA HELPERS
// ============================================================================

// Smooth follow with configurable smoothing (0.0 = instant, 1.0 = very slow)
void camera_follow_smooth(GameState *state, float target_x, float target_y, float smoothing);

// Follow with deadzone (camera only moves when target exits deadzone)
void camera_follow_deadzone(GameState *state, float target_x, float target_y, 
                            float deadzone_w, float deadzone_h, float smoothing);

// Zoom controls with clamping
void camera_zoom_smooth(GameState *state, float target_zoom, float smoothing);
void camera_zoom_step(GameState *state, float step, float min_zoom, float max_zoom);

// ============================================================================
// DEBUG CONTROLS
// ============================================================================
// Call this once per frame to enable common debug hotkeys

// Enables: T=time pause, Left/Right=time skip, L=shadows, A=adaptive lights, Q/E=zoom
void sandbox_debug_controls(GameState *state, float dt);

// Individual debug toggles (if you want custom key bindings)
void sandbox_toggle_time_pause(void);
void sandbox_skip_time(float hours);
void sandbox_toggle_shadows(void);
void sandbox_toggle_adaptive_lights(void);

#endif
