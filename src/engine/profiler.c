// profiler.c — Performance measurement implementation

// Windows headers MUST come before OpenGL to avoid APIENTRY conflicts
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glad/glad.h>
#include <stdio.h>
#include <string.h>

#include "profiler.h"
#include "engine.h"
#include "font.h"

// Global stats instance
FrameStats g_stats = {0};

// High-precision timer state
static LARGE_INTEGER perf_freq;
static int freq_initialized = 0;

// Section timing storage
static double update_start = 0;
static double render_start = 0;
static double swap_start = 0;

// GPU timer queries (double-buffered for async)
static GLuint gpu_timer_queries[2];
static int current_query_index = 0;
static int gpu_timer_initialized = 0;

// --- HIGH PRECISION TIMER ---

double profiler_get_time_ms(void) {
    if (!freq_initialized) {
        QueryPerformanceFrequency(&perf_freq);
        freq_initialized = 1;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)perf_freq.QuadPart * 1000.0;
}

// --- FRAME TIMING ---

void profiler_frame_begin(void) {
    g_stats.frame_start = profiler_get_time_ms();
    
    // Reset per-frame counters
    g_stats.draw_calls = 0;
    g_stats.quads_drawn = 0;
    g_stats.texture_switches = 0;
}

void profiler_frame_end(void) {
    double now = profiler_get_time_ms();
    g_stats.frame_time_ms = now - g_stats.frame_start;
    
    // Rolling average (exponential moving average, ~20 frame window)
    if (g_stats.frame_count == 0) {
        g_stats.avg_frame_time = g_stats.frame_time_ms;
        g_stats.min_frame_time = g_stats.frame_time_ms;
        g_stats.max_frame_time = g_stats.frame_time_ms;
    } else {
        g_stats.avg_frame_time = g_stats.avg_frame_time * 0.95 + g_stats.frame_time_ms * 0.05;
        
        if (g_stats.frame_time_ms < g_stats.min_frame_time) 
            g_stats.min_frame_time = g_stats.frame_time_ms;
        if (g_stats.frame_time_ms > g_stats.max_frame_time) 
            g_stats.max_frame_time = g_stats.frame_time_ms;
    }
    
    // Store in history buffer for graph
    g_stats.frame_history[g_stats.frame_history_index] = (float)g_stats.frame_time_ms;
    g_stats.frame_history_index = (g_stats.frame_history_index + 1) % FRAME_HISTORY_SIZE;
    
    g_stats.frame_count++;
}

// --- SECTION TIMING ---

void profiler_begin_update(void) {
    update_start = profiler_get_time_ms();
}

void profiler_end_update(void) {
    g_stats.update_time_ms = profiler_get_time_ms() - update_start;
}

void profiler_begin_render(void) {
    render_start = profiler_get_time_ms();
}

void profiler_end_render(void) {
    g_stats.render_time_ms = profiler_get_time_ms() - render_start;
}

void profiler_begin_swap(void) {
    swap_start = profiler_get_time_ms();
}

void profiler_end_swap(void) {
    g_stats.swap_time_ms = profiler_get_time_ms() - swap_start;
}

// --- GPU TIMER ---

void profiler_init_gpu_timer(void) {
    glGenQueries(2, gpu_timer_queries);
    gpu_timer_initialized = 1;
    
    // Prime both queries with dummy values
    for (int i = 0; i < 2; i++) {
        glBeginQuery(GL_TIME_ELAPSED, gpu_timer_queries[i]);
        glEndQuery(GL_TIME_ELAPSED);
    }
}

void profiler_gpu_begin(void) {
    if (!gpu_timer_initialized) return;
    glBeginQuery(GL_TIME_ELAPSED, gpu_timer_queries[current_query_index]);
}

void profiler_gpu_end(void) {
    if (!gpu_timer_initialized) return;
    glEndQuery(GL_TIME_ELAPSED);
    
    // Read PREVIOUS frame's result (async - doesn't stall GPU)
    int prev_index = 1 - current_query_index;
    GLuint64 elapsed_ns = 0;
    glGetQueryObjectui64v(gpu_timer_queries[prev_index], GL_QUERY_RESULT, &elapsed_ns);
    g_stats.gpu_time_ms = (double)elapsed_ns / 1000000.0;
    
    // Swap query index for next frame
    current_query_index = prev_index;
}

// --- RENDERER INSTRUMENTATION ---

void profiler_record_draw_call(int quad_count) {
    g_stats.draw_calls++;
    g_stats.quads_drawn += quad_count;
}

void profiler_record_texture_switch(void) {
    g_stats.texture_switches++;
}

// --- UTILITIES ---

