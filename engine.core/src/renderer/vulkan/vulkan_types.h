#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <vulkan/vulkan.h>

// TODO: Так же сделать выбор по индексу видеокарты!
typedef struct vulkan_device_requirements {
    // @brief Поддержка определенного типа физического устройства.
    VkPhysicalDeviceType device_type;
    // @brief Поддержка расширений физического устройства (используется массив darray).
    const char** extensions;
    // @brief Поддержка анизотропии.
    bool sampler_anisotropy;
} vulkan_device_requirements;

typedef struct vulkan_device_queue {
    // @brief Указатель на очередь.
    VkQueue handle;
    // @brief Индекс семейства очередей.
    u32 index;
    // @brief Количество очередей в семействе.
    u32 count;
} vulkan_device_queue;

// TODO: Вместо массивов darray использовать array.
typedef struct vulkan_device_swapchain_support {
    // @brief Флаги возможностей.
    VkSurfaceCapabilitiesKHR capabilities;
    // @brief Доступное количество форматов. 
    u32 format_count;
    // @brief Форматы данных (используется массив darray, для получения размера использовать darray_get_capacity).
    VkSurfaceFormatKHR* formats;
    // @brief Доступное количество режимов.
    u32 present_mode_count;
    // @brief Режимы изображений показа (используется массив darray, для получения размера использовать darray_get_capacity).
    VkPresentModeKHR* present_modes;
} vulkan_device_swapchain_support;

typedef struct vulkan_device {
    // @brief Физическое устройство.
    VkPhysicalDevice physical;
    // @brief Логическое устройство.
    VkDevice logical;
    // @brief Поддерживаемые опции цепочки обмена.
    vulkan_device_swapchain_support swapchain_support;
    // @brief Очередь графики.
    vulkan_device_queue graphics_queue;
    // @brief Очередь показа.
    vulkan_device_queue present_queue;
    // @brief Очередь операций с памятью.
    vulkan_device_queue transfer_queue;
    // @brief Очередь вычислений (не используется).
    vulkan_device_queue compute_queue;
    // @brief Парамерты устройства.
    VkPhysicalDeviceProperties properties;
    // @brief Функции устройства.
    VkPhysicalDeviceFeatures features;
    // @brief Память устройства.
    VkPhysicalDeviceMemoryProperties memory;
    // @brief Поддержка прямой видимости памяти устройства.
    bool memory_local_host_visible_support;
} vulkan_device;

typedef struct vulkan_context {
    VkInstance               instance;
    VkAllocationCallbacks*   allocator;
    VkSurfaceKHR             surface;
    vulkan_device            device;
#if KDEBUG_FLAG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} vulkan_context;
