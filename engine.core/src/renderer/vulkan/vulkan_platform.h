#pragma once

#include <defines.h>
#include <renderer/vulkan/vulkan_types.h>

/*
    @brief Добавляет имена необходимых расширений для платформы.
    @param names Указатель на массив строк (используется darray).
*/
void platform_window_get_vulkan_extentions(const char*** names);

/*
    @brief Создает поверхность Vulkan-Platform.
    @param context Контекст Vulkan.
    @return Значение кода VkResult.
*/
VkResult platform_window_create_vulkan_surface(vulkan_context* context);

/*
    @brief Уничтожает поверхность Vulkan-Platform.
    @param context Контекст Vulkan.
*/
void platform_window_destroy_vulkan_surface(vulkan_context* context);

/*
    @brief Указывает поддерживает ли семейство очередей представление поверхности платформой.
    @param context Контекст Vulkan.
    @param physical_device Указатель на физическое устройство.
    @param queue_family_index Индекс семейства очередей.
    @return True - поддержка присутствует, false - отсутствует.
*/
bool platform_window_get_vulkan_presentation_support(vulkan_context* context, VkPhysicalDevice physical_device, u32 queue_family_index);
