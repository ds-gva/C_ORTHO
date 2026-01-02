#ifndef UTILS_H
#define UTILS_H

#include "engine.h"

void look_at(Entity *e, float target_x, float target_y);
char* load_file_text(const char* path);
Color lerp_color(Color a, Color b, float t);
float randf(float min, float max);

#endif