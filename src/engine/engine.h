#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

extern int g_screen_width;
extern int g_screen_height;
extern int g_debug_draw;
extern int g_y_sort_enabled;   // Toggle Y-sorting (1 = on, 0 = off)
extern int g_shadows_enabled;  // Toggle blob shadows (1 = on, 0 = off)

typedef struct {
    float r, g, b, a;
} Color;

#define COLOR_WHITE   ((Color){1.0f, 1.0f, 1.0f, 1.0f})
#define COLOR_BLACK   ((Color){0.0f, 0.0f, 0.0f, 1.0f})
#define COLOR_RED     ((Color){1.0f, 0.0f, 0.0f, 1.0f})
#define COLOR_GREEN   ((Color){0.0f, 1.0f, 0.0f, 1.0f})
#define COLOR_BLUE    ((Color){0.0f, 0.0f, 1.0f, 1.0f})
#define COLOR_YELLOW  ((Color){1.0f, 1.0f, 0.0f, 1.0f})
#define COLOR_GRAY    ((Color){0.5f, 0.5f, 0.5f, 1.0f})

typedef enum {
    SHAPE_RECT,
    SHAPE_CIRCLE,
    VISUAL_SPRITE

} ShapeType;

typedef enum {
    LAYER_NONE   = 0,
    LAYER_PLAYER = 1 << 0, // 1
    LAYER_ENEMY  = 1 << 1, // 2
    LAYER_WALL   = 1 << 2, // 4
} CollisionLayer;


typedef struct {
    unsigned int id;  // OpenGL ID
    int width;
    int height;
} Texture;

// Sorting layers for Y-sorting (lower = drawn first/behind)
typedef enum {
    SORT_LAYER_BACKGROUND = 0,   // Floor decals, shadows
    SORT_LAYER_GROUND = 1,       // Ground-level objects
    SORT_LAYER_DEFAULT = 2,      // Normal entities (players, enemies, items)
    SORT_LAYER_OVERHEAD = 3,     // Tree tops, roofs (always on top)
} SortLayer;

// ENTITIY
typedef struct {
    uint32_t id; // unique identifier
    uint32_t tag; // who am I?
    int active; // 1 = alive, 0 = dead/recyclable (for recycling entities)

    float x, y; // position
    float rotation; // degrees
    float scale;
    
    // DEPTH SORTING
    int sort_layer;         // Coarse layer (SORT_LAYER_DEFAULT, etc.) - sorted first
    int z_order;            // Fine control within layer (higher = in front) - sorted second
    float sort_offset_y;    // Added to Y for Y-sorting (feet vs center) - sorted third

    // PHYSICS
    float vel_x, vel_y;   // Current velocity (px/s)
    float mass;           // Only for collisions (0 = static/immovable)
    float restitution;    // Bounciness on collision (0-1)
    
    // MOVEMENT
    float max_speed;      // Target max velocity (px/s), e.g. 200
    float acceleration;   // How fast to reach max_speed (px/s²), e.g. 800
    float friction;       // How fast to stop (px/s²), e.g. 600
    
    // --- SHADOW ---
    int casts_shadow;       // 1 = draw drop shadow under this entity
    float shadow_offset;    // Distance from entity center (in pixels) - direction from sun angle
    float shadow_scale;     // Size relative to entity (0.8 = 80% of entity size)
    float shadow_opacity;   // Darkness (0.0 - 1.0)

    // --- VISUALS ---
    Color color;
    ShapeType visual_type; // RECT, CIRCLE, or TRIANGLE
    union {
        struct { float width, height; } rect;
        struct { float radius; } circle;
        struct { Texture *texture; float width, height; } sprite;
    } visual; // Named this union "visual" to be explicit

    struct {
        int active;     // 1 = enabled, 0 = no collision
        ShapeType type; // Usually only RECT or CIRCLE for physics
        
        // Offset allows the hitbox to be somewhere else (e.g., just the feet)
        float offset_x, offset_y; 

        // Collision Size (Different from Visual Size!)
        union {
            struct { float width, height; } rect;
            struct { float radius; } circle;
        };

        // Rules
        uint32_t layer; // Who am I?
        uint32_t mask;  // Who do I hit?
        int is_colliding; // Debug flag (Flash red)

    } collider;

} Entity;

#define MAX_ENTITIES 10000


typedef struct {
    float x, y;
    float zoom;
} Camera;

typedef struct {
    Entity entities[MAX_ENTITIES];
    int count;
    uint32_t next_id; // next unique identifier ; increment after each spawn
    Camera camera;
    Color background;
} GameState;

// RENDERER API
void init_renderer();

void set_texture_filter_mode(int mode);

void clear_screen();
void clear_game_area(Color color);
void enable_scissor_test();

void draw_rect(float x, float y, float w, float h, float rotation, Color color, int hollow);
void draw_circle(float x, float y, float radius, float rotation, Color color, int hollow);
void draw_texture(Texture texture, float x, float y, float w, float h, float rotation, Color tint);
void flush_batch();

// Camera
void set_camera(float x, float y, float zoom);
void begin_camera_mode();
void end_camera_mode();

// ENGINE CORE API (automatic systems)
void engine_update(GameState *state, float dt);
void engine_render(GameState *state);

// GAME API
void init_game(GameState *state);
void update_game(GameState *state, float dt);
void render_world(GameState *state);  // Called inside camera mode, BEFORE entities (for tilemaps, backgrounds)
void render_game(GameState *state);   // Called after camera mode (for UI/HUD)
void close_game(GameState *state);


// Abstract key codes (platform-independent)
typedef enum {
    // Arrows
    KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
    
    // Letters
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H,
    KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P,
    KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X,
    KEY_Y, KEY_Z,
    
    // Numbers
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    
    // Special
    KEY_SPACE, KEY_ESCAPE, KEY_ENTER, KEY_SHIFT, KEY_CTRL,
    KEY_TAB, KEY_BACKSPACE,
    
    // Function keys
    KEY_F1, KEY_F2, KEY_F3, KEY_F4,
    
    // Mouse
    MOUSE_LEFT, MOUSE_RIGHT,
    
    KEY_COUNT  // Always last!
} EngineKey;

// Input API (user-facing)
int is_key_down(EngineKey key);
int is_key_pressed(EngineKey key);   // Just pressed this frame
int is_key_released(EngineKey key);  // Just released this frame

void get_move_input(float *out_x, float *out_y);  // WASD + Arrows combined
void get_mouse_pos(float *out_x, float *out_y);
void get_world_mouse_pos(GameState *state, float *out_x, float *out_y);

// UTILS API
char* load_file_text(const char* path);

#endif