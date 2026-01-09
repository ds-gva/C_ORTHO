#ifndef PHYSICS_H
#define PHYSICS_H

#include "engine.h" // We need the Entity struct definition

// Putting all the collision data ("Manifold")in a struct
typedef struct {
    int hit;
    float normal_x;
    float normal_y;
    float depth;
} Manifold;


// Checks specific shapes and returns collision data
Manifold check_circle_circle(const Entity *a, const Entity *b);
Manifold check_rect_rect(const Entity *a, const Entity *b);
Manifold check_circle_rect(const Entity *a, const Entity *b);

// Main dispatcher that figures out shapes automatically
Manifold check_collision_dispatch(const Entity *a, const Entity *b);

// Physics Responses
void resolve_collision(Entity *a, Entity *b, Manifold *m);

// Physics System Lifecycle
// Call physics_init AFTER setting up your world bounds
void physics_init(float world_width, float world_height, float cell_size);
void physics_shutdown(void);

// Physics Update
void physics_update(GameState *state, float dt);

#endif