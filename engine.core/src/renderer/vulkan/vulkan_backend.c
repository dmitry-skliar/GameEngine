// Cобственные подключения.
#include "renderer/vulkan/vulkan_backend.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_platform.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_command_buffer.h"

// Внутренние подключения.
#include "logger.h"
#include "debug/assert.h"
#include "memory/memory.h"
#include "containers/darray.h"
#include "kstring.h"

// Указатель на контекст Vulkan.
static vulkan_context* context = null;

// Сообщения.
static const char* message_context_not_initialized = "Vulkan renderer was not initialized. Please first call 'vulkan_renderer_backend_initialize'.";

// Объявления функций.
VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_message_handler(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data
);
i32 find_memory_index(u32 type_filter, u32 property_flags);
void command_buffers_create(renderer_backend* backend);
void command_buffers_destroy(renderer_backend* backend);

bool vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name)
{
    kassert_debug(context == null, "Trying to call function 'vulkan_renderer_backend_initialize' more than once!");

    context = kmallocate_t(vulkan_context, MEMORY_TAG_RENDERER);
    if(!context)
    {
        kerror("Failed to allocated memory for vulkan context.");
        return false;
    }
    kmzero_tc(context, vulkan_context, 1);
    ktrace("Vulkan context created.");

    // Начальная инициализация.
    context->find_memory_index = find_memory_index;
    // TODO: Сделать аллокатор.
    context->allocator = null;

    // Заполнение информации приложения.
    VkApplicationInfo appinfo        = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appinfo.apiVersion               = VK_API_VERSION_1_4;
    appinfo.pApplicationName         = application_name;
    appinfo.applicationVersion       = VK_MAKE_VERSION(1, 0, 0);
    appinfo.pEngineName              = "Game Engine";
    appinfo.engineVersion            = VK_MAKE_VERSION(0, 1, 0);

    // TODO: Переделать! Вынести в отдельный файл.
    ktrace("Vulkan API version: %d.%d.%d", VK_VERSION_MAJOR(appinfo.apiVersion), VK_VERSION_MINOR(appinfo.apiVersion), VK_VERSION_PATCH(appinfo.apiVersion));
    ktrace("Engine API version: %d.%d.%d", VK_VERSION_MAJOR(appinfo.engineVersion), VK_VERSION_MINOR(appinfo.engineVersion), VK_VERSION_PATCH(appinfo.engineVersion));

    // Настройка экземпляра vulkan.
    VkInstanceCreateInfo instinfo    = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instinfo.pApplicationInfo        = &appinfo;

    // Получение списока запрашиваемых расширений.
    const char** required_extentions = darray_create(const char*);
    darray_push(required_extentions, &VK_KHR_SURFACE_EXTENSION_NAME);     // Расширение для использования поверхности.
    platform_window_get_vulkan_extentions(&required_extentions);          // Платформо-зависимые расширения.
#if KDEBUG_FLAG
    darray_push(required_extentions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Расширение для отладки.
#endif

    // Получение списока доступных расширений.
    u32 available_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(null, &available_extension_count, null);
    VkExtensionProperties* avaulable_extensions = darray_reserve(VkExtensionProperties, available_extension_count);
    vkEnumerateInstanceExtensionProperties(null, &available_extension_count, avaulable_extensions);

    // Сопоставление запрашиваемых и доступных расширений.
    u32 required_extension_count = darray_get_length(required_extentions);
    ktrace("Vulkan extensions used:");
    for(u32 i = 0; i < required_extension_count; ++i)
    {
        bool found = false;

        for(u32 j = 0; j < available_extension_count; ++j)
        {
            if(string_is_equal(required_extentions[i], avaulable_extensions[j].extensionName))
            {
                ktrace(" * %s", required_extentions[i]);
                found = true;
                break;
            }
        }

        if(!found)
        {
            kerror("Required extension: '%s' is missing.", required_extentions[i]);
            return false;
        }
    }

    // Запись запрашиваемых расширений.
    instinfo.enabledExtensionCount   = required_extension_count;
    instinfo.ppEnabledExtensionNames = required_extentions;
    instinfo.enabledLayerCount       = 0;
    instinfo.ppEnabledLayerNames     = null;

#if KDEBUG_FLAG
    // Получение списка запрашиваемых слоев проверки.
    const char** required_layers = darray_create(const char*);
    darray_push(required_layers, &"VK_LAYER_KHRONOS_validation");          // Проверка правильности использования Vulkan API.
    // darray_push(required_layers, &"VK_LAYER_LUNARG_api_dump");             // Выводит в консоль вызовы и передаваемые параметры.

    // Получение списка доступных слоев проверки.
    u32 available_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&available_layer_count, null);
    VkLayerProperties* available_layers = darray_reserve(VkLayerProperties, available_layer_count);
    vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);

    // Сопоставление запрашиваемых и доступных слоев проверки.
    u32 required_layer_count = darray_get_length(required_layers);
    ktrace("Vulkan validation layers used:");
    for(u32 i = 0; i < required_layer_count; ++i)
    {
        bool found = false;

        for(u32 j = 0; j < available_layer_count; ++j)
        {
            if(string_is_equal(required_layers[i], available_layers[j].layerName))
            {
                ktrace(" * %s", required_layers[i]);
                found = true;
                break;
            }
        }

        if(!found)
        {
            kerror("Required validation layer: '%s' is missing.", required_layers[i]);
            return false;
        }
    }

    // Запись запрашиваемых слоев проверки.
    instinfo.enabledLayerCount   = required_layer_count;
    instinfo.ppEnabledLayerNames = required_layers;
