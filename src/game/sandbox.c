// sandbox.c — Quality-of-life helpers for top-down game demos

#include "sandbox.h"
#include "../engine/lighting.h"
#include "../engine/math_common.h"
#include <math.h>
#include <stdio.h>

// ============================================================================
// TIME OF DAY SYSTEM
// ============================================================================

typedef struct {
    float hour;             // Current time (0-24)
    float seconds_per_hour; // Real seconds = 1 game hour
    int paused;
    char time_string[8];    // "HH:MM" format
} TimeOfDayState;

static TimeOfDayState g_time = {0};

// Default keyframes: { hour, sun_angle, ambient_r/g/b, sun_r/g/b, sun_intensity }
static const float TIME_KEYFRAMES[][9] = {
    //  Hour   Angle   Ambient (RGB)         Sun Color (RGB)        Intensity
    {   0.0f,  180.0f, 0.02f, 0.02f, 0.05f,  0.2f, 0.2f, 0.4f,      0.05f  },  // Midnight
    {   5.0f,  135.0f, 0.05f, 0.04f, 0.08f,  0.3f, 0.2f, 0.3f,      0.1f   },  // Pre-dawn
    {   7.0f,   90.0f, 0.15f, 0.12f, 0.1f,   1.0f, 0.6f, 0.4f,      0.4f   },  // Sunrise
    {  10.0f,   45.0f, 0.15f, 0.15f, 0.15f,  1.0f, 0.95f, 0.9f,     0.7f   },  // Morning
    {  12.0f,    0.0f, 0.2f,  0.2f,  0.2f,   1.0f, 1.0f, 0.95f,     0.8f   },  // Noon
    {  15.0f,  315.0f, 0.18f, 0.17f, 0.15f,  1.0f, 0.95f, 0.85f,    0.7f   },  // Afternoon
    {  18.0f,  270.0f, 0.15f, 0.1f,  0.08f,  1.0f, 0.5f, 0.3f,      0.5f   },  // Sunset
    {  20.0f,  225.0f, 0.08f, 0.06f, 0.1f,   0.4f, 0.3f, 0.5f,      0.2f   },  // Dusk
    {  24.0f,  180.0f, 0.02f, 0.02f, 0.05f,  0.2f, 0.2f, 0.4f,      0.1f  },  // Midnight (wrap)
};
#define TIME_KEYFRAME_COUNT (sizeof(TIME_KEYFRAMES) / sizeof(TIME_KEYFRAMES[0]))

void time_of_day_init(float starting_hour, float seconds_per_hour) {
    g_time.hour = clampf(starting_hour, 0.0f, 24.0f);
    g_time.seconds_per_hour = seconds_per_hour;
    g_time.paused = 0;
}

void time_of_day_pause(int paused) {
    g_time.paused = paused;
}

int time_of_day_is_paused(void) {
    return g_time.paused;
}

float time_of_day_get_hour(void) {
    return g_time.hour;
}

void time_of_day_set_hour(float hour) {
    g_time.hour = fmodf(hour, 24.0f);
    if (g_time.hour < 0) g_time.hour += 24.0f;
}

const char* time_of_day_get_string(void) {
    int hours = (int)g_time.hour;
    int minutes = (int)((g_time.hour - hours) * 60.0f);
    snprintf(g_time.time_string, sizeof(g_time.time_string), "%02d:%02d", hours, minutes);
    return g_time.time_string;
}

