#include "utils.h"
#include <math.h>
#include "math_common.h"
#include <stdio.h>
#include <stdlib.h>

// Helper function to rotate an entity look at a target ; Thinking of it, this might be better suited in the entity module?
void look_at(Entity *e, float target_x, float target_y) {
    float dx = target_x - e->x;
    float dy = target_y - e->y;
    float angle = atan2f(dy, dx) * RAD2DEG;
    
    
    e->rotation = angle;
}

// Helper function to generate a random float between a min and max
float randf(float min, float max) {
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

// Returns a null-terminated string with file contents
// Caller is responsible for freeing the returned pointer
char* load_file_text(const char* path) {
    FILE* file = fopen(path, "rb");  // Binary mode to get accurate size
    if (!file) {
        printf("ERROR: Could not open file: %s\n", path);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);  // Rewind
    
    // Allocate buffer (+1 for null terminator)
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        printf("ERROR: Could not allocate memory for file: %s\n", path);
        fclose(file);
        return NULL;
    }
    
    // Read entire file
    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';  // Null-terminate
    
    fclose(file);
    return buffer;
}