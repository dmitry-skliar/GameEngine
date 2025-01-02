// Cобственные подключения.
#include "application.h"

// Внутренние подключения.
#include "logger.h"
#include "debug/assert.h"
#include "platform/window.h"
#include "platform/memory.h"
#include "memory/memory.h"

// Внешние подключения.

typedef struct application_context {
    game* game;
    bool is_running;
    bool is_suspended;
    f64  last_time;
} application_context;

static application_context* context = null;

void application_on_keyboard_key(u32 keycode, bool pressed);
void application_on_resize(i32 width, i32 height);
void application_on_close();

bool application_create(game* game)
{
    // Проверка вызова функции.
    kassert_debug(context == null, "Trying to call function 'application_create' more than once!");
    kassert_debug(game != null, "The 'application_create' function requires a game structure!");

    context = kmallocate_t(application_context, MEMORY_TAG_APPLICATION);
    if(!context)
    {
        kerror("Memory for application context not allocated! Aborted.");
        return false;
    }
    kmzero_tc(context, application_context, 1);

    context->game = game;

    // Инициализация подсистем.
    window_config config = { game->window_title, game->window_width, game->window_height };
    if(!platform_window_create(&config))
    {
        kerror("The window was not created! Aborted.");
        return false;
    }

    // Обработчики событий окна.
    platform_window_handler_close_set(application_on_close);
    platform_window_handler_resize_set(application_on_resize);
    platform_window_handler_keyboard_key_set(application_on_keyboard_key);

    // Инициализация игры.
    if(!game->initialize(game))
    {
        kerror("Game failed to initialize.");
        return false;
    }

    // Принудительное обновление размера окна.
    game->on_resize(game, game->window_width, game->window_height);

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
            if(!context->game->update(context->game, (f32)0))
            {
                kerror("Game update failed, shutting down!");
                context->is_running = false;
                break;
            }

            if(!context->game->render(context->game, (f32)0))
            {
                kerror("Game render failed, shutting down!");
                context->is_running = false;
                break;
            }
        }
    }

    kmfree(context);

    platform_window_destroy();

    return true;
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
}

void application_on_resize(i32 width, i32 height)
{
    context->game->on_resize(context->game, width, height);
}

void application_on_close()
{
    context->is_running = false;
}
