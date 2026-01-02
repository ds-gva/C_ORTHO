#ifndef LIGHTING_H
#define LIGHTING_H

#include "engine.h"

#define MAX_POINT_LIGHTS 16

// --- DIRECTIONAL LIGHT (Sun) ---
// Controls scene lighting and shadow direction
typedef struct {
    float angle;          // Sun direction: 0-360° (0=North, 90=East, 180=South, 270=West)
    Color color;          // Light color tint
    float intensity;      // Brightness (0.0 - 2.0+)
    int orthogonal;       // 1 = sun directly overhead (no shadows), 0 = angled light
} DirectionalLight;

// --- POINT LIGHT ---
typedef struct {
    float x, y;           // World position
    float radius;         // Light falloff radius
    Color color;          // Light color (RGB, alpha ignored)
    float intensity;      // Brightness multiplier (0.0 - 2.0+)
    int active;           // Is this light slot in use?
} PointLight;

// Initialize the lighting system
void init_lighting(void);

// Master toggle for lighting (1 = enabled, 0 = disabled)
void lighting_enable(int enabled);
int lighting_is_enabled(void);

// Adaptive point lights: scale intensity based on ambient brightness
// When enabled, point lights contribute less during bright daylight (more realistic)
void lighting_set_adaptive(int enabled);
int lighting_is_adaptive(void);

// --- DIRECTIONAL LIGHT API ---
void lighting_set_directional(float angle, Color color, float intensity);
void lighting_set_sun_angle(float angle);             // Quick angle update
void lighting_set_orthogonal(int orthogonal);         // 1 = overhead (no shadows), 0 = angled
int lighting_is_orthogonal(void);                     // Check if shadows are disabled
float lighting_get_sun_angle(void);                   // Get current sun angle in degrees
DirectionalLight lighting_get_directional(void);

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

// Calculate shadow opacity reduction at a world position (0.0 = full shadow, 1.0 = no shadow)
// Used to fade shadows when they're in lit areas
float lighting_get_shadow_fade(float world_x, float world_y);

#endif
