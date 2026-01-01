# C_ORTHO2D

A lightweight top-down 2D game engine written in pure C with OpenGL. Mainly as a learning project

## Features

- **Batch Renderer** — Modern OpenGL (4.6) with automatic batching
- **Entity System** — Spawn, tag, and manage game objects
- **Basic Physics** — AABB & circle collision with impulse resolution
- **Tilemaps** — (very) basic Grid-based level rendering
- **Lighting** — Dynamic 2D point lights with ambient control
- **Depth Sorting** — Layer → Z-order → Y-position sorting
- **Input** — Abstracted keyboard/mouse with press/release detection
- **Resources** — Texture caching with reference counting ; only PNG support for now

## Structure

```
src/
├── engine/
│   ├── engine.h          # Core types (Entity, Color, Camera, GameState)
│   ├── engine_core.c     # Main update/render loop, Y-sorting
│   ├── renderer_opengl.c # Batch renderer, shaders, drawing
│   ├── entity.c          # Entity spawning and queries
│   ├── physics.c         # Collision detection and resolution
│   ├── input.c           # Keyboard/mouse abstraction
│   ├── tilemap.c         # Tilemap creation and rendering
│   ├── lighting.c        # Point light system
│   └── resources.c       # Texture loading and caching
├── game/
│   └── game.c            # Your game logic (init, update, render)
└── platform/
    └── platform_glfw.c   # Window, input polling, main loop

shaders/
├── basic.vert            # Vertex shader
└── basic.frag            # Fragment shader (lighting calc)
```

## Quick Start

```c
// game.c
void init_game(GameState *state) {
    Texture* tex = resource_load_texture("assets/player.png");
    Entity* player = spawn_sprite(state, tex, 400, 300);
    player->tag = TAG_PLAYER;
}

void update_game(GameState *state, float dt) {
    Entity* player = find_entity_with_tag(state, TAG_PLAYER);
    float dx, dy;
    get_move_input(&dx, &dy);
    player->vel_x = dx * 200.0f;
    player->vel_y = dy * 200.0f;
}
```

## Build (Windows/MSVC)

```
cl.exe /Zi /EHsc /MD src/**/*.c third_party/glad/glad.c /I include /Fe:game.exe
```

## Dependencies

- **GLFW** — Windowing
- **glad** — OpenGL loader
- **stb_image** — PNG Texture loading

## License

MIT
