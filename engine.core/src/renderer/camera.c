// Собственные подключения.
#include "renderer/camera.h"

// Внутренние подключения.
#include "logger.h"
#include "math/kmath.h"

bool camera_check_status(const camera* c, const char* func)
{
    if(!c)
    {
        if(func)
        {
            kwarng("Function '%s' requires a valid pointer to camera. Nothing to do.", func);
        }
        return false;
    }
    return true;
}

camera camera_create()
{
    camera c;
    camera_reset(&c);
    return c;
}

void camera_reset(camera* c)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    c->euler_rotation = vec3_zero();
    c->position = vec3_zero();
    c->is_dirty = false;
    c->view_matrix = mat4_identity();
}

vec3 camera_position_get(const camera* c)
{
    if(!camera_check_status(c, __FUNCTION__)) return vec3_zero();
    return c->position;
}

void camera_position_set(camera* c, vec3 position)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    c->position = position;
    c->is_dirty = true;
}

vec3 camera_rotation_euler_get(const camera* c)
{
    if(!camera_check_status(c, __FUNCTION__)) return vec3_zero();
    return c->euler_rotation;
}

void camera_rotation_euler_set(camera* c, vec3 rotation)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    c->euler_rotation = rotation;
    c->is_dirty = true;    
}

mat4 camera_view_get(camera* c)
{
    if(!camera_check_status(c, __FUNCTION__)) return mat4_identity();

    if(c->is_dirty)
    {
        mat4 rotation = mat4_euler_xyz(c->euler_rotation.x, c->euler_rotation.y, c->euler_rotation.z);
        mat4 translation = mat4_translation(c->position);

        c->view_matrix = mat4_mul(rotation, translation);
        c->view_matrix = mat4_inverse(c->view_matrix);

        c->is_dirty = false;
    }

    return c->view_matrix;
}

vec3 camera_forward(camera* c)
{
    mat4 view = camera_view_get(c);
    return mat4_forward(view);
}

vec3 camera_backward(camera* c)
{
    mat4 view = camera_view_get(c);
    return mat4_backward(view);
}

vec3 camera_left(camera* c)
{
    mat4 view = camera_view_get(c);
    return mat4_left(view);
}

vec3 camera_right(camera* c)
{
    mat4 view = camera_view_get(c);
    return mat4_right(view);
}

void camera_move_forward(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    vec3 direction = camera_forward(c);
    direction = vec3_mul_scalar(direction, amount);
    c->position = vec3_add(c->position, direction);
    c->is_dirty = true;
}

void camera_move_backward(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    vec3 direction = camera_backward(c);
    direction = vec3_mul_scalar(direction, amount);
    c->position = vec3_add(c->position, direction);
    c->is_dirty = true;
}

void camera_move_left(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    vec3 direction = camera_left(c);
    direction = vec3_mul_scalar(direction, amount);
    c->position = vec3_add(c->position, direction);
    c->is_dirty = true;
}

void camera_move_right(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    vec3 direction = camera_right(c);
    direction = vec3_mul_scalar(direction, amount);
    c->position = vec3_add(c->position, direction);
    c->is_dirty = true;
}

void camera_move_up(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    vec3 direction = vec3_up();
    direction = vec3_mul_scalar(direction, amount);
    c->position = vec3_add(c->position, direction);
    c->is_dirty = true;
}

void camera_move_down(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    vec3 direction = vec3_down();
    direction = vec3_mul_scalar(direction, amount);
    c->position = vec3_add(c->position, direction);
    c->is_dirty = true;
}

void camera_yaw(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    c->euler_rotation.y += amount;
    c->is_dirty = true;
}

void camera_pitch(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    c->euler_rotation.x += amount;
    c->is_dirty = true;
}

void camera_roll(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    c->euler_rotation.z += amount;
    c->is_dirty = true;
}

void camera_move_horizontal(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    c->euler_rotation.y += amount;
    c->is_dirty = true;
}

void camera_move_vertical(camera* c, f32 amount)
{
    if(!camera_check_status(c, __FUNCTION__)) return;
    c->euler_rotation.x += amount;

    // TODO: Перенести в структуру и сделать настраиваемым.
    static const f32 limit = 1.55334306f; // 89 градусов (deg_to_rad(89.0f))
    c->euler_rotation.x = KCLAMP(c->euler_rotation.x, -limit, limit);

    c->is_dirty = true;
}
