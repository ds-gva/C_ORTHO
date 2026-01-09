// spatial.c — Spatial partitioning implementation
// 
// Currently implements: Uniform Grid
// Designed for easy addition of other backends (hash, quadtree, etc.)
//

#include "spatial.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// --- UNIFORM GRID IMPLEMENTATION ---

typedef struct {
    Entity* entities[SPATIAL_MAX_PER_CELL];
    int count;
} GridCell;

typedef struct {
    GridCell* cells;        // Flat array of cells
    int cols;               // Number of columns (X)
    int rows;               // Number of rows (Y)
    float cell_size;
    float world_width;
    float world_height;
    int total_entities;     // Stats tracking
} UniformGrid;

// The actual SpatialIndex structure (opaque to user)
struct SpatialIndex {
    SpatialType type;
    union {
        UniformGrid grid;
        // Future: SpatialHash hash;
        // Future: Quadtree tree;
    } data;
};

// --- GRID HELPERS ---

static inline int grid_get_cell_x(UniformGrid* grid, float x) {
    int cx = (int)(x / grid->cell_size);
    if (cx < 0) cx = 0;
    if (cx >= grid->cols) cx = grid->cols - 1;
    return cx;
}

static inline int grid_get_cell_y(UniformGrid* grid, float y) {
    int cy = (int)(y / grid->cell_size);
    if (cy < 0) cy = 0;
    if (cy >= grid->rows) cy = grid->rows - 1;
    return cy;
}

static inline int grid_cell_index(UniformGrid* grid, int cx, int cy) {
    return cy * grid->cols + cx;
}

static void grid_insert_into_cell(UniformGrid* grid, int cx, int cy, Entity* entity) {
    if (cx < 0 || cx >= grid->cols || cy < 0 || cy >= grid->rows) return;
    
    int idx = grid_cell_index(grid, cx, cy);
    GridCell* cell = &grid->cells[idx];
    
    if (cell->count < SPATIAL_MAX_PER_CELL) {
        cell->entities[cell->count++] = entity;
    }
    // If cell is full, entity won't be added (increase SPATIAL_MAX_PER_CELL if this happens)
}

// Get the AABB of an entity's collider
static void get_entity_bounds(Entity* e, float* out_min_x, float* out_min_y, 
                               float* out_max_x, float* out_max_y) {
    float cx = e->x + e->collider.offset_x;
    float cy = e->y + e->collider.offset_y;
    
    if (e->collider.type == SHAPE_CIRCLE) {
        float r = e->collider.circle.radius;
        *out_min_x = cx - r;
        *out_min_y = cy - r;
        *out_max_x = cx + r;
        *out_max_y = cy + r;
    } else { // SHAPE_RECT
        float hw = e->collider.rect.width / 2.0f;
        float hh = e->collider.rect.height / 2.0f;
        *out_min_x = cx - hw;
        *out_min_y = cy - hh;
        *out_max_x = cx + hw;
        *out_max_y = cy + hh;
    }
}

// --- PUBLIC API IMPLEMENTATION ---

SpatialIndex* spatial_create(SpatialConfig config) {
    SpatialIndex* index = calloc(1, sizeof(SpatialIndex));
    if (!index) return NULL;
    
    index->type = config.type;
    
    switch (config.type) {
        case SPATIAL_TYPE_GRID: {
            UniformGrid* grid = &index->data.grid;
            
            grid->cell_size = config.cell_size;
            grid->world_width = config.world_width;
            grid->world_height = config.world_height;
            
            // Calculate grid dimensions
            grid->cols = (int)ceilf(config.world_width / config.cell_size);
            grid->rows = (int)ceilf(config.world_height / config.cell_size);
            
            // Allocate cells
            int total_cells = grid->cols * grid->rows;
            grid->cells = calloc(total_cells, sizeof(GridCell));
            if (!grid->cells) {
                free(index);
                return NULL;
            }
            break;
        }
        
        // Future backends would be initialized here
        default:
            free(index);
            return NULL;
    }
    
    return index;
}

void spatial_destroy(SpatialIndex* index) {
    if (!index) return;
    
    switch (index->type) {
        case SPATIAL_TYPE_GRID:
            free(index->data.grid.cells);
            break;
        // Future cleanup here
    }
    
    free(index);
}

