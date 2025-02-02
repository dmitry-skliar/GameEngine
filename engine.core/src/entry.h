#pragma once

#include <logger.h>
#include <application.h>

// @brief Внешняя функция для создания игры.
extern bool game_create(game* inst);
extern void game_destroy(game* inst);

int main()
{
    game game_inst = {0};

    if(!game_create(&game_inst))
    {
        kerror("Could not create game!");
        return -1;
    }

    if(!game_inst.initialize || !game_inst.update || !game_inst.render || !game_inst.on_resize)
    {
        kerror("The application's function pointers must be assigned!");
        return -2;
    }

    if(!game_inst.window_width || !game_inst.window_height || !game_inst.window_title)
    {
        kerror("The application configuration requires the window width, height, and title.");
        return -3;
    }

    if(!application_create(&game_inst))
    {
        kerror("Failed to create application!");
        return 1;
    }

    if(!application_run())
    {
        kerror("The application didn't shutdown gracefully.");
        return 2;
    }

    game_destroy(&game_inst);

    return 0;
}
