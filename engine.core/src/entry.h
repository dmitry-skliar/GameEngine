#pragma once

#include <logger.h>
#include <application.h>
#include <memory/memory.h>

// Внешняя функция для создания игры.
extern bool create_game(struct game* inst);

int main()
{
    memory_system_initialize();

    game* inst = kmallocate_t(game, MEMORY_TAG_GAME);
    kmzero_tc(inst, game, 1);

    if(!create_game(inst))
    {
        kerror("Could not create game!");
        return -1;
    }

    if(!inst->initialize || !inst->update || !inst->render || !inst->on_resize)
    {
        kerror("The application's function pointers must be assigned!");
        return -2;
    }

    if(!inst->window_width || !inst->window_height || !inst->window_title)
    {
        kerror("The application configuration requires the window width, height, and title.");
        return -3;
    }

    if(!application_create(inst))
    {
        kerror("The application failed to create!");
        return 1;
    }

    if(!application_run())
    {
        kerror("The application didn't shutdown gracefully.");
        return 2;
    }

    kmfree(inst->state);
    kmfree(inst);

    memory_system_shutdown();

    return 0;
}
