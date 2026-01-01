#ifndef LIGHTING_H
#define LIGHTING_H

#include "engine.h"

#define MAX_POINT_LIGHTS 16

typedef struct {
    float x, y;           // World position
    float radius;         // Light falloff radius
    Color color;          // Light color (RGB, alpha ignored)
    float intensity;      // Brightness multiplier (0.0 - 2.0+)
    int active;           // Is this light slot in use?
} PointLight;

// Initialize the lighting system
void lighting_init(void);

// Master toggle for lighting (1 = enabled, 0 = disabled)
void lighting_enable(int enabled);
int lighting_is_enabled(void);

// Set the ambient light color (the "darkness" color when no lights)
void lighting_set_ambient(Color color);

// Add a point light, returns light ID (or -1 if full)
int lighting_add_point(float x, float y, float radius, Color color, float intensity);

// Update a light's position (useful for lights that follow entities)
void lighting_update_point(int light_id, float x, float y);

// Update a light's properties
void lighting_set_point(int light_id, float x, float y, float radius, Color color, float intensity);

// Remove a specific light
void lighting_remove_point(int light_id);

// Clear all lights (call at start of frame if lights are dynamic)
void lighting_clear_all(void);

// Get current light count
int lighting_get_count(void);

// Apply lighting uniforms to shader (called internally by renderer)
void lighting_apply(void);

#endif
