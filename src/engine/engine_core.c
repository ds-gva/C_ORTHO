// engine_core.c
#include "engine.h"
#include "physics.h"
#include "lighting.h"
#include <stdlib.h>
#include <math.h>

// Y-sorting toggle (default OFF)
int g_y_sort_enabled = 0;

void engine_update(GameState *state, float dt) {
    physics_update(state, dt);
}

// --- Y-SORTING ---
// We sort pointers to entities, not the entities themselves (faster, preserves array)
static Entity* sorted_entities[MAX_ENTITIES];
static int sorted_count = 0;

// Comparison function for qsort: layer → z_order → Y position
static int compare_entities_for_sort(const void* a, const void* b) {
    Entity* ea = *(Entity**)a;
    Entity* eb = *(Entity**)b;
    
    // Compare by sort layer (coarse grouping)
    if (ea->sort_layer != eb->sort_layer) {
        return ea->sort_layer - eb->sort_layer;
    }
    
    // Compare by z_order (fine control within layer)
    if (ea->z_order != eb->z_order) {
        return ea->z_order - eb->z_order;
    }
    
    // Same layer and z_order: sort by Y position (+ offset)
    float ya = ea->y + ea->sort_offset_y;
    float yb = eb->y + eb->sort_offset_y;
    
    if (ya < yb) return -1;
    if (ya > yb) return 1;
    return 0;
}

// Here we render all entities
void engine_render(GameState *state) {

    clear_screen(state->background); // Clear the screen to the background color
    enable_scissor_test();
    clear_game_area(COLOR_BLACK);

    
    set_camera(state->camera.x, state->camera.y, state->camera.zoom); // Set the camera position and zoom level

    begin_camera_mode();
    
    // Let game render world-space content first (tilemaps, backgrounds)
    render_world(state);
    
    // Build sorted list of active entities
    sorted_count = 0;
    for (int i = 0; i < state->count; i++) {
        if (state->entities[i].active) {
            sorted_entities[sorted_count++] = &state->entities[i];
        }
    }
    
    // Sort by layer, then by Y (if enabled)
    if (g_y_sort_enabled) {
        qsort(sorted_entities, sorted_count, sizeof(Entity*), compare_entities_for_sort);
    }
    
    // --- DROP SHADOW PASS ---
    // Draw shadows BEFORE entities (so shadows appear underneath)
    // Simple flat blob shadows offset by sun angle
    if (!lighting_is_orthogonal()) {
        // Get sun angle and calculate shadow direction
        float sun_angle = lighting_get_sun_angle();
        float rad = sun_angle * (3.14159f / 180.0f);
        
        // Shadow direction is opposite to sun direction
        // Sun at 0° (North) -> shadow points South (+Y)
        // Sun at 90° (East) -> shadow points West (-X)
        float dir_x = -sinf(rad);
        float dir_y = cosf(rad);
        
        for (int i = 0; i < sorted_count; i++) {
            Entity *e = sorted_entities[i];
            if (!e->casts_shadow) continue;
            
            float s = e->scale * e->shadow_scale;
            
            // Shadow position = entity + offset in sun direction
            float shadow_x = e->x + dir_x * e->shadow_offset;
            float shadow_y = e->y + dir_y * e->shadow_offset;
            
            // Calculate shadow fade from point lights
            float fade = lighting_get_shadow_fade(shadow_x, shadow_y);
            float final_opacity = e->shadow_opacity * (1.0f - fade * 0.8f);
            
            if (final_opacity < 0.01f) continue;
            
            Color shadow_color = (Color){0.0f, 0.0f, 0.0f, final_opacity};
            
            switch (e->visual_type) {
                case SHAPE_RECT: {
                    float sw = e->visual.rect.width * s;
                    float sh = e->visual.rect.height * s * 0.8f;
                    draw_rect(shadow_x, shadow_y, sw, sh, e->rotation, shadow_color, 0);
                    break;
                }
                case SHAPE_CIRCLE: {
                    float sr = e->visual.circle.radius * s;
                    draw_circle(shadow_x, shadow_y, sr, 0, shadow_color, 0);
                    break;
                }
                case VISUAL_SPRITE: {
                    float sw = e->visual.sprite.width * s;
                    float sh = e->visual.sprite.height * s * 0.8f;
                    draw_texture(*e->visual.sprite.texture, shadow_x, shadow_y, 
                                sw, sh, e->rotation, shadow_color);
                    break;
                }
            }
        }
    }
    
    // --- ENTITY PASS ---
    // Render all entities in sorted order
    for (int i = 0; i < sorted_count; i++) {
        Entity *e = sorted_entities[i];

        float s = e->scale;
        
        switch (e->visual_type) {
            case SHAPE_RECT:
                draw_rect(e->x, e->y, e->visual.rect.width * s, e->visual.rect.height * s, e->rotation, e->color, 0);
                break;
            case SHAPE_CIRCLE:
                draw_circle(e->x, e->y, e->visual.circle.radius * s, e->rotation, e->color, 0);
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
                draw_circle(cx, cy, e->collider.circle.radius, 0, debug_color, 1);
            } else if (e->collider.type == SHAPE_RECT) {
                draw_rect(cx, cy, e->collider.rect.width, e->collider.rect.height, 0, debug_color, 1);
            }
        }
    }

    
    end_camera_mode();
}