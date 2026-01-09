#include "physics.h"
#include "spatial.h"
#include "math_common.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Spatial index for broad-phase collision detection
static SpatialIndex* g_spatial = NULL;

// Buffer for spatial query results
#define MAX_QUERY_RESULTS 128
static Entity* query_buffer[MAX_QUERY_RESULTS];


// Set of colission checks for different shapes
// RECT vs RECT (AABB)
Manifold check_rect_rect(const Entity *a, const Entity *b) {
    Manifold m = {0};
    
    // Get Centers and Half-Widths
    float ax = a->x + a->collider.offset_x;
    float ay = a->y + a->collider.offset_y;
    float aw = a->collider.rect.width / 2.0f;
    float ah = a->collider.rect.height / 2.0f;
    
    float bx = b->x + b->collider.offset_x;
    float by = b->y + b->collider.offset_y;
    float bw = b->collider.rect.width / 2.0f;
    float bh = b->collider.rect.height / 2.0f;

    // Calculate overlap on X and Y
    float dx = bx - ax;
    float dy = by - ay;
    float overlap_x = (aw + bw) - fabsf(dx);
    float overlap_y = (ah + bh) - fabsf(dy);

    // If either overlap is <= 0, no collision
    if (overlap_x <= 0 || overlap_y <= 0) return m;

    m.hit = 1;

    // Find the "Path of Least Resistance"
    // We want to push out the shallowest way (X or Y?)
    if (overlap_x < overlap_y) {
        m.depth = overlap_x;
        m.normal_x = (dx < 0) ? -1.0f : 1.0f; // Point towards B
        m.normal_y = 0;
    } else {
        m.depth = overlap_y;
        m.normal_x = 0;
        m.normal_y = (dy < 0) ? -1.0f : 1.0f;
    }
    return m;
}

// CIRCLE vs CIRCLE
// This is a simple collision check for two circles
Manifold check_circle_circle(const Entity *a, const Entity *b) {
    Manifold m = {0}; // Default to no hit

    float ax = a->x + a->collider.offset_x;
    float ay = a->y + a->collider.offset_y;
    float bx = b->x + b->collider.offset_x;
    float by = b->y + b->collider.offset_y;

    float dx = bx - ax;
    float dy = by - ay;
    float dist_sq = dx*dx + dy*dy;
    float radius_sum = a->collider.circle.radius + b->collider.circle.radius;

    // Check overlap
    if (dist_sq >= radius_sum * radius_sum) return m; // No hit

    float distance = sqrtf(dist_sq);
    m.hit = 1;

    if (distance == 0.0f) {
        // Exact center overlap (rare but bad). Choose random up.
        m.depth = radius_sum;
        m.normal_x = 0;
        m.normal_y = -1;
    } else {
        m.depth = radius_sum - distance;
        // Normalize vector (dx/dist, dy/dist)
        m.normal_x = dx / distance; 
        m.normal_y = dy / distance;
    }

    return m;
}