void time_of_day_update(float dt) {
    if (g_time.paused) return;
    
    // Advance time
    if (g_time.seconds_per_hour > 0.0f) {
        g_time.hour += dt / g_time.seconds_per_hour;
    }
    
    // Wrap at 24
    if (g_time.hour >= 24.0f) g_time.hour -= 24.0f;
    
    // Find keyframes we're between
    int idx_before = 0;
    int idx_after = 1;
    
    for (int i = 0; i < (int)TIME_KEYFRAME_COUNT - 1; i++) {
        if (g_time.hour >= TIME_KEYFRAMES[i][0] && g_time.hour < TIME_KEYFRAMES[i + 1][0]) {
            idx_before = i;
            idx_after = i + 1;
            break;
        }
    }
    
    // Interpolation factor
    float hour_before = TIME_KEYFRAMES[idx_before][0];
    float hour_after = TIME_KEYFRAMES[idx_after][0];
    float t = (g_time.hour - hour_before) / (hour_after - hour_before);
    
    const float *kf_a = TIME_KEYFRAMES[idx_before];
    const float *kf_b = TIME_KEYFRAMES[idx_after];
    
    float sun_angle = lerpf(kf_a[1], kf_b[1], t);
    Color ambient = {
        lerpf(kf_a[2], kf_b[2], t),
        lerpf(kf_a[3], kf_b[3], t),
        lerpf(kf_a[4], kf_b[4], t),
        1.0f
    };
    Color sun_color = {
        lerpf(kf_a[5], kf_b[5], t),
        lerpf(kf_a[6], kf_b[6], t),
        lerpf(kf_a[7], kf_b[7], t),
        1.0f
    };
    float sun_intensity = lerpf(kf_a[8], kf_b[8], t);
    
    // Apply to lighting system
    lighting_set_ambient(ambient);
    lighting_set_directional(sun_angle, sun_color, sun_intensity);
}

// ============================================================================
// MOVEMENT CONTROLLERS
// ============================================================================

// Internal: get normalized movement input
static void get_movement_input(float *out_dx, float *out_dy) {
    float dx = 0.0f, dy = 0.0f;
    get_move_input(&dx, &dy);
    *out_dx = dx;
    *out_dy = dy;
}

void movement_8dir(Entity *e, float dt) {
    float dx, dy;
    get_movement_input(&dx, &dy);
    
    // Calculate target velocity
    float target_vx = dx * e->max_speed;
    float target_vy = dy * e->max_speed;
    
    // Accelerate toward target (or let physics friction slow us down if no input)
    if (fabsf(dx) > 0.01f || fabsf(dy) > 0.01f) {
        e->vel_x = move_towardf(e->vel_x, target_vx, e->acceleration * dt);
        e->vel_y = move_towardf(e->vel_y, target_vy, e->acceleration * dt);
        // Face movement direction
        e->rotation = atan2f(dy, dx) * (180.0f / 3.14159f);
    }
    // Note: friction is applied in physics_update when no input
}

void movement_4dir(Entity *e, float dt) {
    float dx, dy;
    get_movement_input(&dx, &dy);
    
    // Snap to cardinal direction (prioritize larger axis)
    if (fabsf(dx) > fabsf(dy)) {
        dy = 0.0f;
        dx = dx > 0 ? 1.0f : (dx < 0 ? -1.0f : 0.0f);
    } else {
        dx = 0.0f;
        dy = dy > 0 ? 1.0f : (dy < 0 ? -1.0f : 0.0f);
    }
    
    // Calculate target velocity
    float target_vx = dx * e->max_speed;
    float target_vy = dy * e->max_speed;
    
    // Accelerate toward target
    if (fabsf(dx) > 0.01f || fabsf(dy) > 0.01f) {
        e->vel_x = move_towardf(e->vel_x, target_vx, e->acceleration * dt);
        e->vel_y = move_towardf(e->vel_y, target_vy, e->acceleration * dt);
        // Face movement direction
        e->rotation = atan2f(dy, dx) * (180.0f / 3.14159f);
    }
}

void movement_tank(Entity *e, float dt) {
    float dx, dy;
    get_movement_input(&dx, &dy);
    
    // Rotate with A/D (left/right input)
    float rotation_speed = 180.0f;  // degrees per second
    e->rotation += dx * rotation_speed * dt;
    
    // Move forward/backward based on current facing direction
    // Convention: rotation 0 = facing RIGHT (+X), same as look_at()
    float rad = e->rotation * (3.14159f / 180.0f);
    float forward_x = cosf(rad);
    float forward_y = sinf(rad);
    
    // W/S controls forward/backward
    // Note: dy is -1 for W (up), +1 for S (down) in screen coords
    // We negate so W = forward (positive along facing direction)
    float forward_input = -dy;
    
    if (fabsf(forward_input) > 0.01f) {
        float target_vx = forward_x * forward_input * e->max_speed;
        float target_vy = forward_y * forward_input * e->max_speed;
        e->vel_x = move_towardf(e->vel_x, target_vx, e->acceleration * dt);
        e->vel_y = move_towardf(e->vel_y, target_vy, e->acceleration * dt);
    }
}

