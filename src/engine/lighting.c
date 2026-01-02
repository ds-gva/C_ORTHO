// lighting.c — 2D lighting system with directional (sun) and point lights

#include "lighting.h"
#include "math_common.h"
#include <glad/glad.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// External reference to shader program from renderer
extern GLuint shader_program;

// Internal lighting state
typedef struct {
    DirectionalLight directional;  // Sun/global light
    PointLight lights[MAX_POINT_LIGHTS];
    int count;
    Color ambient;
    int enabled;
    int adaptive;  // Scale point light intensity based on ambient brightness
} LightingState;

static LightingState g_lighting = {0};

void init_lighting(void) {
    memset(&g_lighting, 0, sizeof(LightingState));
    g_lighting.enabled = 1;
    g_lighting.count = 0;
    g_lighting.adaptive = 1;  // Adaptive point lights ON by default
    
    // Ambient = base darkness (the "night" level without any lights)
    g_lighting.ambient = (Color){0.08f, 0.08f, 0.12f, 1.0f};  // Dark with cool tint
    
    // Default sun: subtle fill light so point lights are the stars
    // Keep low so colored lights (torches, campfires) show their color
    g_lighting.directional.angle = 180.0f;      // Sun from South
    g_lighting.directional.color = (Color){0.9f, 0.85f, 0.8f, 1.0f};   // Slightly warm
    g_lighting.directional.intensity = 0.25f;   // Low - lets point lights dominate
    g_lighting.directional.orthogonal = 1;      // Shadows disabled by default
}

void lighting_enable(int enabled) {
    g_lighting.enabled = enabled;
}

int lighting_is_enabled(void) {
    return g_lighting.enabled;
}

void lighting_set_adaptive(int enabled) {
    g_lighting.adaptive = enabled;
}

int lighting_is_adaptive(void) {
    return g_lighting.adaptive;
}

void lighting_set_ambient(Color color) {
    g_lighting.ambient = color;
}


// --- DIRECTIONAL LIGHT ---

void lighting_set_directional(float angle, Color color, float intensity) {
    g_lighting.directional.angle = angle;
    g_lighting.directional.color = color;
    g_lighting.directional.intensity = intensity;
}

void lighting_set_sun_angle(float angle) {
    g_lighting.directional.angle = angle;
}

void lighting_set_orthogonal(int orthogonal) {
    g_lighting.directional.orthogonal = orthogonal;
}

int lighting_is_orthogonal(void) {
    return g_lighting.directional.orthogonal;
}

float lighting_get_sun_angle(void) {
    return g_lighting.directional.angle;
}

DirectionalLight lighting_get_directional(void) {
    return g_lighting.directional;
}

// --- POINT LIGHTS ---

int lighting_add_point(float x, float y, float radius, Color color, float intensity) {
    // Find first inactive slot or use next available
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        if (!g_lighting.lights[i].active) {
            PointLight* light = &g_lighting.lights[i];
            light->x = x;
            light->y = y;
            light->radius = radius;
            light->color = color;
            light->intensity = intensity;
            light->active = 1;
            
            // Update count if needed
            if (i >= g_lighting.count) {
                g_lighting.count = i + 1;
            }
            return i;
        }
    }
    
    printf("WARNING: Max point lights (%d) reached!\n", MAX_POINT_LIGHTS);
    return -1;
}

void lighting_update_point(int light_id, float x, float y) {
    if (light_id < 0 || light_id >= MAX_POINT_LIGHTS) return;
    if (!g_lighting.lights[light_id].active) return;
    
    g_lighting.lights[light_id].x = x;
    g_lighting.lights[light_id].y = y;
}

void lighting_set_point(int light_id, float x, float y, float radius, Color color, float intensity) {
    if (light_id < 0 || light_id >= MAX_POINT_LIGHTS) return;
    
    PointLight* light = &g_lighting.lights[light_id];
    light->x = x;
    light->y = y;
    light->radius = radius;
    light->color = color;
    light->intensity = intensity;
    light->active = 1;
}