// CIRCLE vs RECT
Manifold check_circle_rect(const Entity *a, const Entity *b) {
    Manifold m = {0};

    // Determine which is which
    // We assume 'a' is the Circle and 'b' is the Rect for calculation
    // If they came in swapped, we fix it later in the dispatcher
    const Entity *circ = (a->collider.type == SHAPE_CIRCLE) ? a : b;
    const Entity *rect = (a->collider.type == SHAPE_RECT)   ? a : b;

    // Get positions considering offsets
    float cx = circ->x + circ->collider.offset_x;
    float cy = circ->y + circ->collider.offset_y;
    float rx = rect->x + rect->collider.offset_x;
    float ry = rect->y + rect->collider.offset_y;
    float rw = rect->collider.rect.width / 2.0f;
    float rh = rect->collider.rect.height / 2.0f;

    // Clamp Circle Center to Rect Bounds (Find closest point on box)
    float closest_x = fmaxf(rx - rw, fminf(cx, rx + rw));
    float closest_y = fmaxf(ry - rh, fminf(cy, ry + rh));

    // Vector from Circle Center to Closest Point
    float dx = closest_x - cx;
    float dy = closest_y - cy;
    float dist_sq = dx*dx + dy*dy;
    float r = circ->collider.circle.radius;

    // No hit?
    if (dist_sq >= r * r) return m;

    // Create Manifold
    m.hit = 1;
    float distance = sqrtf(dist_sq);

    if (distance == 0.0f) {
        // Circle center is exactly inside the rect center (or closest point overlap)
        // Push out along Y by default
        m.depth = r;
        m.normal_x = 0;
        m.normal_y = -1; 
    } else {
        m.depth = r - distance;
        m.normal_x = dx / distance; // Normal points from Circle -> Rect
        m.normal_y = dy / distance;
    }

    // Ensure Normal points from A to B
    // If 'a' was the Rect, our calculation (Circle->Rect) is effectively (B->A).
    // We need to invert it so it matches the resolve_collision expectation (A->B).
    if (a == rect) {
        m.normal_x *= -1;
        m.normal_y *= -1;
    }

    return m;
}


// Check collision dispatch
Manifold check_collision_dispatch(const Entity *a, const Entity *b) {
    Manifold m = {0};
    int type_a = a->collider.type;
    int type_b = b->collider.type;

    if (type_a == SHAPE_CIRCLE && type_b == SHAPE_CIRCLE) 
        return check_circle_circle(a, b);
    
    if (type_a == SHAPE_RECT && type_b == SHAPE_RECT) 
        return check_rect_rect(a, b);
    
    if ((type_a == SHAPE_CIRCLE && type_b == SHAPE_RECT) || 
        (type_a == SHAPE_RECT && type_b == SHAPE_CIRCLE)) 
    {
        return check_circle_rect(a, b);
    }

    return m;
}

// Resolve all collisions

void resolve_collision(Entity *a, Entity *b, Manifold *m) {

    // SEPARATION (Stop them from overlapping)
    // We push them apart based on their Inverse Mass.
    // Heavy objects move less. Static objects (mass=0) don't move at all.
    float inv_mass_a = (a->mass == 0.0f) ? 0.0f : 1.0f / a->mass;
    float inv_mass_b = (b->mass == 0.0f) ? 0.0f : 1.0f / b->mass;
    float total_inv_mass = inv_mass_a + inv_mass_b;

    if (total_inv_mass == 0.0f) return; // Both are static

    // Calculate how much to move each
    float move_per_inv_mass = m->depth / total_inv_mass;
    
    // Move A (Negative normal because normal points A->B)
    a->x -= m->normal_x * move_per_inv_mass * inv_mass_a;
    a->y -= m->normal_y * move_per_inv_mass * inv_mass_a;
    
    // Move B
    b->x += m->normal_x * move_per_inv_mass * inv_mass_b;
    b->y += m->normal_y * move_per_inv_mass * inv_mass_b;


    // IMPULSE (Bounce)
    // Calculate relative velocity
    float rv_x = b->vel_x - a->vel_x;
    float rv_y = b->vel_y - a->vel_y;

    // Calculate velocity along the normal (Dot Product)
    float vel_along_normal = (rv_x * m->normal_x) + (rv_y * m->normal_y);

    // If velocities are separating, don't bounce (already moving apart)
    if (vel_along_normal > 0) return;

    // Calculate restitution (bounciness) - use the lower of the two
    float e = fminf(a->restitution, b->restitution);

    // Calculate impulse scalar
    float j = -(1.0f + e) * vel_along_normal;
    j /= total_inv_mass;

    // Apply impulse to velocities
    float impulse_x = m->normal_x * j;
    float impulse_y = m->normal_y * j;

    a->vel_x -= impulse_x * inv_mass_a;
    a->vel_y -= impulse_y * inv_mass_a;
    
    b->vel_x += impulse_x * inv_mass_b;
    b->vel_y += impulse_y * inv_mass_b;
}

