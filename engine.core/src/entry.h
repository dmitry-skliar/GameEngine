#pragma once

#include <defines.h>
#include <logger.h>
#include <application.h>

// Внешняя функция для создания игры.
extern bool create_game(struct game* inst);

int main()
{
    game inst = {0};

    if(!create_game(&inst))
    {
        KERROR("Could not create game!");
        return -1;
    }

    if(!inst.initialize || !inst.update || !inst.render || !inst.on_resize)
    {
        KERROR("The application's function pointers must be assigned!");
        return -2;
    }

    if(!inst.window_width || !inst.window_height || !inst.window_title)
    {
        KERROR("The application configuration requires the window width, height, and title.");
        return -3;
    }

    if(!application_create(&inst))
    {
        KERROR("The application failed to create!");
        return 1;
    }

    if(!application_run())
    {
        KERROR("The application didn't shutdown gracefully.");
        return 2;
    }

    return 0;
}