#endif

    // TODO: Реализовать аллокатор памяти для vulkan.
    VkResult result = vkCreateInstance(&instinfo, context->allocator, &context->instance);
    if(!vulkan_result_is_success(result))
    {
        kerror("Failed to create vulkan instance with result: %s", vulkan_result_get_string(result, true));
        return false;
    }
    ktrace("Vulkan instance created.");

    // Очистка используемой памяти.
    darray_destroy(required_extentions);
    darray_destroy(avaulable_extensions);
#if KDEBUG_FLAG
    darray_destroy(required_layers);
    darray_destroy(available_layers);
#endif

    // Создание отладочного мессенджара Vulkan.
#if KDEBUG_FLAG
    VkDebugUtilsMessengerCreateInfoEXT msginfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    msginfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                            // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                            // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    msginfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
    msginfo.pfnUserCallback = vulkan_debug_message_handler;

    PFN_vkCreateDebugUtilsMessengerEXT messenger_create =
        (void*)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");

    result = messenger_create(context->instance, &msginfo, context->allocator, &context->debug_messenger);
    if(!vulkan_result_is_success(result))
    {
        kerror("Failed to create vulkan debug messenger with result: %s", vulkan_result_get_string(result, true));
        return false;
    }
    ktrace("Vulkan debug messenger created.");
