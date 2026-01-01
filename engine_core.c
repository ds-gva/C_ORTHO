// engine_core.c
#include "engine.h"
#include "physics.h"

void engine_update(GameState *state, float dt) {
    physics_update(state, dt);
}

// Here we render all entities
void engine_render(GameState *state) {

    clear_screen(state->background); // Clear the screen to the background color
    
    set_camera(state->camera.x, state->camera.y, state->camera.zoom); // Set the camera position and zoom level
    begin_camera_mode(); // Begin the camera mode
    
    // Render all entities
    for (int i = 0; i < state->count; i++) {
        Entity *e = &state->entities[i];
        if (!e->active) continue;

        float s = e->scale;
        
        switch (e->visual_type) {
            case SHAPE_RECT:
                draw_rect(e->x, e->y, e->visual.rect.width * s, e->visual.rect.height * s, e->rotation, e->color);
                break;
            case SHAPE_CIRCLE:
                draw_circle(e->x, e->y, e->visual.circle.radius * s, e->rotation, e->color);
                break;
            case VISUAL_SPRITE:
                draw_texture(*e->visual.sprite.texture, e->x, e->y, 
                            e->visual.sprite.width * s, e->visual.sprite.height * s, 
                            e->rotation, e->color);
                break;
        }
    }

    // Render debug draw (for collision boxes)
    if (g_debug_draw) {
        for (int i = 0; i < state->count; i++) {
            Entity *e = &state->entities[i];
            if (!e->active || !e->collider.active) continue;
            
            float cx = e->x + e->collider.offset_x;
            float cy = e->y + e->collider.offset_y;
            
            // Green = normal, Red = colliding
            Color debug_color = e->collider.is_colliding ? COLOR_RED : COLOR_GREEN;
            
            if (e->collider.type == SHAPE_CIRCLE) {
                draw_circle_outline(cx, cy, e->collider.circle.radius, 0, debug_color);
            } else if (e->collider.type == SHAPE_RECT) {
                draw_rect_outline(cx, cy, e->collider.rect.width, e->collider.rect.height, 0, debug_color);
            }
        }
    }
    
    end_camera_mode();
}