// --- PHYSICS LIFECYCLE ---

void physics_init(float world_width, float world_height, float cell_size) {
    if (g_spatial) {
        spatial_destroy(g_spatial);
    }
    
    SpatialConfig config = {
        .type = SPATIAL_TYPE_GRID,
        .world_width = world_width,
        .world_height = world_height,
        .cell_size = cell_size
    };
    
    g_spatial = spatial_create(config);
    
    if (g_spatial) {
        SpatialStats stats = spatial_get_stats(g_spatial);
        printf("Physics: Spatial grid initialized (%d cells, %.0fx%.0f world, %.0f cell size)\n",
               stats.total_cells, world_width, world_height, cell_size);
    } else {
        printf("Physics: WARNING - Failed to create spatial index, using O(n^2) fallback\n");
    }
}

void physics_shutdown(void) {
    if (g_spatial) {
        spatial_destroy(g_spatial);
        g_spatial = NULL;
    }
}

// --- PHYSICS UPDATE ---

void physics_update(GameState *state, float dt) {
    // Apply velocity and drag to all dynamic entities
    for (int i = 0; i < state->count; i++) {
        Entity *e = &state->entities[i];
        if (!e->active || e->mass == 0.0f) continue;

        // Apply position update
        e->x += e->vel_x * dt;
        e->y += e->vel_y * dt;
        
        // Apply friction (linear slowdown toward zero)
        // This slows down objects that were pushed/bumped
        e->vel_x = move_towardf(e->vel_x, 0.0f, e->friction * dt);
        e->vel_y = move_towardf(e->vel_y, 0.0f, e->friction * dt);
    }

    // Reset collision flags
    for (int i = 0; i < state->count; i++) {
        state->entities[i].collider.is_colliding = 0;
    }

    // --- BROAD PHASE: Spatial Partitioning ---
    if (g_spatial) {
        // Clear and rebuild spatial index
        spatial_clear(g_spatial);
        for (int i = 0; i < state->count; i++) {
            Entity *e = &state->entities[i];
            if (e->active && e->collider.active) {
                spatial_insert(g_spatial, e);
            }
        }
        
        // Check collisions using spatial queries
        for (int i = 0; i < state->count; i++) {
            Entity *a = &state->entities[i];
            if (!a->active || !a->collider.active) continue;
            
            // Query for nearby entities
            int num_candidates = spatial_query(g_spatial, a, query_buffer, MAX_QUERY_RESULTS);
            
            for (int j = 0; j < num_candidates; j++) {
                Entity *b = query_buffer[j];
                
                // Skip if we've already checked this pair (a->id < b->id ensures each pair checked once)
                if (a->id >= b->id) continue;
                
                // Layer Check
                if (!((a->collider.mask & b->collider.layer) || (b->collider.mask & a->collider.layer))) continue;
                
                // Narrow Phase: Actual collision check
                Manifold m = check_collision_dispatch(a, b);
                
                if (m.hit) {
                    a->collider.is_colliding = 1;
                    b->collider.is_colliding = 1;
                    resolve_collision(a, b, &m);
                }
            }
        }
    } 
    else {
        // Fallback: O(n^2) brute force (if spatial index failed to initialize)
        for (int i = 0; i < state->count; i++) {
            Entity *a = &state->entities[i];
            if (!a->active) continue; 

            for (int j = i + 1; j < state->count; j++) {
                Entity *b = &state->entities[j];
                if (!b->active) continue;

                // Layer Check
                if (!a->collider.active || !b->collider.active) continue;
                if (!((a->collider.mask & b->collider.layer) || (b->collider.mask & a->collider.layer))) continue;

                // Dispatch
                Manifold m = check_collision_dispatch(a, b);

                // Resolve
                if (m.hit) {
                    a->collider.is_colliding = 1;
                    b->collider.is_colliding = 1;
                    resolve_collision(a, b, &m);
                }
            }
        }
    }
}