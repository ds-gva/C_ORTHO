// profiler.h — Performance measurement and statistics
#ifndef PROFILER_H
#define PROFILER_H

// Frame statistics structure
typedef struct {
    // Timing (in milliseconds)
    double frame_start;        // Internal: start timestamp
    double frame_time_ms;      // Total frame time
    double update_time_ms;     // Game logic time
    double render_time_ms;     // Rendering time
    double swap_time_ms;       // Buffer swap / vsync wait
    
    // GPU timing (1 frame behind due to async query)
    double gpu_time_ms;
    
    // Counters (reset each frame)
    int draw_calls;            // How many flushes
    int quads_drawn;           // Total quads this frame
    int texture_switches;      // Texture change flushes
    
    // Rolling statistics
    double avg_frame_time;     // Exponential moving average
    double min_frame_time;     // Min over recent frames
    double max_frame_time;     // Max over recent frames
    int frame_count;           // Total frames since start
    
    // Frame time history for graph
    #define FRAME_HISTORY_SIZE 120
    float frame_history[FRAME_HISTORY_SIZE];
    int frame_history_index;
} FrameStats;

// Global stats instance
extern FrameStats g_stats;

// High-precision timer
double profiler_get_time_ms(void);

// Frame timing functions (call from main loop)
void profiler_frame_begin(void);
void profiler_frame_end(void);

// Section timing (call around update/render)
void profiler_begin_update(void);
void profiler_end_update(void);
void profiler_begin_render(void);
void profiler_end_render(void);
void profiler_begin_swap(void);
void profiler_end_swap(void);

// GPU timer (requires OpenGL context)
void profiler_init_gpu_timer(void);
void profiler_gpu_begin(void);
void profiler_gpu_end(void);

// Renderer instrumentation (called from renderer_opengl.c)
void profiler_record_draw_call(int quad_count);
void profiler_record_texture_switch(void);

// Reset min/max stats (call periodically, e.g., every second)
void profiler_reset_minmax(void);

// --- DEBUG OVERLAY ---
// Draw performance stats overlay (requires font system)
// Call this AFTER end_camera_mode() in your render loop
struct Font;  // Forward declaration
void profiler_draw_overlay(struct Font* font, float x, float y);

// Draw a frame time graph showing recent performance
void profiler_draw_graph(float x, float y, float width, float height);

#endif
