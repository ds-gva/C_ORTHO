// game.c — Engine Tech Demo
#include "../engine/engine.h"
#include "../engine/entity.h"
#include "../engine/resources.h"
#include "../engine/utils.h"
#include "../engine/math_common.h"
#include "../engine/tilemap.h"
#include "../engine/lighting.h"
#include <stdlib.h>
#include <time.h>

// User-defined tags
#define TAG_PLAYER  (1 << 0)
#define TAG_BALL    (1 << 1)

static Texture* tex_boat;
static Texture* tex_tile;
static Tilemap* test_map;
static int player_light;  // Light that follows the player

void init_game(GameState *state) {
    srand((unsigned int)time(NULL));

    set_texture_filter_mode(1);
    
    // Load textures
    tex_boat = resource_load_texture("assets/boat.png");
    tex_tile = resource_load_texture("assets/tile.png");  // You'll need to add this image
    
    // Create a test tilemap (25x19 tiles, each 32x32 pixels)
    test_map = tilemap_create(25, 19, 32, 32);
    tilemap_fill(test_map, 0);  // Fill entire map with tile ID 0 (meaning "draw")
    
    
    // Static campfire in the corner
    lighting_add_point(100, 100, 120.0f, (Color){1.0f, 0.5f, 0.2f, 1.0f}, 0.9f);
    
    // Setup world
    state->background = COLOR_BLACK;
    state->camera.x = 400;
    state->camera.y = 300;
    state->camera.zoom = 1.0f;
    
    spawn_world_bounds(state, 800, 600);
    
    // Spawn player as SPRITE instead of ball
    Entity *player = spawn_sprite(state, tex_boat, 400, 300);
    player->casts_shadow = 1;

    player->scale = 0.3f;
    player->tag = TAG_PLAYER;
    player->collider.layer = LAYER_PLAYER;
    player->collider.mask = LAYER_WALL | LAYER_ENEMY;
    player->restitution = 0.3f;
    
    // Use circle collider for smoother movement (optional)
    player->collider.type = SHAPE_CIRCLE;
    player->collider.circle.radius = 25.0f;  // Adjust to fit boat
    
    // Spawn initial bouncing balls
    Color colors[] = {COLOR_RED, COLOR_BLUE, COLOR_GREEN, COLOR_YELLOW};
    for (int i = 0; i < 8; i++) {
        Entity *ball = spawn_ball(state, randf(100, 700), randf(100, 500), randf(15, 35), colors[i % 4]);
        ball->casts_shadow = 1;
        ball->tag = TAG_BALL;
        ball->vel_x = randf(-200, 200);
        ball->vel_y = randf(-200, 200);
    }
}

void update_game(GameState *state, float dt) {
    Entity *player = find_entity_with_tag(state, TAG_PLAYER);
    if (!player) return;
    
    // --- PLAYER MOVEMENT ---
    float dx, dy;
    get_move_input(&dx, &dy);
    player->vel_x += dx * player->move_speed * dt;
    player->vel_y += dy * player->move_speed * dt;
    player->vel_x *= player->drag;
    player->vel_y *= player->drag;
    
    // --- LOOK AT MOUSE ---
    float mx, my;
    get_world_mouse_pos(state, &mx, &my);
    look_at(player, mx, my);
    
    // --- SPAWN BALL ON CLICK ---
    if (is_key_pressed(MOUSE_LEFT)) {
        Color colors[] = {COLOR_RED, COLOR_BLUE, COLOR_GREEN, COLOR_YELLOW};
        printf("Clicked");
        // Direction from player to mouse
        float dir_x = mx - player->x;
        float dir_y = my - player->y;
        float len = sqrtf(dir_x*dir_x + dir_y*dir_y) + 0.001f;
        dir_x /= len;
        dir_y /= len;
        
        // Spawn slightly ahead of player
        float spawn_x = player->x + dir_x * 60.0f;
        float spawn_y = player->y + dir_y * 60.0f;
        
        Entity *ball = spawn_ball(state, spawn_x, spawn_y, randf(10, 30), colors[rand() % 4]);
        ball->tag = TAG_BALL;
        ball->vel_x = dir_x * 400.0f;
        ball->vel_y = dir_y * 400.0f;
    }
    
    // --- DESTROY BALLS ON RIGHT CLICK ---
    if (is_key_pressed(MOUSE_RIGHT)) {
        Entity *balls[50];
        int count = find_all_with_tag(state, TAG_BALL, balls, 50);
        if (count > 0) {
            entity_destroy(balls[rand() % count]);  // Destroy random ball
        }
    }
    
    // --- UPDATE PLAYER LIGHT ---
    //lighting_update_point(player_light, player->x, player->y);
    
    // --- CAMERA FOLLOWS PLAYER (smooth) ---
    state->camera.x = lerpf(state->camera.x, player->x, 0.1f);
    state->camera.y = lerpf(state->camera.y, player->y, 0.1f);
    
    // --- ZOOM WITH SCROLL (simulated with keys) ---
    if (is_key_down(KEY_Q)) state->camera.zoom *= 1.02f;
    if (is_key_down(KEY_E)) state->camera.zoom *= 0.98f;
    state->camera.zoom = clampf(state->camera.zoom, 0.5f, 2.0f);
}

void render_world(GameState *state) {
    // Render tilemap BEFORE entities (this is called inside camera mode)
    tilemap_render_simple(test_map, tex_tile, 0, 0);
}

void render_game(GameState *state) {
    // This is for UI/HUD elements that appear on top of everything (screen space)
}

void close_game(GameState *state) {
    // Clean up tilemap
    tilemap_destroy(test_map);
    
    // Textures are cleaned up by resources_shutdown()
}