void movement_strafe(Entity *e, GameState *state, float dt) {
    float dx, dy;
    get_movement_input(&dx, &dy);
    
    // Calculate target velocity
    float target_vx = dx * e->max_speed;
    float target_vy = dy * e->max_speed;
    
    // Accelerate toward target
    if (fabsf(dx) > 0.01f || fabsf(dy) > 0.01f) {
        e->vel_x = move_towardf(e->vel_x, target_vx, e->acceleration * dt);
        e->vel_y = move_towardf(e->vel_y, target_vy, e->acceleration * dt);
    }

    // Face mouse position using SCREEN-SPACE coordinates
    // This avoids flickering caused by camera movement during fixed timestep
    float screen_mouse_x, screen_mouse_y;
    get_mouse_pos(&screen_mouse_x, &screen_mouse_y);
    
    // Convert entity position to screen space
    float screen_center_x = (float)g_screen_width / 2.0f;
    float screen_center_y = (float)g_screen_height / 2.0f;
    float entity_screen_x = screen_center_x + (e->x - state->camera.x) * state->camera.zoom;
    float entity_screen_y = screen_center_y + (e->y - state->camera.y) * state->camera.zoom;
    
    // Calculate angle from entity (in screen space) to mouse (in screen space)
    float to_mouse_x = screen_mouse_x - entity_screen_x;
    float to_mouse_y = screen_mouse_y - entity_screen_y;
    
    // Only update rotation if mouse is far enough from entity (avoid jitter when overlapping)
    if (fabsf(to_mouse_x) > 1.0f || fabsf(to_mouse_y) > 1.0f) {
        e->rotation = atan2f(to_mouse_y, to_mouse_x) * (180.0f / 3.14159f);
    }
}

// --- CLICK-TO-MOVE ---
static struct {
    float target_x, target_y;
    int has_target;
    float arrival_distance;  // How close before we stop
} g_click_move = { 0, 0, 0, 10.0f };

void movement_click_set_target(float x, float y) {
    g_click_move.target_x = x;
    g_click_move.target_y = y;
    g_click_move.has_target = 1;
}

void movement_click_clear(void) {
    g_click_move.has_target = 0;
}

int movement_click_has_target(void) {
    return g_click_move.has_target;
}

void movement_click_get_target(float *x, float *y) {
    *x = g_click_move.target_x;
    *y = g_click_move.target_y;
}

void movement_click(Entity *e, GameState *state, float dt, int look_at_mouse) {
    // Check for new click - set target on left mouse button
    if (is_key_pressed(MOUSE_LEFT)) {
        float mx, my;
        get_world_mouse_pos(state, &mx, &my);
        movement_click_set_target(mx, my);
    }
    
    // Right click cancels movement
    if (is_key_pressed(MOUSE_RIGHT)) {
        movement_click_clear();
    }
    
    // Move toward target if we have one
    if (g_click_move.has_target) {
        float dx = g_click_move.target_x - e->x;
        float dy = g_click_move.target_y - e->y;
        float dist = sqrtf(dx * dx + dy * dy);
        
        // Check if we've arrived
        if (dist < g_click_move.arrival_distance) {
            movement_click_clear();
        } else {
            // Normalize direction and calculate target velocity
            float dir_x = dx / dist;
            float dir_y = dy / dist;
            float target_vx = dir_x * e->max_speed;
            float target_vy = dir_y * e->max_speed;
            
            // Accelerate toward target
            e->vel_x = move_towardf(e->vel_x, target_vx, e->acceleration * dt);
            e->vel_y = move_towardf(e->vel_y, target_vy, e->acceleration * dt);
            
            // Face movement direction (if not looking at mouse)
            if (!look_at_mouse) {
                e->rotation = atan2f(dir_y, dir_x) * (180.0f / 3.14159f);
            }
        }
    }
    
    // Always face mouse if look_at_mouse is enabled
    if (look_at_mouse) {
        float screen_mouse_x, screen_mouse_y;
        get_mouse_pos(&screen_mouse_x, &screen_mouse_y);
        
        float screen_center_x = (float)g_screen_width / 2.0f;
        float screen_center_y = (float)g_screen_height / 2.0f;
        float entity_screen_x = screen_center_x + (e->x - state->camera.x) * state->camera.zoom;
        float entity_screen_y = screen_center_y + (e->y - state->camera.y) * state->camera.zoom;
        
        float to_mouse_x = screen_mouse_x - entity_screen_x;
        float to_mouse_y = screen_mouse_y - entity_screen_y;
        
        if (fabsf(to_mouse_x) > 1.0f || fabsf(to_mouse_y) > 1.0f) {
            e->rotation = atan2f(to_mouse_y, to_mouse_x) * (180.0f / 3.14159f);
        }
    }
}

