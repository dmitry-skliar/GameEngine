// Cобственные подключения.
#include "renderer/vulkan/vulkan_backend.h"
#include "renderer/vulkan/vulkan_types.h"

// Внутренние подключения.
#include "logger.h"
#include "debug/assert.h"
#include "memory/memory.h"

// Указатель на контекст vulkan.
static vulkan_context* context = null;

// Сообщения.
static const char* message_context_not_initialized = "Vulkan renderer was not initialized. Please first call 'vulkan_renderer_backend_initialize'.";

bool vulkan_renderer_backend_initialize(renderer_backend* backend, const char* application_name)
{
    kassert_debug(context == null, "Trying to call function 'vulkan_renderer_backend_initialize' more than once!");

    context = kmallocate_t(vulkan_context, MEMORY_TAG_RENDERER);
    if(!context)
    {
        kerror("Failed to allocated memory for vulkan context. Aborted!");
        return false;
    }
    kmzero_tc(context, vulkan_context, 1);

    // Начальная инициализация.
    //

    // Настройка экземпляра vulkan.
    VkApplicationInfo appinfo        = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appinfo.apiVersion               = VK_API_VERSION_1_4;
    appinfo.pApplicationName         = application_name;
    appinfo.applicationVersion       = VK_MAKE_VERSION(1, 0, 0);
    appinfo.pEngineName              = "Copy Game Engine";
    appinfo.engineVersion            = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo instinfo    = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instinfo.pApplicationInfo        = &appinfo;
    instinfo.enabledExtensionCount   = 0;
    instinfo.ppEnabledExtensionNames = null;
    instinfo.enabledLayerCount       = 0;
    instinfo.ppEnabledLayerNames     = null;

    // TODO: Реализовать аллокатор памяти для vulkan.
    VkResult result = vkCreateInstance(&instinfo, context->allocator, &context->instance);
    if(result != VK_SUCCESS)
    {
        kerror("Failed to create vulkan instance with result: %u", result);
        return false;
    }

    kinfor("Renderer vulkan started.");
    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend)
{
    kassert_debug(context != null, message_context_not_initialized);

    // Уничтожение экземпляра vulkan.
    vkDestroyInstance(context->instance, context->allocator);
    context->instance = null;

    // Уничтожение контекста vulkan.
    kmfree(context);
    context = null;
    kinfor("Renderer vulkan stopped.");
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
