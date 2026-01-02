#include "entity.h"
#include <stdio.h>
#include <string.h>

static void entity_set_defaults(Entity *e) {
    e->active = 1;  // Mark alive by default
    e->scale = 1.0f;
    e->color = (Color){1,1,1,1};
    e->mass  = 1.0f;
    e->drag = 0.95f;
    e->move_speed = 500.0f;
    e->collider.active = 1;
    
    // Depth sorting defaults
    e->sort_layer = SORT_LAYER_DEFAULT;
    e->z_order = 0;
    e->sort_offset_y = 0.0f;
    
    // Shadow defaults (off by default)
    e->casts_shadow = 0;
    e->shadow_offset = 10.0f;   // Offset in pixels
    e->shadow_scale = 1.0f;
    e->shadow_opacity = 0.8f;
}

Entity* entity_alloc(GameState *state) {
    // First, try to recycle a dead entity
    for (int i = 0; i < state->count; i++) {
        if (!state->entities[i].active) {
            Entity *e = &state->entities[i];
            
            uint32_t old_id = e->id;  // Preserve ID slot? No, assign new!
            memset(e, 0, sizeof(Entity));
            entity_set_defaults(e);

            e->id = state->next_id++;  // Assign NEW unique ID
            return e;
        }
    }
    
    // No recyclable slot, allocate new
    if (state->count >= MAX_ENTITIES) {
        printf("CRITICAL: Entity limit reached!\n");
        return NULL;
    }
    
    Entity *e = &state->entities[state->count++];
    memset(e, 0, sizeof(Entity));
    entity_set_defaults(e);

    e->id = state->next_id++;  // Assign unique ID
    return e;
}
void entity_destroy(Entity *e) {
    e->active = 0;
}

Entity* spawn_sprite(GameState *state, Texture *tex, float x, float y) {
    Entity *e = entity_alloc(state);
    if (!e) return NULL;
    
    e->x = x; e->y = y;
    e->visual_type = VISUAL_SPRITE;
    e->visual.sprite.texture = tex;
    e->visual.sprite.width = (float)tex->width;
    e->visual.sprite.height = (float)tex->height;
    
    // Default Collider matches sprite
    e->collider.type = SHAPE_RECT;
    e->collider.rect.width = (float)tex->width;
    e->collider.rect.height = (float)tex->height;
    
    return e;
}

Entity* spawn_primitive_wall(GameState *state, float x, float y, float w, float h) {
    Entity *e = entity_alloc(state);
    if (!e) return NULL;

    e->x = x;
    e->y = y;
    e->mass = 0.0f;        // Static
    e->restitution = 0.5f; // Bouncy
    e->color = COLOR_BLUE;

    // Visuals (Primitive)
    e->visual_type = SHAPE_RECT;
    e->visual.rect.width = w;
    e->visual.rect.height = h;

    // Collider (Primitive)
    e->collider.active = 1;
    e->collider.type = SHAPE_RECT;
    e->collider.rect.width = w;
    e->collider.rect.height = h;
    e->collider.layer = LAYER_WALL;
    e->collider.mask = LAYER_PLAYER | LAYER_ENEMY;

    return e;
}

// Spawn a bouncy ball with physics
Entity* spawn_ball(GameState *state, float x, float y, float radius, Color color) {
    Entity *e = entity_alloc(state);
    if (!e) return NULL;
    
    e->x = x;
    e->y = y;
    e->color = color;
    e->mass = radius * 0.1f;  // Heavier = bigger
    e->restitution = 0.9f;    // Bouncy!
    
    e->visual_type = SHAPE_CIRCLE;
    e->visual.circle.radius = radius;
    
    e->collider.type = SHAPE_CIRCLE;
    e->collider.circle.radius = radius;
    e->collider.layer = LAYER_ENEMY;  // Default layer
    e->collider.mask = LAYER_ENEMY | LAYER_WALL;
    
    return e;
}

// Spawn world bounds (4 walls)
void spawn_world_bounds(GameState *state, float width, float height) {
    float t = 20.0f;  // thickness
    spawn_primitive_wall(state, width/2, -t/2, width + t*2, t);      // Top
    spawn_primitive_wall(state, width/2, height + t/2, width + t*2, t); // Bottom
    spawn_primitive_wall(state, -t/2, height/2, t, height);          // Left
    spawn_primitive_wall(state, width + t/2, height/2, t, height);   // Right
}

// Find entity by unique ID (returns NULL if not found or inactive)
Entity* get_entity_by_id(GameState *state, uint32_t id) {
    for (int i = 0; i < state->count; i++) {
        Entity *e = &state->entities[i];
        if (e->active && e->id == id) {
            return e;
        }
    }
    return NULL;
}

// Find first entity with matching tag (bitmask)
Entity* find_entity_with_tag(GameState *state, uint32_t tag) {
    for (int i = 0; i < state->count; i++) {
        Entity *e = &state->entities[i];
        if (e->active && (e->tag & tag)) {
            return e;
        }
    }
    return NULL;
}

// Find ALL entities with matching tag, returns count found
int find_all_with_tag(GameState *state, uint32_t tag, Entity **out, int max) {
    int found = 0;
    for (int i = 0; i < state->count && found < max; i++) {
        Entity *e = &state->entities[i];
        if (e->active && (e->tag & tag)) {
            out[found++] = e;
        }
    }
    return found;
}