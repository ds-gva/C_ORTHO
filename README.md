# C_ORTHO2D

A lightweight top-down 2D game engine written in pure C with OpenGL. Mainly as a learning project.

## Features

- **Batch Renderer** — Modern OpenGL (3.3+) with automatic batching
- **Entity System** — Spawn, tag, and manage game objects
- **Physics** — AABB & circle collision with impulse resolution, Godot-style movement (max_speed, acceleration, friction)
- **Tilemaps** — Basic grid-based level rendering
- **Lighting** — Ambient, directional (sun), and dynamic point lights with smooth falloff
- **Shadows** — Blob/sprite shape shadows with directional offset based on sun angle
- **Depth Sorting** — Layer → Z-order → Y-position sorting
- **Input** — Abstracted keyboard/mouse with press/release detection
- **Resources** — Texture caching with reference counting (PNG only)
- **Sandbox Helpers** — Movement modes (8-dir, 4-dir, tank, strafe, click-to-move), camera follow, time-of-day system

## Structure

```
src/
├── engine/
│   ├── engine.h          # Core types (Entity, Color, Camera, GameState)
│   ├── engine_core.c     # Main update/render loop, Y-sorting, shadow pass
│   ├── renderer_opengl.c # Batch renderer, shaders, drawing primitives
│   ├── entity.c/.h       # Entity spawning and queries
│   ├── physics.c/.h      # Collision detection, resolution, friction
│   ├── input.c/.h        # Keyboard/mouse abstraction
│   ├── tilemap.c/.h      # Tilemap creation and rendering
│   ├── lighting.c/.h     # Ambient, directional, and point light system
│   ├── resources.c/.h    # Texture loading and caching
│   ├── sandbox.c         # Movement controllers, camera helpers, time-of-day
│   ├── math_common.h     # Math utilities (clamp, lerp, move_toward)
│   └── utils.c/.h        # File loading, random numbers
├── game/
│   ├── game.c            # Your game logic (init, update, render)
│   ├── sandbox.c/.h      # Game-specific sandbox helpers (can override engine's)
└── platform/
    └── platform_glfw.c   # Window creation, input polling, main loop

shaders/
├── basic.vert            # Vertex shader (transform, pass world pos)
└── basic.frag            # Fragment shader (shapes, lighting, point lights)

include/                  # Third-party headers (GLFW, glad, stb_image)
third_party/              # Third-party source (glad.c)
```

## Quick Start

```c
// game.c
#include "sandbox.h"

void init_game(GameState *state) {
    Texture* tex = resource_load_texture("assets/player.png");
    Entity* player = spawn_sprite(state, tex, 400, 300);
    player->tag = TAG_PLAYER;
    player->max_speed = 200.0f;      // pixels/second
    player->acceleration = 800.0f;   // how fast to reach max speed
    player->friction = 600.0f;       // how fast to stop
    
    // Setup lighting
    lighting_set_ambient((Color){0.1f, 0.1f, 0.1f, 1.0f});
    lighting_add_point(100, 100, 150.0f, (Color){1.0f, 0.5f, 0.2f, 1.0f}, 1.0f);
}

void update_game(GameState *state, float dt) {
    Entity* player = find_entity_with_tag(state, TAG_PLAYER);
    
    // Use sandbox helper for movement (8-dir, 4-dir, tank, strafe, click-to-move)
    movement_apply(player, state, MOVE_MODE_8DIR, dt);
    
    // Camera follows player
    camera_follow_smooth(state, player->x, player->y, 0.9f);
}
```

## Build (Windows/MSVC)

Uses VSCode tasks (see `.vscode/tasks.json`) or manually:

```
cl.exe /Zi /EHsc /MD src/engine/*.c src/game/*.c src/platform/*.c third_party/glad/glad.c /I include /Fe:game.exe /link glfw3.lib opengl32.lib
```

## Dependencies

- **GLFW** — Windowing
- **glad** — OpenGL loader
- **stb_image** — PNG Texture loading

## License

MIT
