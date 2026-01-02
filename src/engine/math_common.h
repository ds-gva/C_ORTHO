#ifndef MATH_COMMON_H
#define MATH_COMMON_H

#define PI      3.14159265358979f
#define TAU     (PI * 2.0f)
#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)

// Useful for top-down games
static inline float clampf(float v, float min, float max) {
    return v < min ? min : (v > max ? max : v);
}

static inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

// Move 'from' toward 'to' by at most 'max_delta'
// Returns 'to' if distance is less than max_delta
static inline float move_towardf(float from, float to, float max_delta) {
    float diff = to - from;
    if (diff > max_delta) return from + max_delta;
    if (diff < -max_delta) return from - max_delta;
    return to;
}

#endif