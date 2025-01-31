// Cобственные подключения.
#include "application.h"

// Внутренние подключения.
#include "logger.h"
#include "event.h"
#include "debug/assert.h"
#include "platform/window.h"
#include "platform/memory.h"
#include "platform/time.h"
#include "platform/thread.h"
#include "memory/memory.h"
#include "input.h"
#include "clock.h"
#include "renderer/renderer_frontend.h"

typedef struct application_context {
    application* application;
    bool  is_running;
    bool  is_suspended;
    clock clock;
    f64   last_time;
} application_context;

static application_context* context = null;

void application_on_focus(bool focused);
void application_on_mouse_wheel(i32 zdelta);
void application_on_mouse_button(u32 button, bool pressed);
void application_on_mouse_move(i32 x, i32 y);
void application_on_keyboard_key(u32 keycode, bool pressed);
void application_on_resize(i32 width, i32 height);
void application_on_close();

bool application_create(application* application)
{
    kassert_debug(context == null, "Trying to call function 'application_create' more than once!");

    if(!application)
    {
        kerror("Function '%s' requires an application structure!", __FUNCTION__);
        return false;
    }

    context = kmallocate_t(application_context, MEMORY_TAG_APPLICATION);
    if(!context)
    {
        kerror("Failed to allocated memory for application context. Aborted!");
        return false;
    }
    kmzero_tc(context, application_context, 1);

    // Начальная инициализация.
    context->application = application;

    // Инициализация подсистемы окна.
    window_config config = { application->window_title, application->window_width, application->window_height };
    if(!platform_window_create(&config))
    {
        kerror("Failed to initialize platform window. Aborted!");
        return false;
    }

    // Обработчики событий окна.
    platform_window_set_on_close_handler(application_on_close);
    platform_window_set_on_resize_handler(application_on_resize);
    platform_window_set_on_keyboard_key_handler(application_on_keyboard_key);
    platform_window_set_on_mouse_move_handler(application_on_mouse_move);
    platform_window_set_on_mouse_button_handler(application_on_mouse_button);
    platform_window_set_on_mouse_wheel_handler(application_on_mouse_wheel);
    platform_window_set_on_focus_handler(application_on_focus);

    // Инициализация подсистемы событий.
    if(!event_system_initialize())
    {
        kerror("Failed to initialize event system. Aborted!");
        return false;
    }

    // Инициализация подсистемы ввода.
    if(!input_system_initialize())
    {
        kerror("Failed to initialize input system. Aborted!");
        return false;
    }

    // Инициализация редререра.
    if(!renderer_initialize(application->window_title, application->window_width, application->window_height))
    {
        kerror("Failed to initialize renderer. Aborted!");
        return false;
    }

    // Инициализация приложения пользователя (игры, 3d приложения).
    if(!application->initialize(application))
    {
        kerror("Failed to initialize user application. Aborted!");
        return false;
    }

    // Принудительное обновление размера окна.
    application->on_resize(application, application->window_width, application->window_height);

    return true;
}

bool application_run()
{
    // Проверка вызова функции.
    kassert_debug(context != null, "Application context was not created. Please first call 'application_create'.");

    context->is_running = true;

    clock_start(&context->clock);
    clock_update(&context->clock);
    context->last_time = context->clock.elapsed;

    f64 running_time     = 0;
    u16 frame_count      = 0;
    f64 frame_limit_time = 1.0f / 60;

    while(context->is_running)
    {
        if(!platform_window_dispatch())
        {
            context->is_running = false;
        }

        if(!context->is_suspended)
        {
            // Обновляем таймер и получаем дельту!
            clock_update(&context->clock);
            f64 current_time = context->clock.elapsed;
            f64 delta = current_time - context->last_time;
            f64 frame_start_time = platform_time_get_absolute();
            
            if(!context->application->update(context->application, (f32)delta))
            {
                kerror("Game update failed, shutting down!");
                context->is_running = false;
                break;
            }

            // Пользовательский рендер.
            if(!context->application->render(context->application, (f32)delta))
            {
                kerror("Game render failed, shutting down!");
                context->is_running = false;
                break;
            }

            // TODO: Провести рефактор render_packet.
            render_packet packet;
            packet.delta_time = (f32)delta;
            renderer_draw_frame(&packet);

            // Расчет времени кадра.
            f64 frame_end_time = platform_time_get_absolute();
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

            context->last_time = current_time;
        }
    }

    // Нормальное завершение работы.
    renderer_shutdown();
    input_system_shutdown();
    event_system_shutdown();
    platform_window_destroy();
    kmfree(context);
    context = null;

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
    if(keycode == KEY_Q && pressed) context->is_running = false;
    if(keycode == KEY_I && pressed)
    {
        const char* meminfo = memory_system_usage_get();
        kinfor(meminfo);
        platform_memory_free((void*)meminfo);
    }

    input_update_keyboard_key(keycode, pressed);
}

void application_on_resize(i32 width, i32 height)
{
    // Обновление размеров в приложения.
    context->application->on_resize(context->application, width, height);

    // Обновление размеров в визуализаторе.
    renderer_on_resize(width, height);

    // Создание события на обновление размеров.
    event_context data = { .i32[0] = width, .i32[1] = height };
    event_send(EVENT_CODE_APPLICATION_RESIZE, null, data);
}

void application_on_close()
{
    context->is_running = false;

    event_context data = {0};
    event_send(EVENT_CODE_APPLICATION_QUIT, null, data);
}
