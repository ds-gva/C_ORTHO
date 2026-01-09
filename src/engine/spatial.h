// spatial.h — Spatial partitioning for broad-phase collision detection
// 
// This module provides an abstract interface for spatial acceleration structures.
// Currently implements: Uniform Grid
// Future options: Spatial Hash, Quadtree, etc.
//
#ifndef SPATIAL_H
#define SPATIAL_H

#include "engine.h"

// Maximum entities that can be in a single cell
// If exceeded, collisions may be missed (increase if needed)
#define SPATIAL_MAX_PER_CELL 64

// Opaque spatial index handle
typedef struct SpatialIndex SpatialIndex;

// --- CONFIGURATION ---

typedef enum {
    SPATIAL_TYPE_GRID,      // Uniform grid (fixed bounds, fast)
    // SPATIAL_TYPE_HASH,   // Spatial hash (infinite bounds, future)
    // SPATIAL_TYPE_QUADTREE // Quadtree (varying sizes, future)
} SpatialType;

typedef struct {
    SpatialType type;
    
    // Grid-specific settings
    float world_width;      // Total world width
    float world_height;     // Total world height
    float cell_size;        // Size of each cell (should be >= largest entity)
    
    // Future: hash table size, quadtree depth, etc.
} SpatialConfig;

// --- CORE API ---

// Create a spatial index with the given configuration
// Returns NULL on failure
SpatialIndex* spatial_create(SpatialConfig config);

// Destroy a spatial index and free memory
void spatial_destroy(SpatialIndex* index);

// Clear all entities from the index (call at start of each physics frame)
void spatial_clear(SpatialIndex* index);

// Insert an entity into the spatial index
// The entity's position and collider size determine which cell(s) it occupies
void spatial_insert(SpatialIndex* index, Entity* entity);

// Query for entities that might collide with the given entity
// Returns the number of candidates found
// Candidates are written to the 'out_candidates' array (up to max_candidates)
// NOTE: This may include the entity itself - caller should filter it out
int spatial_query(SpatialIndex* index, Entity* entity, 
                  Entity** out_candidates, int max_candidates);

// --- STATS (for debugging/profiling) ---

typedef struct {
    int total_cells;        // Number of cells in the structure
    int occupied_cells;     // Cells with at least one entity
    int total_entities;     // Total entities inserted
    int max_per_cell;       // Most entities in any single cell
    float avg_per_cell;     // Average entities per occupied cell
} SpatialStats;

// Get statistics about the current state of the index
SpatialStats spatial_get_stats(SpatialIndex* index);

#endif