void lighting_remove_point(int light_id) {
    if (light_id < 0 || light_id >= MAX_POINT_LIGHTS) return;
    
    g_lighting.lights[light_id].active = 0;
    
    // Recalculate count (find highest active index + 1)
    g_lighting.count = 0;
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        if (g_lighting.lights[i].active) {
            g_lighting.count = i + 1;
        }
    }
}

void lighting_clear_all(void) {
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        g_lighting.lights[i].active = 0;
    }
    g_lighting.count = 0;
}

int lighting_get_count(void) {
    return g_lighting.count;
}

void lighting_apply(void) {
    // Calculate effective ambient = base ambient + directional light contribution
    // Directional light acts as the "sun" that illuminates everything uniformly
    float eff_r = g_lighting.ambient.r + g_lighting.directional.color.r * g_lighting.directional.intensity;
    float eff_g = g_lighting.ambient.g + g_lighting.directional.color.g * g_lighting.directional.intensity;
    float eff_b = g_lighting.ambient.b + g_lighting.directional.color.b * g_lighting.directional.intensity;
    
    // Upload combined ambient + directional as the scene's base lighting
    GLint loc = glGetUniformLocation(shader_program, "uAmbient");
    if (loc != -1) {
        glUniform3f(loc, eff_r, eff_g, eff_b);
    }
    
    // Upload enabled state
    loc = glGetUniformLocation(shader_program, "uLightingEnabled");
    if (loc != -1) {
        glUniform1i(loc, g_lighting.enabled);
    }
    
    // Upload adaptive lighting state
    loc = glGetUniformLocation(shader_program, "uAdaptiveLights");
    if (loc != -1) {
        glUniform1i(loc, g_lighting.adaptive);
    }

    // Count active lights and upload
    int active_count = 0;
    for (int i = 0; i < g_lighting.count; i++) {
        if (g_lighting.lights[i].active) {
            active_count++;
        }
    }
    
    loc = glGetUniformLocation(shader_program, "uLightCount");
    if (loc != -1) {
        glUniform1i(loc, active_count);
    }
    
    // Upload each active light
    int upload_index = 0;
    for (int i = 0; i < g_lighting.count && upload_index < MAX_POINT_LIGHTS; i++) {
        PointLight* l = &g_lighting.lights[i];
        if (!l->active) continue;
        
        char name[64];

        sprintf(name, "uLightPos[%d]", upload_index);
        loc = glGetUniformLocation(shader_program, name);
        if (loc != -1) glUniform2f(loc, l->x, l->y);
        
        sprintf(name, "uLightColor[%d]", upload_index);
        loc = glGetUniformLocation(shader_program, name);
        if (loc != -1) glUniform3f(loc, l->color.r, l->color.g, l->color.b);

        sprintf(name, "uLightRadius[%d]", upload_index);
        loc = glGetUniformLocation(shader_program, name);
        if (loc != -1) glUniform1f(loc, l->radius);
        
        sprintf(name, "uLightIntensity[%d]", upload_index);
        loc = glGetUniformLocation(shader_program, name);
        if (loc != -1) glUniform1f(loc, l->intensity);
        
        upload_index++;
    }
}

float lighting_get_shadow_fade(float world_x, float world_y) {
    if (!g_lighting.enabled) return 0.0f;  // No fade if lighting disabled
    
    float total_light = 0.0f;
    
    // Accumulate light contribution from all point lights at this position
    for (int i = 0; i < g_lighting.count; i++) {
        PointLight* l = &g_lighting.lights[i];
        if (!l->active) continue;
        
        // Distance from shadow position to light
        float dx = world_x - l->x;
        float dy = world_y - l->y;
        float dist = sqrtf(dx * dx + dy * dy);
        
        // Smooth falloff within light radius
        if (dist < l->radius) {
            float attenuation = 1.0f - (dist / l->radius);
            attenuation = attenuation * attenuation;  // Quadratic falloff
            total_light += attenuation * l->intensity;
        }
    }
    
    // Clamp to 0-1 range (1.0 = fully lit, shadow should be invisible)
    return clampf(total_light, 0.0f, 1.0f);
}
