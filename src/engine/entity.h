#ifndef ENTITY_H
#define ENTITY_H

#include "engine.h"

// Core Allocator
Entity* entity_alloc(GameState *state);
void entity_destroy(Entity *e);

// Generic Spawners
Entity* spawn_sprite(GameState *state, Texture *tex, float x, float y);
Entity* spawn_primitive_wall(GameState *state, float x, float y, float w, float h);
Entity* spawn_ball(GameState *state, float x, float y, float radius, Color color);
void spawn_world_bounds(GameState *state, float width, float height);

// Finders
Entity* get_entity_by_id(GameState *state, uint32_t id);
Entity* find_entity_with_tag(GameState *state, uint32_t tag);
int find_all_with_tag(GameState *state, uint32_t tag, Entity **out, int max);

#endif