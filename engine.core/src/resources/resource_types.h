#pragma once

#include <math/math_types.h>

// @brief Описывает текстуру.
typedef struct texture {
    u32 id;
    u32 width;
    u32 height;
    u8 channel_count;
    bool has_transparency;
    u32 generation;
    void* data;
} texture;
