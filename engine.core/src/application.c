// Cобственные подключения.
#include "application.h"

// Внутренние подключения.
#include "logger.h"
#include "event.h"
#include "debug/assert.h"
#include "platform/window.h"
#include "platform/memory.h"
#include "memory/memory.h"
#include "input.h"

typedef struct application_context {
    application* application;
    bool is_running;
    bool is_suspended;
    f64  last_time;
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
        kerror("Memory for application context not allocated! Aborted.");
        return false;
    }
    kmzero_tc(context, application_context, 1);

    context->application = application;

    // Инициализация подсистемы окна.
    window_config config = { application->window_title, application->window_width, application->window_height };
    if(!platform_window_create(&config))
    {
        kerror("The window was not created. Application aborted!");
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
        kerror("The event system was not initialized. Application aborted!");
        return false;
    }

    // Инициализация подсистемы ввода.
    if(!input_system_initialize())
    {
        kerror("The input system was not initialized. Application aborted!");
        return false;
    }

    // Инициализация игры.
    if(!application->initialize(application))
    {
        kerror("Game failed to initialize.");
        return false;
    }

    // Принудительное обновление размера окна.
    application->on_resize(application, application->window_width, application->window_height);

    context->is_running = true;

    return true;
}

bool application_run()
{
    // Проверка вызова функции.
    kassert_debug(context != null, "Application context was not created. Please first call 'application_create'.");

    while(context->is_running)
    {
        if(!platform_window_dispatch())
        {
            context->is_running = false;
        }

        if(!context->is_suspended)
        {
            if(!context->application->update(context->application, (f32)0))
            {
                kerror("Game update failed, shutting down!");
                context->is_running = false;
                break;
            }

            if(!context->application->render(context->application, (f32)0))
            {
                kerror("Game render failed, shutting down!");
                context->is_running = false;
                break;
            }
        }
    }

    // Нормальное завершение работы.
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
    context->application->on_resize(context->application, width, height);

    event_context data = { .i32[0] = width, .i32[1] = height };
    event_send(EVENT_CODE_APPLICATION_RESIZE, null, data);
}

void application_on_close()
{
    context->is_running = false;

    event_context data = {0};
    event_send(EVENT_CODE_APPLICATION_QUIT, null, data);
}
