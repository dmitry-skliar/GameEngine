// Cобственные подключения.
#include "application.h"

// Внутренние подключения.
#include "logger.h"
#include "event.h"
#include "platform/window.h"
#include "platform/memory.h"
#include "platform/time.h"
#include "platform/thread.h"
#include "memory/memory.h"
#include "memory/allocators/linear_allocator.h"
#include "input.h"
#include "clock.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"

typedef struct application_state {
    game* game_inst;
    bool  is_running;
    bool  is_suspended;
    clock clock;
    f64   last_time;

    linear_allocator* systems_allocator;

    u64 memory_system_memory_requirement;
    void* memory_system_state;

    u64 event_system_memory_requirement;
    void* event_system_state;

    u64 input_system_memory_requirement;
    void* input_system_state;

    u64 platform_window_memory_requirement;
    window* platform_window_state;

    u64 renderer_system_memory_requirement;
    void* renderer_system_state;

    u64 texture_system_memory_requirement;
    void* textute_system_state;

    u64 material_system_memory_requirement;
    void* material_system_state;

} application_state;

static application_state* app_state = null;

void application_on_focus(bool focused);
void application_on_mouse_wheel(i32 zdelta);
void application_on_mouse_button(u32 button, bool pressed);
void application_on_mouse_move(i32 x, i32 y);
void application_on_keyboard_key(u32 keycode, bool pressed);
void application_on_resize(i32 width, i32 height);
void application_on_close();

bool application_create(game* game_inst)
{
    if(app_state)
    {
        kerror("Function '%s' was called more than once. Return false!", __FUNCTION__);
        return false;
    }

    if(!game_inst)
    {
        kerror("Function '%s' require a game structure. Return false!", __FUNCTION__);
        return false;
    }

    // Создание контекста приложения.
    app_state = kallocate_tc(application_state, 1, MEMORY_TAG_APPLICATION);
    kzero_tc(app_state, application_state, 1);
    app_state->game_inst = game_inst;
    game_inst->application_state = app_state;

    // TODO: Сделать менеджер систем и подсистем. Решит проблему правильной инициализации и завершения.
    // Создание системного линейного распределителя памяти.
    u64 systems_allocator_total_size = 64 * 1024 * 1024; // 64 Mb
    app_state->systems_allocator = linear_allocator_create(systems_allocator_total_size);

    // Система контроля памяти.
    memory_system_initialize(&app_state->memory_system_memory_requirement, null);
    app_state->memory_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->memory_system_memory_requirement);
    memory_system_initialize(&app_state->memory_system_memory_requirement, app_state->memory_system_state);
    kinfor("Memory system started.");

    // Система событий (должно быть инициализировано до создания окна приложения).
    event_system_initialize(&app_state->event_system_memory_requirement, null);
    app_state->event_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->event_system_memory_requirement);
    event_system_initialize(&app_state->event_system_memory_requirement, app_state->event_system_state);
    kinfor("Event system started.");

    // TODO: Отвязать от системы событий!
    // Система ввода (должно быть инициализировано до создания окна приложения, но после системы событий - связаны).
    input_system_initialize(&app_state->input_system_memory_requirement, null);
    app_state->input_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->input_system_memory_requirement);
    input_system_initialize(&app_state->input_system_memory_requirement, app_state->input_system_state);
    kinfor("Input system started.");

    // Создание окна приложения.
    window_config window_sys_config;
    window_sys_config.title = game_inst->window_title;
    window_sys_config.width = game_inst->window_width;
    window_sys_config.height = game_inst->window_height;
    platform_window_create(&app_state->platform_window_memory_requirement, null, null);
    app_state->platform_window_state = linear_allocator_allocate(app_state->systems_allocator, app_state->platform_window_memory_requirement);
    if(!platform_window_create(&app_state->platform_window_memory_requirement, app_state->platform_window_state, &window_sys_config))
    {
        kerror("Failed to create window. Aborted!");
        return false;
    }
    kinfor("Platform window created.");

    // Установка обработчиков событий окна.
    platform_window_set_on_close_handler(app_state->platform_window_state, application_on_close);
    platform_window_set_on_resize_handler(app_state->platform_window_state, application_on_resize);
    platform_window_set_on_keyboard_key_handler(app_state->platform_window_state, application_on_keyboard_key);
    platform_window_set_on_mouse_move_handler(app_state->platform_window_state, application_on_mouse_move);
    platform_window_set_on_mouse_button_handler(app_state->platform_window_state, application_on_mouse_button);
    platform_window_set_on_mouse_wheel_handler(app_state->platform_window_state, application_on_mouse_wheel);
    platform_window_set_on_focus_handler(app_state->platform_window_state, application_on_focus);

    // Система визуализатора графики.
    renderer_system_initialize(&app_state->renderer_system_memory_requirement, null, null);
    app_state->renderer_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->renderer_system_memory_requirement);
    if(!renderer_system_initialize(&app_state->renderer_system_memory_requirement, app_state->renderer_system_state, app_state->platform_window_state))
    {
        kerror("Failed to initialize renderer. Aborted!");
        return false;
    }
    kinfor("Renderer system started.");

    // Система упавления текстурами.
    texture_system_config texture_sys_config;
    texture_sys_config.max_texture_count = 65536;
    texture_system_initialize(&app_state->texture_system_memory_requirement, null, &texture_sys_config);
    app_state->textute_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->texture_system_memory_requirement);
    if(!texture_system_initialize(&app_state->texture_system_memory_requirement, app_state->textute_system_state, &texture_sys_config))
    {
        kerror("Failed to initialize texture system. Aborted!");
        return false;
    }
    kinfor("Texture system started.");

    // Система управления материалами.
    material_system_config material_sys_config;
    material_sys_config.max_material_count = 4096;
    material_system_initialize(&app_state->material_system_memory_requirement, null, &material_sys_config);
    app_state->material_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->material_system_memory_requirement);
    // NOTE: Как я заебался искать повреждение памяти, проблема была в том, неправильно передал: &app_state->material_system_state!!!!!
    //       Случайно поставил символ '&'.
    // TODO: Переделать без использования void*!
    if(!material_system_initialize(&app_state->material_system_memory_requirement, app_state->material_system_state, &material_sys_config))
    {
        kerror("Failed to initialize material system. Aborted!");
        return false;
    }
    kinfor("Material system started.");

    // Инициализация приложения пользователя (игры, 3d приложения).
    if(!game_inst->initialize(game_inst))
    {
        kerror("Failed to initialize game. Aborted!");
        return false;
    }
    kinfor("Game initialized.");

    // Принудительное обновление размера окна.
    game_inst->on_resize(game_inst, game_inst->window_width, game_inst->window_height);

    return true;
}

