// Собственные подключения.
#include "math/kmath.h"

// Внутренние подключения.
#include "platform/math.h"

f32 ksin(f32 x)
{
    return platform_math_sin(x);
}

f32 kcos(f32 x)
{
    return platform_math_cos(x);
}

f32 ktan(f32 x)
{
    return platform_math_tan(x);
}

f32 katan(f32 x)
{
    return platform_math_atan(x);
}

f32 kacos(f32 x)
{
    return platform_math_acos(x);
}

f32 ksqrt(f32 x)
{
    return platform_math_sqrt(x);
}

f32 kabs(f32 x)
{
    return platform_math_abs(x);
}

f32 kfloor(f32 x)
{
    return platform_math_floor(x);
}

f32 kceil(f32 x)
{
    return platform_math_ceil(x);
}

f32 klog2(f32 x)
{
    return platform_math_log2(x);
}

f32 kpow(f32 x, f32 p)
{
    return platform_math_pow(x, p);
}

i32 krandom()
{
    return platform_math_random();
}

f32 kfrandom()
{
    return platform_math_frandom();
}

i32 krandom_in_range(i32 min, i32 max)
{
    return platform_math_random_in_range(min, max);
}

f32 kfrandom_in_range(f32 min, f32 max)
{
    return platform_math_frandom_in_range(min, max);
}

// f32 kattenuation_min_max(f32 min, f32 max, f32 x)
// {
// }

// f32 vec3_distance_to_line(vec3 point, vec3 line_start, vec3 line_direction)
// {
// }

// plane_3d plane_3d_create(vec3 p1, vec3 norm)
// {
// }

// frustum frustum_create(const vec3 *position, const vec3 *forward, const vec3 *right, const vec3 *up, f32 aspect, f32 fov, f32 near, f32 far)
// {
// }

// frustum frustum_from_view_projection(mat4 view_projection)
// {
// }

// void frustum_corner_points_world_space(mat4 projection_view, vec4 *corners)
// {
// }

// f32 plane_signed_distance(const plane_3d *p, const vec3 *position)
// {
// }

// bool plane_intersects_sphere(const plane_3d *p, const vec3 *center, f32 radius)
// {
// }

// bool frustum_intersects_sphere(const frustum *f, const vec3 *center, f32 radius)
// {
// }

// bool plane_intersects_aabb(const plane_3d *p, const vec3 *center, const vec3 *extents)
// {
// }

// bool frustum_intersects_aabb(const frustum *f, const vec3 *center, const vec3 *extents)
// {
// }