void spatial_clear(SpatialIndex* index) {
    if (!index) return;
    
    switch (index->type) {
        case SPATIAL_TYPE_GRID: {
            UniformGrid* grid = &index->data.grid;
            int total_cells = grid->cols * grid->rows;
            
            // Reset all cell counts (fast - just zero the counts)
            for (int i = 0; i < total_cells; i++) {
                grid->cells[i].count = 0;
            }
            grid->total_entities = 0;
            break;
        }
    }
}

void spatial_insert(SpatialIndex* index, Entity* entity) {
    if (!index || !entity) return;
    if (!entity->active || !entity->collider.active) return;
    
    switch (index->type) {
        case SPATIAL_TYPE_GRID: {
            UniformGrid* grid = &index->data.grid;
            
            // Get entity bounds
            float min_x, min_y, max_x, max_y;
            get_entity_bounds(entity, &min_x, &min_y, &max_x, &max_y);
            
            // Find which cells this entity overlaps
            int start_cx = grid_get_cell_x(grid, min_x);
            int start_cy = grid_get_cell_y(grid, min_y);
            int end_cx = grid_get_cell_x(grid, max_x);
            int end_cy = grid_get_cell_y(grid, max_y);
            
            // Insert into all overlapped cells
            for (int cy = start_cy; cy <= end_cy; cy++) {
                for (int cx = start_cx; cx <= end_cx; cx++) {
                    grid_insert_into_cell(grid, cx, cy, entity);
                }
            }
            
            grid->total_entities++;
            break;
        }
    }
}

int spatial_query(SpatialIndex* index, Entity* entity,
                  Entity** out_candidates, int max_candidates) {
    if (!index || !entity || !out_candidates) return 0;
    
    int count = 0;
    
    switch (index->type) {
        case SPATIAL_TYPE_GRID: {
            UniformGrid* grid = &index->data.grid;
            
            // Get entity bounds
            float min_x, min_y, max_x, max_y;
            get_entity_bounds(entity, &min_x, &min_y, &max_x, &max_y);
            
            // Find which cells to check
            int start_cx = grid_get_cell_x(grid, min_x);
            int start_cy = grid_get_cell_y(grid, min_y);
            int end_cx = grid_get_cell_x(grid, max_x);
            int end_cy = grid_get_cell_y(grid, max_y);
            
            // Collect all entities from overlapped cells
            for (int cy = start_cy; cy <= end_cy && count < max_candidates; cy++) {
                for (int cx = start_cx; cx <= end_cx && count < max_candidates; cx++) {
                    int idx = grid_cell_index(grid, cx, cy);
                    GridCell* cell = &grid->cells[idx];
                    
                    for (int i = 0; i < cell->count && count < max_candidates; i++) {
                        Entity* candidate = cell->entities[i];
                        
                        // Skip self
                        if (candidate == entity) continue;
                        
                        // Check if already in output (avoid duplicates from multi-cell entities)
                        int duplicate = 0;
                        for (int j = 0; j < count; j++) {
                            if (out_candidates[j] == candidate) {
                                duplicate = 1;
                                break;
                            }
                        }
                        
                        if (!duplicate) {
                            out_candidates[count++] = candidate;
                        }
                    }
                }
            }
            break;
        }
    }
    
    return count;
}

SpatialStats spatial_get_stats(SpatialIndex* index) {
    SpatialStats stats = {0};
    if (!index) return stats;
    
    switch (index->type) {
        case SPATIAL_TYPE_GRID: {
            UniformGrid* grid = &index->data.grid;
            
            stats.total_cells = grid->cols * grid->rows;
            stats.total_entities = grid->total_entities;
            
            int total_in_occupied = 0;
            for (int i = 0; i < stats.total_cells; i++) {
                int c = grid->cells[i].count;
                if (c > 0) {
                    stats.occupied_cells++;
                    total_in_occupied += c;
                    if (c > stats.max_per_cell) {
                        stats.max_per_cell = c;
                    }
                }
            }
            
            if (stats.occupied_cells > 0) {
                stats.avg_per_cell = (float)total_in_occupied / (float)stats.occupied_cells;
            }
            break;
        }
    }
    
    return stats;
}