void profiler_reset_minmax(void) {
    g_stats.min_frame_time = g_stats.frame_time_ms;
    g_stats.max_frame_time = g_stats.frame_time_ms;
}

// --- DEBUG OVERLAY ---

void profiler_draw_overlay(Font* font, float x, float y) {
    if (!font) return;
    
    char buf[128];
    float line_height = 18.0f;
    float current_y = y;
    
    // FPS and frame time
    double fps = (g_stats.avg_frame_time > 0.001) ? 1000.0 / g_stats.avg_frame_time : 0.0;
    snprintf(buf, sizeof(buf), "FPS: %.1f (%.2f ms)", fps, g_stats.avg_frame_time);
    draw_text(font, buf, x, current_y, COLOR_WHITE);
    current_y += line_height;
    
    // Frame time breakdown
    snprintf(buf, sizeof(buf), "Update: %.2f ms", g_stats.update_time_ms);
    draw_text(font, buf, x, current_y, COLOR_GREEN);
    current_y += line_height;
    
    snprintf(buf, sizeof(buf), "Render: %.2f ms", g_stats.render_time_ms);
    draw_text(font, buf, x, current_y, COLOR_YELLOW);
    current_y += line_height;
    
    snprintf(buf, sizeof(buf), "GPU: %.2f ms", g_stats.gpu_time_ms);
    draw_text(font, buf, x, current_y, (Color){0.5f, 0.8f, 1.0f, 1.0f});
    current_y += line_height;
    
    snprintf(buf, sizeof(buf), "VSync: %.2f ms", g_stats.swap_time_ms);
    draw_text(font, buf, x, current_y, COLOR_GRAY);
    current_y += line_height;
    
    // Renderer stats
    current_y += 4.0f; // Small gap
    snprintf(buf, sizeof(buf), "Draw Calls: %d", g_stats.draw_calls);
    draw_text(font, buf, x, current_y, COLOR_WHITE);
    current_y += line_height;
    
    snprintf(buf, sizeof(buf), "Quads: %d", g_stats.quads_drawn);
    draw_text(font, buf, x, current_y, COLOR_WHITE);
    current_y += line_height;
    
    snprintf(buf, sizeof(buf), "Tex Switches: %d", g_stats.texture_switches);
    draw_text(font, buf, x, current_y, 
        g_stats.texture_switches > 10 ? COLOR_RED : COLOR_WHITE);
    current_y += line_height;
    
    // Min/Max frame times
    current_y += 4.0f;
    snprintf(buf, sizeof(buf), "Min: %.2f  Max: %.2f ms", 
        g_stats.min_frame_time, g_stats.max_frame_time);
    draw_text(font, buf, x, current_y, COLOR_GRAY);
}

void profiler_draw_graph(float x, float y, float width, float height) {
    // Background
    draw_rect(x + width/2, y + height/2, width, height, 0, 
        (Color){0.0f, 0.0f, 0.0f, 0.7f}, 0);
    
    // 16.6ms target line (60 FPS)
    float target_y = y + height - (16.66f / 33.33f) * height;
    draw_rect(x + width/2, target_y, width, 1, 0, 
        (Color){0.0f, 0.6f, 0.0f, 0.8f}, 0);
    
    // 33.3ms line (30 FPS)
    float slow_y = y + height - (33.33f / 33.33f) * height;
    draw_rect(x + width/2, slow_y, width, 1, 0, 
        (Color){0.6f, 0.0f, 0.0f, 0.8f}, 0);
    
    // Frame time bars
    float bar_width = width / (float)FRAME_HISTORY_SIZE;
    
    for (int i = 0; i < FRAME_HISTORY_SIZE; i++) {
        // Read from history buffer (oldest to newest)
        int idx = (g_stats.frame_history_index + i) % FRAME_HISTORY_SIZE;
        float frame_ms = g_stats.frame_history[idx];
        
        // Calculate bar height (scale to 33.33ms max)
        float bar_height = (frame_ms / 33.33f) * height;
        if (bar_height > height) bar_height = height;
        if (bar_height < 1.0f) bar_height = 1.0f;
        
        // Color: green if under 16.6ms, yellow if under 33.3ms, red if over
        Color bar_color;
        if (frame_ms <= 16.66f) {
            bar_color = (Color){0.2f, 0.8f, 0.2f, 0.9f};
        } else if (frame_ms <= 33.33f) {
            bar_color = (Color){0.9f, 0.7f, 0.1f, 0.9f};
        } else {
            bar_color = (Color){0.9f, 0.2f, 0.2f, 0.9f};
        }
        
        float bar_x = x + i * bar_width + bar_width / 2;
        float bar_y = y + height - bar_height / 2;
        
        draw_rect(bar_x, bar_y, bar_width - 1, bar_height, 0, bar_color, 0);
    }
}