int movement_apply(Entity *e, GameState *state, MovementMode mode, float dt) {
    switch (mode) {
        case MOVE_MODE_8DIR:      movement_8dir(e, dt); break;
        case MOVE_MODE_4DIR:      movement_4dir(e, dt); break;
        case MOVE_MODE_TANK:      movement_tank(e, dt); break;
        case MOVE_MODE_STRAFE:    movement_strafe(e, state, dt); break;
        case MOVE_MODE_CLICK:     movement_click(e, state, dt, 0); break;  // Face movement
        case MOVE_MODE_CLICK_LOOK: movement_click(e, state, dt, 1); break; // Face mouse
    }
    
    // Return 1 if moving (velocity changed significantly)
    float speed = sqrtf(e->vel_x * e->vel_x + e->vel_y * e->vel_y);
    return speed > 1.0f ? 1 : 0;
}

// ============================================================================
// CAMERA HELPERS
// ============================================================================

void camera_follow_smooth(GameState *state, float target_x, float target_y, float smoothing) {
    float t = 1.0f - smoothing;
    state->camera.x = lerpf(state->camera.x, target_x, t);
    state->camera.y = lerpf(state->camera.y, target_y, t);
}

void camera_follow_deadzone(GameState *state, float target_x, float target_y,
                            float deadzone_w, float deadzone_h, float smoothing) {
    float dx = target_x - state->camera.x;
    float dy = target_y - state->camera.y;
    
    float target_cam_x = state->camera.x;
    float target_cam_y = state->camera.y;
    
    // Only move camera if target is outside deadzone
    if (fabsf(dx) > deadzone_w / 2.0f) {
        target_cam_x = target_x - (dx > 0 ? deadzone_w / 2.0f : -deadzone_w / 2.0f);
    }
    if (fabsf(dy) > deadzone_h / 2.0f) {
        target_cam_y = target_y - (dy > 0 ? deadzone_h / 2.0f : -deadzone_h / 2.0f);
    }
    
    float t = 1.0f - smoothing;
    state->camera.x = lerpf(state->camera.x, target_cam_x, t);
    state->camera.y = lerpf(state->camera.y, target_cam_y, t);
}

void camera_zoom_smooth(GameState *state, float target_zoom, float smoothing) {
    float t = 1.0f - smoothing;
    state->camera.zoom = lerpf(state->camera.zoom, target_zoom, t);
}

void camera_zoom_step(GameState *state, float step, float min_zoom, float max_zoom) {
    state->camera.zoom += step;
    state->camera.zoom = clampf(state->camera.zoom, min_zoom, max_zoom);
}

// ============================================================================
// DEBUG CONTROLS
// ============================================================================

void sandbox_toggle_time_pause(void) {
    g_time.paused = !g_time.paused;
}

void sandbox_skip_time(float hours) {
    time_of_day_set_hour(g_time.hour + hours);
}

void sandbox_toggle_shadows(void) {
    lighting_set_orthogonal(!lighting_is_orthogonal());
}

void sandbox_toggle_adaptive_lights(void) {
    lighting_set_adaptive(!lighting_is_adaptive());
}