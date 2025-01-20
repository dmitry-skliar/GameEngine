#pragma once

#include <defines.h>

/*
    @brief Добавляет имена необходимых расширений для платформы.
    @param names Указатель на массив строк (используется darray).
*/
void platform_window_get_vulkan_extentions(const char*** names);
