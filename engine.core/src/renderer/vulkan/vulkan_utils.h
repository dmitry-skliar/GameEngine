#pragma once

#include <defines.h>
#include <renderer/vulkan/vulkan_types.h>

/*
    @brief Возвращает строковое представление значения кода VkResult (только для Vulkan).
    NOTE: По умолчанию для неизвестного значения кода VkResult используется VK_SUCCESS.
    @param result Значение кода VkResult.
    @param extended Указывает, следует ли возвращать расширеное представление об ошибке.
    @return Строковое представление об ошибке и/или расширенное представление об ошибке.
*/
const char* vulkan_result_get_string(VkResult result, bool extended);

/*
    @brief Указывает, является ли значение кода VkResult ошибочным или успешным, в
           соответствии со спецификацией Vulkan spec.
    NOTE: По умолчанию для неизвестного значения кода VkResult возвращает false.
    @param result Значение кода VkResult.
    @return True - при успешном или false - при ошибочном значении кода VkResult.
*/
bool vulkan_result_is_success(VkResult result);
