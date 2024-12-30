#include "application.h"

#include "logger.h"
#include "debug/assert.h"
#include "platform/memory.h"
#include "platform/window.h"

typedef struct application_context {
    game* game;
    bool is_running;
    bool is_suspended;
    f64  last_time;
} application_context;

static application_context* context = null;

void application_on_close()
{
    context->is_running = false;
}

bool application_create(game* game)
{
    // Проверка вызова функции.
    KASSERT_DEBUG(context == null, "Trying to call function 'application_create' more than once!");
    KASSERT_DEBUG(game != null, "The 'application_create' function requires a game structure!");

    context = platform_memory_allocate(sizeof(application_context));
    if(!context)
    {
        KERROR("Memory for application context not allocated! Aborted.");
        return false;
    }
    platform_memory_zero(context, sizeof(application_context));

    context->game = game;

    // Инициализация подсистем.
    window_config config = { game->window_title, game->window_width, game->window_height };
    if(!platform_window_create(&config))
    {
        KERROR("The window was not created! Aborted.");
        return false;
    }

    // Обработчики событий окна.
    platform_window_handler_close_set(application_on_close);

    // Инициализация игры.
    if(!game->initialize(game))
    {
        KERROR("Game failed to initialize.");
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
    KASSERT_DEBUG(context != null, "Application context was not created. Please first call 'application_create'.");

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
                KERROR("Game update failed, shutting down!");
                context->is_running = false;
                break;
            }

            if(!context->game->render(context->game, (f32)0))
            {
                KERROR("Game render failed, shutting down!");
                context->is_running = false;
                break;
            }
        }
    }

    platform_window_destroy();

    return true;
}
