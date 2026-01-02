// game.c — Engine Tech Demo (using sandbox helpers)
#include "../engine/engine.h"
#include "../engine/entity.h"
#include "../engine/resources.h"
#include "../engine/utils.h"
#include "../engine/math_common.h"
#include "../engine/tilemap.h"
#include "../engine/lighting.h"
#include "../engine/font.h"
#include "sandbox.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

// User-defined tags
#define TAG_PLAYER  (1 << 0)
#define TAG_BARREL  (1 << 1)

// Game assets
static Texture* tex_boat;
static Texture* tex_tile;
static Texture* tex_barrel;
static Tilemap* test_map;

// Movement mode (can be changed at runtime)
static MovementMode player_movement = MOVE_MODE_CLICK_LOOK;
static Font* my_font;

int player_light;


// ============================================================================
// GAME INITIALIZATION
// ============================================================================

void init_game(GameState *state) {
    srand((unsigned int)time(NULL));
    set_texture_filter_mode(1);

    my_font = font_load("assets/fonts/OpenSans-Regular.ttf", 32.0f);
    
    // LOAD ASSETS
    tex_boat = resource_load_texture("assets/boat.png");
    tex_barrel = resource_load_texture("assets/barrel.png");
    tex_tile = resource_load_texture("assets/tile.png");
    
    // CREATE TILEMAP
    test_map = tilemap_create(25, 19, 32, 32);
    tilemap_fill(test_map, 0);

    lighting_set_adaptive(0);
    lighting_set_orthogonal(0);
    
    // Slightly dark, cool-tinted ambient (like twilight/shade)
    lighting_set_ambient((Color){0.35f, 0.38f, 0.45f, 1.0f});
    lighting_set_directional(90, COLOR_WHITE, 0.0f);  // No directional contribution

    // Warm orange/yellow player torch
    player_light = lighting_add_point(100, 100, 180.0f, (Color){1.0f, 0.6f, 0.25f, 1.0f}, 1.2f);
    
    // SETUP WORLD
    state->background = COLOR_BLACK;
    state->camera.x = 400;
    state->camera.y = 300;
    state->camera.zoom = 1.0f;
    
    spawn_world_bounds(state, 800, 600);
    
    // SPAWN PLAYER
    Entity *player = spawn_sprite(state, tex_boat, 400, 300);
    player->casts_shadow = 1;
    player->scale = 0.3f;
    player->tag = TAG_PLAYER;
    player->collider.layer = LAYER_PLAYER;
    player->collider.mask = LAYER_WALL | LAYER_ENEMY;
    player->restitution = 0.3f;
    player->collider.type = SHAPE_CIRCLE;
    player->collider.circle.radius = 25.0f;
    
    // SPAWN BARRELS
    for (int i = 0; i < 5; i++) {
        Entity *barrel = spawn_sprite(state, tex_barrel, randf(100, 700), randf(100, 500));
        barrel->casts_shadow = 1;
        barrel->scale = 0.6f;
        barrel->mass = 0.2f;
        barrel->friction = 0.0f;      // No friction
        barrel->restitution = 1.0f;   // Perfect bounce - no energy loss
        barrel->collider.type = SHAPE_CIRCLE;
        barrel->collider.circle.radius = 30.0f;
        barrel->tag = TAG_BARREL;
        barrel->vel_x = randf(-200, 200);
        barrel->vel_y = randf(-200, 200);
        barrel->collider.layer = LAYER_ENEMY;
        barrel->collider.mask = LAYER_WALL | LAYER_ENEMY | LAYER_PLAYER;
    }
}

// ============================================================================
// GAME UPDATE
// ============================================================================

void update_game(GameState *state, float dt) {
    Entity *player = find_entity_with_tag(state, TAG_PLAYER);
    if (!player) return;
    
    // PLAYER MOVEMENT (using sandbox helper)
    movement_apply(player, state, player_movement, dt);
    lighting_update_point(player_light, player->x, player->y);


    // CAMERA (using sandbox helper)
    camera_follow_smooth(state, player->x, player->y, 0.9f);
    
    // Demonstrate Camera Zoom
    if (is_key_down(KEY_Q)) {
        camera_zoom_step(state, 0.02f, 0.5f, 2.0f);
    }
    if (is_key_down(KEY_E)) {
        camera_zoom_step(state, -0.02f, 0.5f, 2.0f);
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void render_world(GameState *state) {
    // Render tilemap BEFORE entities
    tilemap_render_simple(test_map, tex_tile, 0, 0);
}

void render_game(GameState *state) {
    draw_text(my_font, "Hello World!", 50, 50, COLOR_WHITE);
}

// ============================================================================
// CLEANUP
// ============================================================================

void close_game(GameState *state) {
    tilemap_destroy(test_map);
    // Textures cleaned up by resources_shutdown()
}
