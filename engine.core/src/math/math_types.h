#pragma once

#include <defines.h>

// @brief Вектор из 2х элементов c плавающей точкой.
typedef union vec2_u {
    // @brief Массив из 2х элементов.
    f32 elements[2];
    struct {
        union {
            // @brief Первый элемент.
            f32 x, r, s, u;
        };
        union {
            // @brief Второй элемент.
            f32 y, g, t, v;
        };
    };
} vec2;

// @brief Вектор из 3х элементов c плавающей точкой.
typedef union vec3_u {
    // @brief Массив из 3х элементов.
    f32 elements[3];
    struct {
        union {
            // @brief Первый элемент.
            f32 x, r, s, u;
        };
        union {
            // @brief Второй элемент.
            f32 y, g, t, v;
        };
        union {
            // @brief Третий элемент.
            f32 z, b, p, w;
        };
    };
} vec3;

// @brief Вектор из 4х элементов c плавающей точкой.
typedef union vec4_u {
    // @brief Массив из 4х элементов.
    f32 elements[4];
    struct {
        union {
            // @brief Первый элемент.
            f32 x, r, s;
        };
        union {
            // @brief Второй элемент.
            f32 y, g, t;
        };
        union {
            // @brief Третий элемент.
            f32 z, b, p, width;
        };
        union {
            // @brief Четвертый элемент.
            f32 w, a, q, height;
        };
    };
} vec4;

// @brief Кватернион, предоставляет вращение.
typedef vec4 quat;

// @brief 2d прямоугольник.
typedef vec4 rect_2d;

// @brief Вектор из 4х целочисленных элементов без знака.
typedef union uvec4_u {
    // @brief Массив из 4х элементов.
    u32 elements[4];
    struct {
        union {
            // @brief Первый элемент.
            u32 x, r, s;
        };
        union {
            // @brief Второй элемент.
            u32 y, g, t;
        };
        union {
            // @brief Третий элемент.
            u32 z, b, p, width;
        };
        union {
            // @brief Четвертый элемент.
            u32 w, a, q, height;
        };
    };
} uvec4;

// @brief Матрица 3х3 из чисел c плавающей точкой.
typedef union mat3_u {
    // @brief Элементы матрицы.
    f32 data[12];
} mat3;

// @brief Матрица 4х4 из чисел с плавающей точкой. Обычно используется для трансформаций объекта.
typedef union mat4_u {
    // @brief Элементы матрицы.
    f32 data[16];
} mat4;

// @brief Представляет размеры 2D-объекта из чисел с плавающей точкой.
typedef struct extents_2d {
    // @brief Минимальные размеры объекта.
    vec2 min;
    // @brief Максимальные размеры объекта.
    vec2 max;
} extents_2d;

// @brief Представляет размеры 3D-объекта из чисел с плавающей точкой.
typedef struct extents_3d {
    // @brief Минимальные размеры объекта.
    vec3 min;
    // @brief Максимальные размеры объекта.
    vec3 max;
} extents_3d;

// @brief Представляет одну вершину в двумерном пространстве из чисел с плавающей точкой.
typedef struct vertex_2d {
    // @brief Положение вершины.
    vec2 position;
    // @brief Текстурная координата вершины.
    vec2 texcoord;
} vertex_2d;

// @brief Представляет одну вершину в трехмерном пространстве из чисел с плавающей точкой.
typedef struct vertex_3d {
    // @brief Положение вершины.
    vec3 position;
    // @brief Нормаль вершины.
    // vec3 normal;
    // @brief Текстурная координата вершины.
    // vec2 texcoord;
    // @brief Цвет вершины.
    // vec4 color;
    // @brief Касательная вершины.
    // vec3 tangent;
} vertex_3d;

// @brief Представляет отдельную вершину в трехмерном пространстве только с данными о положении и цвете.
typedef struct color_vertex_3d {
    // @brief Положение вершины (w игнорируется).
    vec4 position;
    // @brief Цвет вершины.
    vec4 color;
} color_vertex_3d;

typedef struct plane_3d {
    vec3 normal;
    f32 distance;
} plane_3d;

typedef enum frustum_side {
    FRUSTUM_SIDE_TOP    = 0,
    FRUSTUM_SIDE_BOTTOM = 1,
    FRUSTUM_SIDE_RIGHT  = 2,
    FRUSTUM_SIDE_LEFT   = 3,
    FRUSTUM_SIDE_FAR    = 4,
    FRUSTUM_SIDE_2BEAR  = 5,
    FRUSTUM_SIDES_MAX   = 6
} frustum_side;

typedef struct frustum {
    // Верхний, нижний, правый, левый, дальний, ближний.
    plane_3d sides[FRUSTUM_SIDES_MAX];
} frustum;

// @brief Вектор из 2x целочисленных элементов со знаком.
typedef union vec2i_u {
    // @brief Массив из 2х элементов.
    i32 elements[2];
    struct {
        union {
            // Первый элемент.
            i32 x, r, s, u;
        };
        union {
            // Второй элемент.
            i32 y, g, t, v;
        };
    };
} vec2i;

// @brief Вектор из 4х целочисленных элементов со знаком.
typedef union vec4i_u {
    // @brief Массив из 4х элементов.
    i32 elements[4];
    struct {
        union {
            // Первый элемент.
            i32 x, r, s;
        };
        union {
            // Второй элемент.
            i32 y, g, t;
        };
        union {
            // Третий элемент.
            i32 z, b, p, width;
        };
        union {
            // Четвертый элемент.
            i32 w, a, q, height;
        };
    };
} vec4i;
