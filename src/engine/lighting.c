// lighting.c — Simple 2D point light system

#include "lighting.h"
#include <glad/glad.h>
#include <stdio.h>
#include <string.h>

// External reference to shader program from renderer
extern GLuint shader_program;

// Internal lighting state
typedef struct {
    PointLight lights[MAX_POINT_LIGHTS];
    int count;
    Color ambient;
    int enabled;
} LightingState;

static LightingState g_lighting = {0};

void lighting_init(void) {
    memset(&g_lighting, 0, sizeof(LightingState));
    g_lighting.enabled = 1;
    g_lighting.ambient = (Color){0.3f, 0.3f, 0.35f, 1.0f};  // Slightly bright default
    g_lighting.count = 0;
}

void lighting_enable(int enabled) {
    g_lighting.enabled = enabled;
}

int lighting_is_enabled(void) {
    return g_lighting.enabled;
}

void lighting_set_ambient(Color color) {
    g_lighting.ambient = color;
}

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
    // Upload ambient color
    GLint loc = glGetUniformLocation(shader_program, "uAmbient");
    if (loc != -1) {
        glUniform3f(loc, g_lighting.ambient.r, g_lighting.ambient.g, g_lighting.ambient.b);
    }
    
    // Upload enabled state
    loc = glGetUniformLocation(shader_program, "uLightingEnabled");
    if (loc != -1) {
        glUniform1i(loc, g_lighting.enabled);
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
