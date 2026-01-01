#ifndef RESOURCES_H
#define RESOURCES_H

#include "engine.h"

// Initialize/shutdown the resource system
void resources_init(void);
void resources_shutdown(void);  // Frees ALL loaded resources

// Textures
Texture* resource_load_texture(const char* path);  // Returns cached if already loaded
void resource_unload_texture(const char* path);    // Manual unload (optional)

// Future:
// Sound* resource_load_sound(const char* path);
// Font* resource_load_font(const char* path, int size);

#endif