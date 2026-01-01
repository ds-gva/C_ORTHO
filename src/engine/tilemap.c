// tilemap.c — Simple tilemap system for 2D top-down games

#include "tilemap.h"
#include <stdlib.h>
#include <stdio.h>

// ============================================================================
// TILESET
// ============================================================================

Tileset* tileset_create(Texture* texture, int tile_width, int tile_height) {
    if (!texture) {
        printf("ERROR: tileset_create called with NULL texture\n");
        return NULL;
    }
    
    Tileset* ts = (Tileset*)malloc(sizeof(Tileset));
    if (!ts) return NULL;
    
    ts->texture = texture;
    ts->tile_width = tile_width;
    ts->tile_height = tile_height;
    ts->tiles_per_row = texture->width / tile_width;
    ts->tile_count = ts->tiles_per_row * (texture->height / tile_height);
    
    return ts;
}

void tileset_destroy(Tileset* ts) {
    if (ts) {
        // Note: We don't free the texture here - resources module owns it
        free(ts);
    }
}

// ============================================================================
// TILEMAP
// ============================================================================

Tilemap* tilemap_create(int width, int height, int tile_width, int tile_height) {
    Tilemap* map = (Tilemap*)malloc(sizeof(Tilemap));
    if (!map) return NULL;
    
    map->width = width;
    map->height = height;
    map->tile_width = tile_width;
    map->tile_height = tile_height;
    map->tileset = NULL;
    
    // Allocate grid (all -1 = empty)
    map->tiles = (int*)malloc(width * height * sizeof(int));
    if (!map->tiles) {
        free(map);
        return NULL;
    }
    
    // Initialize all tiles to empty
    for (int i = 0; i < width * height; i++) {
        map->tiles[i] = -1;
    }
    
    return map;
}

void tilemap_destroy(Tilemap* map) {
    if (map) {
        if (map->tiles) {
            free(map->tiles);
        }
        free(map);
    }
}

void tilemap_set_tileset(Tilemap* map, Tileset* ts) {
    if (map) {
        map->tileset = ts;
    }
}

void tilemap_set_tile(Tilemap* map, int x, int y, int tile_id) {
    if (!map) return;
    if (x < 0 || x >= map->width || y < 0 || y >= map->height) return;
    
    map->tiles[y * map->width + x] = tile_id;
}

int tilemap_get_tile(Tilemap* map, int x, int y) {
    if (!map) return -1;
    if (x < 0 || x >= map->width || y < 0 || y >= map->height) return -1;
    
    return map->tiles[y * map->width + x];
}

void tilemap_fill(Tilemap* map, int tile_id) {
    if (!map || !map->tiles) return;
    
    for (int i = 0; i < map->width * map->height; i++) {
        map->tiles[i] = tile_id;
    }
}

// ============================================================================
// RENDERING
// ============================================================================

// Simple version: one texture repeated across the grid
// Perfect for testing or single-texture floors/backgrounds
void tilemap_render_simple(Tilemap* map, Texture* tex, float offset_x, float offset_y) {
    if (!map || !tex) return;
    
    float tw = (float)map->tile_width;
    float th = (float)map->tile_height;
    
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            int tile_id = map->tiles[y * map->width + x];
            if (tile_id < 0) continue;  // Skip empty tiles
            
            // Calculate center position (draw_texture uses center coords)
            float px = offset_x + x * tw + tw / 2.0f;
            float py = offset_y + y * th + th / 2.0f;
            
            draw_texture(*tex, px, py, tw, th, 0, COLOR_WHITE);
        }
    }
}

// Full tileset version: uses UV coords to pick tiles from a spritesheet
// TODO: This requires a draw_texture_region function in the renderer
// For now, this is a placeholder that falls back to simple render
void tilemap_render(Tilemap* map, float offset_x, float offset_y) {
    if (!map) return;
    
    // If no tileset, can't render
    if (!map->tileset || !map->tileset->texture) {
        return;
    }
    
    // For now, use the tileset texture but render the whole thing per tile
    // This is a simplified version - proper UV-based rendering comes later
    tilemap_render_simple(map, map->tileset->texture, offset_x, offset_y);
}
