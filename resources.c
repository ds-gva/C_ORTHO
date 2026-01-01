// resources.c — Centralized asset loading and caching

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include "resources.h"
#include <glad/glad.h>
#include <string.h>
#include <stdio.h>

// --- TEXTURE CACHE ---
#define MAX_TEXTURES 128

typedef struct {
    char path[256];      // Key for lookup
    Texture texture;     // The actual data
    int ref_count;       // Track how many things use it
} TextureEntry;

static TextureEntry texture_cache[MAX_TEXTURES];
static int texture_count = 0;

// Texture filter mode (exposed via set_texture_filter_mode in renderer)
extern GLint g_texture_filter_mode;

// Find texture by path (returns NULL if not cached)
static TextureEntry* find_texture(const char* path) {
    for (int i = 0; i < texture_count; i++) {
        if (strcmp(texture_cache[i].path, path) == 0) {
            return &texture_cache[i];
        }
    }
    return NULL;
}

void resources_init(void) {
    // Currently nothing to initialize
    // Future: could pre-load common assets here
}

Texture* resource_load_texture(const char* path) {
    // 1. Check cache first
    TextureEntry* existing = find_texture(path);
    if (existing) {
        existing->ref_count++;
        return &existing->texture;
    }
    
    // 2. Not cached — load from disk
    if (texture_count >= MAX_TEXTURES) {
        printf("ERROR: Texture cache full!\n");
        return NULL;
    }
    
    // Load Image Data from Disk (CPU RAM)
    int width, height, channels;
    unsigned char *data = stbi_load(path, &width, &height, &channels, 0);
    
    if (!data) {
        printf("ERROR: Could not load texture: %s\n", path);
        return NULL;
    }

    // Generate Texture on GPU
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Setup Filtering 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_texture_filter_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_texture_filter_mode);
    
    // Setup Wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload Data to VRAM
    int format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    // Free CPU RAM
    stbi_image_free(data);

    // 3. Store in cache
    TextureEntry* entry = &texture_cache[texture_count++];
    strncpy(entry->path, path, 255);
    entry->path[255] = '\0';  // Ensure null termination
    entry->texture.id = texture_id;
    entry->texture.width = width;
    entry->texture.height = height;
    entry->ref_count = 1;
    
    return &entry->texture;
}

void resource_unload_texture(const char* path) {
    for (int i = 0; i < texture_count; i++) {
        if (strcmp(texture_cache[i].path, path) == 0) {
            texture_cache[i].ref_count--;
            
            // Only actually delete if no more references
            if (texture_cache[i].ref_count <= 0) {
                glDeleteTextures(1, &texture_cache[i].texture.id);
                
                // Swap with last entry to keep array packed
                texture_cache[i] = texture_cache[texture_count - 1];
                texture_count--;
            }
            return;
        }
    }
}

void resources_shutdown(void) {
    // Free all textures
    for (int i = 0; i < texture_count; i++) {
        glDeleteTextures(1, &texture_cache[i].texture.id);
    }
    texture_count = 0;
    
    printf("Resources shutdown: freed all cached textures\n");
}
