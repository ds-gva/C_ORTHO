#include "utils.h"
#include <math.h>
#include "math_common.h"
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