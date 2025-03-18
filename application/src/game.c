#include "game.h"

#include <input.h>
#include <event.h>
#include <memory/memory.h>
#include <systems/camera_system.h>

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

bool game_initialize(game* inst)
{
    game_state* state = inst->state;

    // Получение камеры и настройка камеры.
    state->world_camera = camera_system_get_default();
    camera_position_set(state->world_camera, vec3_create(47.65f, 14.88f, -6.06f));
    camera_rotation_euler_set(state->world_camera, vec3_create(-0.16f, 1.55f, 0.0f));
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
        camera_move_horizontal(state->world_camera, 1.0f * delta_time);
    }

    if(input_is_keyboard_key_down(KEY_RIGHT))
    {
        camera_move_horizontal(state->world_camera, -1.0f * delta_time);
    }

    if(input_is_keyboard_key_down(KEY_UP))
    {
        camera_move_vertical(state->world_camera, 1.0f * delta_time);
    }

    if(input_is_keyboard_key_down(KEY_DOWN))
    {
        camera_move_vertical(state->world_camera, -1.0f * delta_time);
    }

    f32 move_speed = 50.0f * delta_time;

    if(input_is_keyboard_key_down('W'))
    {
        camera_move_forward(state->world_camera, move_speed);
    }

    if(input_is_keyboard_key_down('S'))
    {
        camera_move_backward(state->world_camera, move_speed);
    }

    if(input_is_keyboard_key_down('A'))
    {
        camera_move_left(state->world_camera, move_speed);
    }

    if(input_is_keyboard_key_down('D'))
    {
        camera_move_right(state->world_camera, move_speed);
    }

    if(input_is_keyboard_key_down(KEY_SPACE))
    {
        camera_move_up(state->world_camera, move_speed);
    }

    if(input_is_keyboard_key_down(KEY_LSHIFT))
    {
        camera_move_down(state->world_camera, move_speed);
    }

    if(input_keyboard_key_press_detect('P'))
    {
        kdebug(
            "Camera position: [%.2f, %.2f, %.2f]",
            state->world_camera->position.x, state->world_camera->position.y, state->world_camera->position.z
        );

        kdebug(
            "Camera euler   : [%.2f, %.2f, %.2f]",
            state->world_camera->euler_rotation.x, state->world_camera->euler_rotation.y,
            state->world_camera->euler_rotation.z
        );
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
