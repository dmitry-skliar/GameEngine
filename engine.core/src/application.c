// Cобственные подключения.
#include "application.h"

// Внутренние подключения.
#include "logger.h"
#include "event.h"
#include "debug/assert.h"
#include "platform/window.h"
#include "platform/memory.h"
#include "memory/memory.h"

// Внешние подключения.

typedef struct application_context {
    application* application;
    bool is_running;
    bool is_suspended;
    f64  last_time;
} application_context;

static application_context* context = null;

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
        kerror("The window was not created! Aborted.");
        return false;
    }

    // Обработчики событий окна.
    platform_window_handler_close_set(application_on_close);
    platform_window_handler_resize_set(application_on_resize);
    platform_window_handler_keyboard_key_set(application_on_keyboard_key);
    platform_window_handler_mouse_move_set(application_on_mouse_move);

    // Инициализация подсистемы событий.
    event_system_initialize();

    // Регистрация обработчиков событий.

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
    event_system_shutdown();
    platform_window_destroy();
    kmfree(context);

    return true;
}

void application_on_mouse_move(i32 x, i32 y)
{
    event_context data = { .i32[0] = x, .i32[1] = y };
    event_send(EVENT_CODE_MOUSE_MOVED, null, data);
}

void application_on_keyboard_key(u32 keycode, bool pressed)
{
    if(keycode == 16 && pressed) context->is_running = false;
    if(keycode == 23 && pressed)
    {
        const char* meminfo = memory_system_usage_get();
        kinfor(meminfo);
        // TODO: Заменить на kmfree(meminfo);
        platform_memory_free((void*)meminfo);
    }

    event_context data = { .u32[0] = keycode };
    if(pressed)
    {
        event_send(EVENT_CODE_KEYBOARD_KEY_PRESSED, null, data);
    }
    else
    {
        event_send(EVENT_CODE_KEYBOARD_KEY_RELEASED, null, data);
    }
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
