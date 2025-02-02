#pragma once

#include <defines.h>
#include <platform/window.h>
#include <renderer/vulkan/vulkan_types.h>

/*
    @brief Добавляет имена необходимых расширений для платформы.
    @param instance Указатель на выделенную память экземпляра окна.
    @param names Указатель на массив строк (используется darray).
*/
void platform_window_get_vulkan_extentions(window* instance, const char*** names);

/*
    @brief Создает поверхность Vulkan-Platform.
    @param instance Указатель на выделенную память экземпляра окна.
    @param context Контекст Vulkan.
    @return Значение кода VkResult.
*/
VkResult platform_window_create_vulkan_surface(window* instance, vulkan_context* context);

/*
    @brief Уничтожает поверхность Vulkan-Platform.
    @param instance Указатель на выделенную память экземпляра окна.
    @param context Контекст Vulkan.
*/
void platform_window_destroy_vulkan_surface(window* instance, vulkan_context* context);

/*
    @brief Указывает поддерживает ли семейство очередей представление поверхности платформой.
    @param instance Указатель на выделенную память экземпляра окна.
    @param physical_device Указатель на физическое устройство.
    @param queue_family_index Индекс семейства очередей.
    @return True поддержка присутствует, false отсутствует.
*/
bool platform_window_get_vulkan_presentation_support(window* instance, VkPhysicalDevice physical_device, u32 queue_family_index);
