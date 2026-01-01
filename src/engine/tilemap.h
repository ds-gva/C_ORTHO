#ifndef TILEMAP_H
#define TILEMAP_H

#include "engine.h"

// A tileset is a texture divided into a grid of tiles
typedef struct {
    Texture* texture;       // The tileset image
    int tile_width;         // Width of one tile in pixels (in the texture)
    int tile_height;        // Height of one tile in pixels (in the texture)
    int tiles_per_row;      // How many tiles across the texture
    int tile_count;         // Total number of tiles
} Tileset;

// A tilemap is a grid of tile IDs that reference a tileset
typedef struct {
    int* tiles;             // 1D array of tile IDs (-1 = empty, 0+ = tile index)
    int width;              // Map width in tiles
    int height;             // Map height in tiles
    int tile_width;         // Rendered tile width in world units
    int tile_height;        // Rendered tile height in world units
    Tileset* tileset;       // Which tileset to use
} Tilemap;

// Tileset API
Tileset* tileset_create(Texture* texture, int tile_width, int tile_height);
void tileset_destroy(Tileset* ts);

// Tilemap API
Tilemap* tilemap_create(int width, int height, int tile_width, int tile_height);
void tilemap_destroy(Tilemap* map);
void tilemap_set_tileset(Tilemap* map, Tileset* ts);
void tilemap_set_tile(Tilemap* map, int x, int y, int tile_id);
int tilemap_get_tile(Tilemap* map, int x, int y);
void tilemap_fill(Tilemap* map, int tile_id);

// Rendering
void tilemap_render(Tilemap* map, float offset_x, float offset_y);

// Simple render (single texture tiled, no tileset needed)
void tilemap_render_simple(Tilemap* map, Texture* tex, float offset_x, float offset_y);

#endif