bool application_run()
{
    if(!app_state)
    {
        kerror("Application context was not created. Call 'application_create' first.");
        return false;
    }

    app_state->is_running = true;

    clock_start(&app_state->clock);
    clock_update(&app_state->clock);
    app_state->last_time = app_state->clock.elapsed;

    f64 running_time     = 0;
    u16 frame_count      = 0;
    f64 frame_limit_time = 1.0f / 60;

    while(app_state->is_running)
    {
        if(!platform_window_dispatch(app_state->platform_window_state))
        {
            app_state->is_running = false;
        }

        if(!app_state->is_suspended)
        {
            // Обновляем таймер и получаем дельту!
            clock_update(&app_state->clock);
            f64 current_time = app_state->clock.elapsed;
            f64 delta = current_time - app_state->last_time;
            f64 frame_start_time = platform_time_absolute();
            
            if(!app_state->game_inst->update(app_state->game_inst, (f32)delta))
            {
                kerror("Game update failed, shutting down!");
                app_state->is_running = false;
                break;
            }

            // Пользовательский рендер.
            if(!app_state->game_inst->render(app_state->game_inst, (f32)delta))
            {
                kerror("Game render failed, shutting down!");
                app_state->is_running = false;
                break;
            }

            // TODO: Провести рефактор render_packet.
            render_packet packet;
            packet.delta_time = (f32)delta;
            if(!renderer_draw_frame(&packet))
            {
                kerror("Renderer failed draw frame, shutting down!");
                app_state->is_running = false;
                break;
            }

            // Расчет времени кадра.
            f64 frame_end_time = platform_time_absolute();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_secounds = frame_limit_time - frame_elapsed_time;

            if(remaining_secounds > 0)
            {
                u64 remaining_ms = remaining_secounds * 1000;

                // Если время еще не вышло, то возвращаем управление операционной системе!
                // TODO: Вынести лимитер кадров в настройки приложения.
                bool frame_limit_on = true;
                if(remaining_ms > 0 && frame_limit_on)
                {
                    platform_thread_sleep(remaining_ms - 1);
                }

                frame_count++;

            }

            // NOTE: Устройства ввода последнее что должно обновляться в кадре!
            input_system_update(delta);

            app_state->last_time = current_time;
        }
    }
    kinfor("Game stopped.");

    material_system_shutdown();
    kinfor("Material system stopped.");

    texture_system_shutdown();
    kinfor("Texture system stopped.");

    renderer_system_shutdown();
    kinfor("Renderer system stopped.");

    platform_window_destroy(app_state->platform_window_state);
    kinfor("Platform window destroyed.");

    input_system_shutdown();
    kinfor("Input system stopped.");

    event_system_shutdown();
    kinfor("Event system stopped.");

    memory_system_shutdown();
    kinfor("Memory system stopped.");

    linear_allocator_free_all(app_state->systems_allocator);
    linear_allocator_destroy(app_state->systems_allocator);
    app_state->systems_allocator = null;
    app_state->game_inst->application_state = null;

    kfree_tc(app_state, application_state, 1, MEMORY_TAG_APPLICATION);
    app_state = null;

    return true;
}

//////////////////////////////// EVENTS /////////////////////////////////

void application_on_focus(bool focused)
{
    // Stub!
}

void application_on_mouse_wheel(i32 zdelta)
{
    input_update_mouse_wheel(zdelta);    
}

void application_on_mouse_button(u32 button, bool pressed)
{
    input_update_mouse_button(button, pressed);
}

void application_on_mouse_move(i32 x, i32 y)
{
    input_update_mouse_move(x, y);
}

void application_on_keyboard_key(u32 keycode, bool pressed)
{
    if(keycode == KEY_Q && pressed) app_state->is_running = false;
    if(keycode == KEY_I && pressed)
    {
        const char* meminfo = memory_system_usage_str();
        kinfor(meminfo);
        platform_memory_free((void*)meminfo);
    }

    input_update_keyboard_key(keycode, pressed);
}

void application_on_resize(i32 width, i32 height)
{
    // Обновление размеров в приложения.
    app_state->game_inst->on_resize(app_state->game_inst, width, height);

    // Обновление размеров в визуализаторе.
    renderer_on_resize(width, height);

    // Создание события на обновление размеров.
    event_context context = { .i32[0] = width, .i32[1] = height };
    event_send(EVENT_CODE_APPLICATION_RESIZE, null, &context);
}

void application_on_close()
{
    app_state->is_running = false;
    event_send(EVENT_CODE_APPLICATION_QUIT, null, null);
}
