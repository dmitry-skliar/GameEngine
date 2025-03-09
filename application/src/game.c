#include "game.h"

#include <input.h>
#include <memory/memory.h>
#include <event.h>

// HACK: Удалить позже после тестов!
#include <renderer/renderer_frontend.h>

bool game_create(game* inst)
{
    inst->window_title  = "Game Application";
    inst->window_width  = 1024;
    inst->window_height = 768;

    inst->initialize = game_initialize;
    inst->update     = game_update;
    inst->render     = game_render;
    inst->on_resize  = game_on_resize;

    inst->state_memory_requirement = sizeof(game_state);

    return true;
}

void game_destroy(game* inst)
{
}

void recalculate_view_matrix(game_state* state)
{
    if(!state->camera_view_dirty)
    {
        return;
    }

    // Тангаж, рыскание, крен (pitch, yaw, roll).
    mat4 rotation = mat4_euler_xyz(state->camera_euler.x, state->camera_euler.y, state->camera_euler.z);
    mat4 translation = mat4_translation(state->camera_position);

    // NOTE: Порядок трансформации: масштаб, поворот и перемещение!
    state->view = mat4_mul(rotation, translation);
    state->view = mat4_inverse(state->view);
    state->camera_view_dirty = false;

    // HACK: Удалить!
    renderer_set_view(state->view, state->camera_position);
}

void camera_yaw(game_state* state, f32 amount)
{
    state->camera_euler.y += amount;
    state->camera_view_dirty = true;
}

void camera_pitch(game_state* state, f32 amount)
{
    state->camera_euler.x += amount;

    // Обрезание наклонов.
    f32 limit = deg_to_rad(90.0f);
    state->camera_euler.x = KCLAMP(state->camera_euler.x, -limit, limit);
    
    state->camera_view_dirty = true;
}

bool game_initialize(game* inst)
{
    game_state* state = inst->state;

    // Стартовая позиция камеры.
    state->camera_position = vec3_create(47.65f, 14.88f, -6.06f);
    state->camera_euler = vec3_create(-0.16f, 1.55f, 0.0f);

    state->view = mat4_translation(state->camera_position);
    state->view = mat4_inverse(state->view);
    state->camera_view_dirty = true;

    return true;
}

bool game_update(game* inst, f32 delta_time)
{
    game_state* state = inst->state;

    if(input_keyboard_key_press_detect('M'))
    {
        static u64 alloc_count = 0;
        u64 prev_alloc_count = alloc_count;
        alloc_count = memory_system_allocation_count();
        kdebug("Allocations last: %llu, total %llu.", alloc_count - prev_alloc_count, alloc_count);
    }

    if(input_keyboard_key_press_detect('T'))
    {
        event_send(EVENT_CODE_DEBUG_0, inst, null);
    }

    if(input_is_keyboard_key_down(KEY_LEFT))
    {
        camera_yaw(inst->state, 1.0f * delta_time);
    }

    if(input_is_keyboard_key_down(KEY_RIGHT))
    {
        camera_yaw(inst->state, -1.0f * delta_time);
    }

    if(input_is_keyboard_key_down(KEY_UP))
    {
        camera_pitch(inst->state, 1.0f * delta_time);
    }

    if(input_is_keyboard_key_down(KEY_DOWN))
    {
        camera_pitch(inst->state, -1.0f * delta_time);
    }

    vec3 velocity = vec3_zero();

    if(input_is_keyboard_key_down('W'))
    {
        vec3 forward = mat4_forward(state->view);
        velocity = vec3_add(velocity, forward);
    }

    if(input_is_keyboard_key_down('S'))
    {
        vec3 backward = mat4_backward(state->view);
        velocity = vec3_add(velocity, backward);
    }

    if(input_is_keyboard_key_down('A'))
    {
        vec3 left = mat4_left(state->view);
        velocity = vec3_add(velocity, left);
    }

    if(input_is_keyboard_key_down('D'))
    {
        vec3 right = mat4_right(state->view);
        velocity = vec3_add(velocity, right);
    }

    if(input_is_keyboard_key_down(KEY_SPACE))
    {
        velocity.y += 1.0f;
    }

    if(input_is_keyboard_key_down(KEY_LSHIFT))
    {
        velocity.y -= 1.0f;
    }

    vec3 z = vec3_zero();
    if(!vec3_compare(z, velocity, 0.0002f))
    {
        f32 move_speed = 50.0f * delta_time;

        // Должен быть нормализован перед применением скорости.
        vec3_normalize(&velocity);
        state->camera_position.x += velocity.x * move_speed;
        state->camera_position.y += velocity.y * move_speed;
        state->camera_position.z += velocity.z * move_speed;
        state->camera_view_dirty = true;
    }

    recalculate_view_matrix(inst->state);

    if(input_keyboard_key_press_detect('P'))
    {
        kdebug("Camera position: [%.2f, %.2f, %.2f]", state->camera_position.x, state->camera_position.y, state->camera_position.z);
        kdebug("Camera euler   : [%.2f, %.2f, %.2f]", state->camera_euler.x, state->camera_euler.y, state->camera_euler.z);
    }

    if(input_keyboard_key_press_detect('1'))
    {
        event_context data = {};
        data.i32[0] = RENDERER_VIEW_MODE_DEFAULT;
        event_send(EVENT_CODE_SET_RENDER_MODE, inst, &data);
    }

    if(input_keyboard_key_press_detect('2'))
    {
        event_context data = {};
        data.i32[0] = RENDERER_VIEW_MODE_LIGHTING;
        event_send(EVENT_CODE_SET_RENDER_MODE, inst, &data);
    }

    if(input_keyboard_key_press_detect('3'))
    {
        event_context data = {};
        data.i32[0] = RENDERER_VIEW_MODE_NORMALS;
        event_send(EVENT_CODE_SET_RENDER_MODE, inst, &data);
    }

    return true;
}

bool game_render(game* inst, f32 delta_time)
{
    return true;
}

void game_on_resize(game* inst, i32 width, i32 height)
{
    kdebug("Game resized (w/h): %d / %d", width, height);

}
