// Cобственные подключения.
#include "renderer/vulkan/vulkan_backend.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_platform.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_command_buffer.h"
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_fence.h"
#include "renderer/vulkan/shaders/vulkan_material_shader.h"
#include "renderer/vulkan/vulkan_buffer.h"

// Внутренние подключения.
#include "logger.h"
#include "debug/assert.h"
#include "memory/memory.h"
#include "containers/darray.h"
#include "kstring.h"
#include "math/math_types.h"

static vulkan_context* context = null;

static u32 cached_framebuffer_width  = 0;
static u32 cached_framebuffer_height = 0;

static const char* message_context_not_initialized = "Vulkan renderer was not initialized. Please first call 'vulkan_renderer_backend_initialize'.";

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_message_handler(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data
);
i32 find_memory_index(u32 type_filter, u32 property_flags);
void command_buffers_create(renderer_backend* backend);
void command_buffers_destroy(renderer_backend* backend);
void framebuffers_create(renderer_backend* backend, vulkan_swapchain* swapchain);
void framebuffers_regenerate(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass);
void framebuffers_destroy(renderer_backend* backend, vulkan_swapchain* swapchain);
void sync_objects_create();
void sync_objects_destroy();
bool swapchain_recreate(renderer_backend* backend);
bool buffers_create(vulkan_context* context);
void buffers_destroy(vulkan_context* context);