#endif

    // Создание поверхности Vulkan-Platform.
    result = platform_window_create_vulkan_surface(context);
    if(!vulkan_result_is_success(result))
    {
        kerror("Failed to create vulkan surface with result: %s", vulkan_result_get_string(result, true));
        return false;
    }
    ktrace("Vulkan surfcae created.");

    // Создание физического и логического устройства.
    // TODO: Провести переделку логики vulkan_device_*!
    result = vulkan_device_create(context);
    if(!vulkan_result_is_success(result))
    {
        kerror("Failed to create vulkan device with result: %s", vulkan_result_get_string(result, true));
        return false;
    }
    ktrace("Vulkan device created.");

    // Создание цепочки обмена.
    vulkan_swapchain_create(context, context->framebuffer_width, context->framebuffer_height, &context->swapchain);
    ktrace("Vulkan swapchain created.");

    // Создание визуализатора.
    vulkan_renderpass_create(
        context, &context->main_renderpass, 0, 0, context->framebuffer_width, context->framebuffer_height,
        0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0
    );
    ktrace("Vulkan renderpass created.");

    // Создание буферов команд.
    command_buffers_create(backend);
    ktrace("Vulkan command buffers created (Now only graphics!).");

    kinfor("Renderer started.");
    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend)
{
    kassert_debug(context != null, message_context_not_initialized);

    vkDeviceWaitIdle(context->device.logical);

    // Уничтожение буферов команд.
    command_buffers_destroy(backend);
    ktrace("Vulkan command buffers destroyed.");

    // Уничтожение визуализатора.
    vulkan_renderpass_destroy(context, &context->main_renderpass);
    ktrace("Vulkan renderpass destroyed.");

    // Уничтожение цепочки обмена.
    vulkan_swapchain_destroy(context, &context->swapchain);
    ktrace("Vulkan swapchain destroyed.");

    // Уничтожение физического и логического устройства.
    if(context->device.logical)
    {
        vulkan_device_destroy(context);
        ktrace("Vulkan device destroyed.");
    }

    // Уничтожение поверхности Vulkan-Platform.
    if(context->surface)
    {
        platform_window_destroy_vulkan_surface(context);
        ktrace("Vulkan surface destroyed.");
    }

    // Уничтожение отладочного мессенджара Vulkan.
    if(context->debug_messenger)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT messenger_destroy =
            (void*)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");

        messenger_destroy(context->instance, context->debug_messenger, context->allocator);
        context->debug_messenger = null;
        ktrace("Vulkan debug messenger destroyed.");
    }

    // Уничтожение экземпляра vulkan.
    if(context->instance)
    {
        vkDestroyInstance(context->instance, context->allocator);
        context->instance = null;
        ktrace("Vulkan instance destroyed.");
    }

    // Уничтожение контекста vulkan.
    if(context)
    {
        kmfree(context);
        context = null;
        ktrace("Vulkan context destroyed.");
    }

    kinfor("Renderer stopped.");
}

void vulkan_renderer_backend_on_resized(renderer_backend* backend, i32 width, i32 height)
{
    
}

bool vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time)
{
    return true;    
}

bool vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time)
{
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_message_handler(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data)
{
    switch(severity)
    {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            kerror(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            kwarng(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            kinfor(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            ktrace(callback_data->pMessage);
            break;
    }
    return VK_FALSE;
}

i32 find_memory_index(u32 type_filter, u32 property_flags)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context->device.physical, &memory_properties);

    for(u32 i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        u32 current_property_flags = memory_properties.memoryTypes[i].propertyFlags & property_flags;

        // Проверка каждого типа памяти на наличие установленного бита.
        if(type_filter & (1 << i) && current_property_flags == property_flags)
        {
            return i;
        }
    }

    kwarng("Unable to find suitable memory type!");
    return -1;
}

void command_buffers_create(renderer_backend* backend)
{
    if(!context->graphics_command_buffers)
    {
        context->graphics_command_buffers = darray_reserve(vulkan_command_buffer, context->swapchain.image_count);
        kmzero_tc(context->graphics_command_buffers, vulkan_command_buffer, context->swapchain.image_count);
    }

    for(u32 i = 0; i < context->swapchain.image_count; ++i)
    {
        if(context->graphics_command_buffers[i].handle)
        {
            vulkan_command_buffer_free(context, context->device.graphics_queue.command_pool, &context->graphics_command_buffers[i]);
        }

        vulkan_command_buffer_allocate(context, context->device.graphics_queue.command_pool, true, &context->graphics_command_buffers[i]);
    }
}

void command_buffers_destroy(renderer_backend* backend)
{
    if(context->graphics_command_buffers)
    {
        for(u32 i = 0; i < context->swapchain.image_count; ++i)
        {
            // TODO: Все похожие проверки спрятать внутри функции, дабы не создавать лишних проблем!
            if(context->graphics_command_buffers[i].handle)
            {
                vulkan_command_buffer_free(context, context->device.graphics_queue.command_pool, &context->graphics_command_buffers[i]);
            }
        }

        darray_destroy(context->graphics_command_buffers);
        context->graphics_command_buffers = null;
    }
}
