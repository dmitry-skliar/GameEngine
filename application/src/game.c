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

    inst->state = kallocate_tc(game_state, 1, MEMORY_TAG_GAME);

    return true;
}

void game_destroy(game* inst)
{
    kfree_tc(inst->state, game_state, 1, MEMORY_TAG_GAME);
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

    state->view = mat4_mul(rotation, translation);
    state->view = mat4_inverse(state->view);
    state->camera_view_dirty = false;
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
    f32 limit = deg_to_rad(40.0f);
    state->camera_euler.x = KCLAMP(state->camera_euler.x, -limit, limit);
    
    state->camera_view_dirty = true;
}

bool game_initialize(game* inst)
{
    game_state* state = (game_state*)inst->state;

    state->camera_position = vec3_create(0, 0, 30.0f);
    state->camera_euler = vec3_zero();

    state->view = mat4_translation(state->camera_position);
    state->view = mat4_inverse(state->view);
    state->camera_view_dirty = true;

    return true;
}

bool game_update(game* inst, f32 delta_time)
{
    if(input_keyboard_key_press_detect('M'))
    {
        static u64 alloc_count = 0;
        u64 prev_alloc_count = alloc_count;
        alloc_count = memory_system_alloc_count();

        kdebug("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
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

    f32 move_speed = 50.0f * delta_time;
    vec3 velocity = vec3_zero();
    game_state* state = inst->state;

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
        // Должен быть нормализован перед применением скорости.
        vec3_normalize(&velocity);
        state->camera_position.x += velocity.x * move_speed;
        state->camera_position.y += velocity.y * move_speed;
        state->camera_position.z += velocity.z * move_speed;
        state->camera_view_dirty = true;
    }

    recalculate_view_matrix(inst->state);

    // HACK: Удалить!
    renderer_set_view(((game_state*)inst->state)->view);
    
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