bool vulkan_renderer_backend_initialize(renderer_backend* backend)
{
    kassert_debug(context == null, "Trying to call function 'vulkan_renderer_backend_initialize' more than once!");

    context = kallocate_tc(vulkan_context, 1, MEMORY_TAG_RENDERER);
    if(!context)
    {
        kerror("Failed to allocated memory for vulkan context.");
        return false;
    }
    kzero_tc(context, vulkan_context, 1);

    // Начальная инициализация.
    context->find_memory_index = find_memory_index;
    // TODO: Сделать аллокатор.
    context->allocator = null;

    cached_framebuffer_width = backend->window_state->width;
    cached_framebuffer_height = backend->window_state->height;
    context->framebuffer_width = (cached_framebuffer_width != 0) ? cached_framebuffer_width : 800;
    context->framebuffer_height = (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;
    kdebug("Vulkan initialize framebuffer (w/h): %d / %d", context->framebuffer_width, context->framebuffer_height);
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    // Заполнение информации приложения.
    VkApplicationInfo appinfo        = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appinfo.apiVersion               = VK_API_VERSION_1_4;
    appinfo.pApplicationName         = backend->window_state->title;
    appinfo.applicationVersion       = VK_MAKE_VERSION(1, 0, 0);
    appinfo.pEngineName              = "Game Engine";
    appinfo.engineVersion            = VK_MAKE_VERSION(0, 1, 0);

    u32 app_major = VK_VERSION_MAJOR(appinfo.apiVersion);
    u32 app_minor = VK_VERSION_MINOR(appinfo.apiVersion);
    u32 app_patch = VK_VERSION_PATCH(appinfo.apiVersion);

    u32 eng_major = VK_VERSION_MAJOR(appinfo.engineVersion);
    u32 eng_minor = VK_VERSION_MINOR(appinfo.engineVersion);
    u32 eng_patch = VK_VERSION_PATCH(appinfo.engineVersion);

    // TODO: Переделать! Вынести в отдельный файл.
    ktrace("Vulkan API version: %d.%d.%d", app_major, app_minor, app_patch);
    ktrace("Engine API version: %d.%d.%d", eng_major, eng_minor, eng_patch);

    // Настройка экземпляра vulkan.
    VkInstanceCreateInfo instinfo    = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instinfo.pApplicationInfo        = &appinfo;

    // Получение списока запрашиваемых расширений.
    const char** required_extentions = darray_create(const char*);
    darray_push(required_extentions, &VK_KHR_SURFACE_EXTENSION_NAME);                    // Расширение для использования поверхности.
    platform_window_get_vulkan_extentions(backend->window_state, &required_extentions);  // Платформо-зависимые расширения.
#if KDEBUG_FLAG
    darray_push(required_extentions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Расширение для отладки.
#endif

    // Получение списока доступных расширений.
    u32 available_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(null, &available_extension_count, null);
    VkExtensionProperties* avaulable_extensions = darray_reserve(VkExtensionProperties, available_extension_count);
    vkEnumerateInstanceExtensionProperties(null, &available_extension_count, avaulable_extensions);

    // Сопоставление запрашиваемых и доступных расширений.
    u32 required_extension_count = darray_length(required_extentions);
    ktrace("Vulkan extensions used:");
    for(u32 i = 0; i < required_extension_count; ++i)
    {
        bool found = false;

        for(u32 j = 0; j < available_extension_count; ++j)
        {
            if(string_equal(required_extentions[i], avaulable_extensions[j].extensionName))
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
    u32 required_layer_count = darray_length(required_layers);
    ktrace("Vulkan validation layers used:");
    for(u32 i = 0; i < required_layer_count; ++i)
    {
        bool found = false;

        for(u32 j = 0; j < available_layer_count; ++j)
        {
            if(string_equal(required_layers[i], available_layers[j].layerName))
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
    result = platform_window_create_vulkan_surface(backend->window_state, context);
    if(!vulkan_result_is_success(result))
    {
        kerror("Failed to create vulkan surface with result: %s", vulkan_result_get_string(result, true));
        return false;
    }
    ktrace("Vulkan surfcae created.");

    // Создание физического и логического устройства.
    // TODO: Провести переделку логики vulkan_device_*!
    result = vulkan_device_create(backend, context);
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

    // Создание кадровых буферов цепочки обмена.
    framebuffers_create(backend, &context->swapchain);
    ktrace("Vulkan swapchain framebuffers created.");

    // Создание буферов команд.
    command_buffers_create(backend);
    ktrace("Vulkan command buffers created (Now only graphics!).");

    // Создание объектов синхронизации.
    sync_objects_create();
    ktrace("Vulkan sync objects created.");

    // Создание шейдеров.
    if(!vulkan_material_shader_create(context, &context->material_shader))
    {
        kerror("Function '%': Failed to load built-in basic lighting shader.", __FUNCTION__);
        return false;
    }
    ktrace("Vulkan shaders created.");

    // Создание буферов данных.
    if(!buffers_create(context))
    {
        kerror("Function '%s': Failed to create buffers.", __FUNCTION__);
        return false;
    }
    ktrace("Vulkan buffers created.");

    return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend)
{
    kassert_debug(context != null, message_context_not_initialized);

    vkDeviceWaitIdle(context->device.logical);

    // Уничтожение буферов данных.
    buffers_destroy(context);
    ktrace("Vulkan buffers destroyed.");

    // Уничтожение шейдеров.
    vulkan_material_shader_destroy(context, &context->material_shader);
    ktrace("Vulkan shaders destroyed.");

    // Уничтожение объектов сингхронизации.
    sync_objects_destroy();
    ktrace("Vulkan sync objects destroyed.");

    // Уничтожение буферов команд.
    command_buffers_destroy(backend);
    ktrace("Vulkan command buffers destroyed.");

    // Уничтожение кадровых буферов цепочки обмена.
    framebuffers_destroy(backend, &context->swapchain);
    ktrace("Vulkan swapchain framebuffers destroyed.");

    // Уничтожение визуализатора.
    vulkan_renderpass_destroy(context, &context->main_renderpass);
    ktrace("Vulkan renderpass destroyed.");

    // Уничтожение цепочки обмена.
    vulkan_swapchain_destroy(context, &context->swapchain);
    ktrace("Vulkan swapchain destroyed.");

    // Уничтожение физического и логического устройства.
    vulkan_device_destroy(backend, context);
    ktrace("Vulkan device destroyed.");

    // Уничтожение поверхности Vulkan-Platform.
    if(context->surface)
    {
        platform_window_destroy_vulkan_surface(backend->window_state, context);
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
        kfree_tc(context, vulkan_context, 1, MEMORY_TAG_RENDERER);
        context = null;
        ktrace("Vulkan context destroyed.");
    }
}

void vulkan_renderer_backend_on_resized(renderer_backend* backend, i32 width, i32 height)
{
    // Обновление размер, и генерации, который позволит увидеть изменения размеров!
    cached_framebuffer_width = width;
    cached_framebuffer_height = height;
    context->framebuffer_size_generation++;

    kdebug("Vulkan renderer resized (w/h/gen): %d / %d / %lld", width, height, context->framebuffer_size_generation);
}

bool vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time)
{
    // context->frame_delta_time = delta_time;
    
    vulkan_device* device = &context->device;

    // Проверка на воссоздание цепочки обмена.
    if(context->recreating_swapchain)
    {
        VkResult result = vkDeviceWaitIdle(device->logical);
        if(!vulkan_result_is_success(result))
        {
            kerror("Vulkan renderer begin frame vkDeviceWaitIdle (1) failed: '%s'.", vulkan_result_get_string(result, true));
            return false;
        }

        kdebug("Recreating swapchain, booting!");
        return false;
    }

    // Проверка на изменение размера окна, тогда воссоздание цепочки обмена.
    if(context->framebuffer_size_generation != context->framebuffer_size_last_generation)
    {
        VkResult result = vkDeviceWaitIdle(device->logical);
        if(!vulkan_result_is_success(result))
        {
            kerror("Vulkan renderer begin frame vkDeviceWaitIdle (2) failed: '%s'.", vulkan_result_get_string(result, true));
            return false;
        }

        // Если воссоздание цепочки обмена не удалось (например, из-за того, что окно было свернуто),
        // загрузитесь, прежде чем снимать флаг.
        if(!swapchain_recreate(backend))
        {
            return false;
        }

        kdebug("Resized, booting.");
        return false;
    }

    // Дождитесь завершения выполнения текущего кадра, после этого fence станет свободен, этот кадр может двигаться дальше.
    if(!vulkan_fence_wait(context, &context->in_flight_fences[context->current_frame], U64_MAX))
    {
        kwarng("In-flight fence wait failure!");
        return false;
    }

    // Получить следующее изображение из цепочки обмена. Передать семафор, который должен сигнализировать о завершении.
    // Этот же семафор позже будет ожидаться отправкой очереди, чтобы гарантировать доступность этого изображения.
    if(!vulkan_swapchain_acquire_next_image_index(
            context, &context->swapchain, U64_MAX, context->image_available_semaphores[context->current_frame], 
            null, &context->image_index
    ))
    {
        return false;
    }

    // Начало записи команд.
    vulkan_command_buffer* command_buffer = &context->graphics_command_buffers[context->image_index];
    vulkan_command_buffer_reset(command_buffer);
    vulkan_command_buffer_begin(command_buffer, false, false, false);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = (f32)context->framebuffer_height;
    viewport.width = (f32)context->framebuffer_width;
    viewport.height = -(f32)context->framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context->framebuffer_width;
    scissor.extent.height = context->framebuffer_height;

    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    context->main_renderpass.w = context->framebuffer_width;
    context->main_renderpass.h = context->framebuffer_height;

    // Начало прохода визуализатора.
    vulkan_renderpass_begin(
        command_buffer, &context->main_renderpass, context->swapchain.framebuffers[context->image_index].handle
    );

    return true;
}

bool vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time)
{
    vulkan_command_buffer* command_buffer = &context->graphics_command_buffers[context->image_index];

    // Конец прохода визуализатора.
    vulkan_renderpass_end(command_buffer, &context->main_renderpass);

    // Конец записи команд.
    vulkan_command_buffer_end(command_buffer);

    // Проверка, что предыдущий кадр не использует этот кадр (т.е. его fence находится в режиме ожидания).
    if(context->images_in_flight[context->image_index] != VK_NULL_HANDLE)
    {
        vulkan_fence_wait(context, context->images_in_flight[context->image_index], U64_MAX);
    }

    // Делаем fence изображения как используемый в этом кадре.
    context->images_in_flight[context->image_index] = &context->in_flight_fences[context->current_frame];

    // Сбрасываем fence для нового кадра.
    vulkan_fence_reset(context, &context->in_flight_fences[context->current_frame]);

    // Отправляем в очередь и ждем завершения операции.
    VkSubmitInfo submitinfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitinfo.commandBufferCount = 1;
    submitinfo.pCommandBuffers = &command_buffer->handle;
    submitinfo.signalSemaphoreCount = 1;
    submitinfo.pSignalSemaphores = &context->queue_complete_semaphores[context->current_frame];
    submitinfo.waitSemaphoreCount = 1;
    submitinfo.pWaitSemaphores = &context->image_available_semaphores[context->current_frame];

    // Каждый семафор ждет ответа от pipeline о завершении.
    VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitinfo.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(
        context->device.graphics_queue.handle, 1, &submitinfo, context->in_flight_fences[context->current_frame].handle
    );
    if(!vulkan_result_is_success(result))
    {
        kerror("Failed to submit queue with result: %s", vulkan_result_get_string(result, true));
        return false;
    }

    vulkan_command_buffer_update_submitted(command_buffer);

    // Возвращаем изображение в цепочку обмена.
    vulkan_swapchain_present(
        context, &context->swapchain, context->queue_complete_semaphores[context->current_frame], context->image_index
    );

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_message_handler(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data
)
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
        kzero_tc(context->graphics_command_buffers, vulkan_command_buffer, context->swapchain.image_count);
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

void framebuffers_create(renderer_backend* backend, vulkan_swapchain* swapchain)
{
    swapchain->framebuffers = darray_reserve(vulkan_framebuffer, context->swapchain.image_count);
    kzero_tc(swapchain->framebuffers, vulkan_framebuffer, context->swapchain.image_count);
    framebuffers_regenerate(backend, swapchain, &context->main_renderpass);
}

void framebuffers_regenerate(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass)
{
    for(u32 i = 0; i < swapchain->image_count; ++i)
    {
        if(swapchain->framebuffers[i].handle)
        {
            vulkan_framebuffer_destroy(context, &swapchain->framebuffers[i]);
        }
        
        // TODO: Сделать динамическим на основе текущих вложений.
        u32 attachment_count = 2;

        VkImageView attachments[] = {
            swapchain->views[i],
            swapchain->depth_attachment.view
        };

        vulkan_framebuffer_create(
            context, renderpass, context->framebuffer_width, context->framebuffer_height, attachment_count,
            attachments, &swapchain->framebuffers[i]
        );
    }
}

void framebuffers_destroy(renderer_backend* backend, vulkan_swapchain* swapchain)
{
    for(u32 i = 0; i < swapchain->image_count; ++i)
    {
        vulkan_framebuffer_destroy(context, &swapchain->framebuffers[i]);
    }

    darray_destroy(swapchain->framebuffers);
}

void sync_objects_create()
{
    context->image_available_semaphores = darray_reserve(VkSemaphore, context->swapchain.max_frames_in_flight);
    context->queue_complete_semaphores = darray_reserve(VkSemaphore, context->swapchain.max_frames_in_flight);
    context->in_flight_fences = darray_reserve(vulkan_fence, context->swapchain.max_frames_in_flight);

    for(u8 i = 0; i < context->swapchain.max_frames_in_flight; ++i)
    {
        VkSemaphoreCreateInfo semaphoreinfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        vkCreateSemaphore(
            context->device.logical, &semaphoreinfo, context->allocator, &context->image_available_semaphores[i]
        );

        vkCreateSemaphore(
            context->device.logical, &semaphoreinfo, context->allocator, &context->queue_complete_semaphores[i]
        );

        // Создание ограждения в сигнальном состоянии, указывающее, что первый кадр уже «отрисован».
        // Это предотвратит бесконечное ожидание приложением отрисовки первого кадра, поскольку он
        // не может быть отрисован, пока не будет «отрисован» кадр до него.
        vulkan_fence_create(context, true, &context->in_flight_fences[i]);
    }

    // В полете ограждения еще не должны существовать на этом этапе, поэтому очистите список указателей.
    context->images_in_flight = darray_reserve(vulkan_fence*, context->swapchain.image_count);
    kzero_tc(context->images_in_flight, vulkan_fence*, context->swapchain.image_count);
}

void sync_objects_destroy()
{
    for(u8 i = 0; i < context->swapchain.max_frames_in_flight; ++i)
    {
        if(context->image_available_semaphores[i])
        {
            vkDestroySemaphore(context->device.logical, context->image_available_semaphores[i], context->allocator);
            context->image_available_semaphores[i] = null;
        }

        if(context->queue_complete_semaphores[i])
        {
            vkDestroySemaphore(context->device.logical, context->queue_complete_semaphores[i], context->allocator);
            context->queue_complete_semaphores[i] = null;
        }

        vulkan_fence_destroy(context, &context->in_flight_fences[i]);
    }

    darray_destroy(context->image_available_semaphores);
    context->image_available_semaphores = null;

    darray_destroy(context->queue_complete_semaphores);
    context->queue_complete_semaphores = null;

    darray_destroy(context->in_flight_fences);
    context->in_flight_fences = null;

    darray_destroy(context->images_in_flight);
    context->images_in_flight = null;
}

bool swapchain_recreate(renderer_backend* backend)
{
    // Если уже начато воссоздание, ничего не делать.
    if(context->recreating_swapchain)
    {
        kdebug("Function '%s' called when already recreating. Booting.", __FUNCTION__);
        return false;
    }

    // Обнаружение малых размеров окна.
    if(context->framebuffer_width == 0 || context->framebuffer_height == 0)
    {
        kdebug("Function '%s' called when window is < 1 in a dimension. Booting.", __FUNCTION__);
        return false;
    }

    context->recreating_swapchain = true;

    // Ожидание завершения операций.
    vkDeviceWaitIdle(context->device.logical);
    kzero_tc(context->images_in_flight, vulkan_fence*, context->swapchain.image_count);

    vulkan_swapchain_recreate(context, cached_framebuffer_width, cached_framebuffer_height, &context->swapchain);

    // Синхронизация размера буферов с кэшем.
    context->framebuffer_width = cached_framebuffer_width;
    context->framebuffer_height = cached_framebuffer_height;
    context->main_renderpass.x = 0;
    context->main_renderpass.y = 0;
    context->main_renderpass.w = cached_framebuffer_width;
    context->main_renderpass.h = cached_framebuffer_height;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    // Обновление генерации.
    context->framebuffer_size_last_generation = context->framebuffer_size_generation;

    // Очистка буферов команд.
    // TODO: Сделать регенерацию командных буферов.
    for(u32 i = 0; i < context->swapchain.image_count; ++i)
    {
        vulkan_command_buffer_free(
            context, context->device.graphics_queue.command_pool, &context->graphics_command_buffers[i]
        );
    }

    framebuffers_regenerate(backend, &context->swapchain, &context->main_renderpass);
    // TODO: Вставить регенерацию сюда!
    command_buffers_create(backend);

    context->recreating_swapchain = false;

    return true;
    
}

bool buffers_create(vulkan_context* context)
{
    VkMemoryPropertyFlagBits memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlagBits vertex_usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkBufferUsageFlagBits index_usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    const u64 vertex_buffer_size = sizeof(struct vertex_3d) * 1024 * 1024;
    if(!vulkan_buffer_create(context, vertex_buffer_size, vertex_usage_flags, memory_property_flags, true, &context->object_vertex_buffer))
    {
        kerror("Function '%s': Failed to create vertex buffer.", __FUNCTION__);
        return false;
    }

    context->geometry_vertex_offset = 0;

    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if(!vulkan_buffer_create(context, index_buffer_size, index_usage_flags, memory_property_flags, true, &context->object_index_buffer))
    {
        kerror("Function '%s': Failed to create index buffer.", __FUNCTION__);
        return false;
    }

    return true;
}

void buffers_destroy(vulkan_context* context)
{
    vulkan_buffer_destroy(context, &context->object_vertex_buffer);
    vulkan_buffer_destroy(context, &context->object_index_buffer);
}
