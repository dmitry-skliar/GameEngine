// Cобственные подключения.
#include "renderer/vulkan/vulkan_backend.h"
#include "renderer/vulkan/vulkan_types.h"
#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_platform.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_command_buffer.h"
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_pipeline.h"

// Внутренние подключения.
#include "logger.h"
#include "debug/assert.h"
#include "memory/memory.h"
#include "containers/darray.h"
#include "kstring.h"
#include "math/math_types.h"
#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/shader_system.h"

static vulkan_context* context = null;
static u32 cached_framebuffer_width  = 0;
static u32 cached_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_message_handler(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data
);
i32 find_memory_index(u32 type_filter, u32 property_flags);
void command_buffers_create(renderer_backend* backend);
void command_buffers_destroy(renderer_backend* backend);
void framebuffers_regenerate(bool first_destroy_buffers);
void framebuffers_destroy();
void sync_objects_create();
void sync_objects_destroy();
bool swapchain_recreate(renderer_backend* backend);
bool buffers_create(vulkan_context* context);
void buffers_destroy(vulkan_context* context);
bool upload_data_range(
    VkCommandPool pool, VkFence fence, VkQueue queue, vulkan_buffer* buffer, u64* out_offset, u64 size, const void* data
);
void free_data_range(vulkan_buffer* buffer, u64 offset, u64 size);

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
    appinfo.pApplicationName         = backend->window_state->title;
    appinfo.pEngineName              = "Game Engine";
    appinfo.apiVersion               = VK_API_VERSION_1_4;
    appinfo.applicationVersion       = VK_MAKE_VERSION(1, 0, 0);
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

    kdebug("Vulkan supported extensions:");
    for(u32 i = 0; i < available_extension_count; ++i)
    {
        kdebug("[%2u] %s", i, avaulable_extensions[i].extensionName);
    }

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

    kdebug("Vulkan supported validation layers:");
    for(u32 i = 0; i < available_layer_count; ++i)
    {
        kdebug("[%2u] %s", i, available_layers[i].layerName);
    }

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

    // Создание world прохода визуализатора.
    vulkan_renderpass_create(
        context, &context->main_renderpass, (vec4){{ 0, 0, context->framebuffer_width, context->framebuffer_height }},
        (vec4){{ 0.0f, 0.0f, 0.2f, 1.0f }}, 1.0f, 0, RENDERPASS_CLEAR_COLOR_BUFFER_FLAG | RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG
        | RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG, false, true
    );
    ktrace("Vulkan world renderpass created.");

    // Создание ui прохода визуализатора.
    vulkan_renderpass_create(
        context, &context->ui_renderpass, (vec4){{ 0, 0, context->framebuffer_width, context->framebuffer_height }},
        (vec4){{ 0.0f, 0.0f, 0.0f, 0.0f }}, 1.0f, 0, RENDERPASS_CLEAR_NONE_FLAG, true, false
    );
    ktrace("Vulkan ui renderpass created.");

    // Создание кадровых буферов цепочки обмена.
    framebuffers_regenerate(false);
    ktrace("Vulkan swapchain framebuffers created.");

    // Создание буферов команд.
    command_buffers_create(backend);
    ktrace("Vulkan command buffers created (Now only graphics!).");

    // Создание объектов синхронизации.
    sync_objects_create();
    ktrace("Vulkan sync objects created.");

    // Создание буферов данных в локальной памяти устройства (видеокарте).
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
    kassert_debug(context != null, "");

    vkDeviceWaitIdle(context->device.logical);

    // Уничтожение буферов данных.
    buffers_destroy(context);
    ktrace("Vulkan buffers destroyed.");

    // Уничтожение объектов сингхронизации.
    sync_objects_destroy();
    ktrace("Vulkan sync objects destroyed.");

    // Уничтожение буферов команд.
    command_buffers_destroy(backend);
    ktrace("Vulkan command buffers destroyed.");

    // Уничтожение кадровых буферов цепочки обмена.
    framebuffers_destroy();
    ktrace("Vulkan swapchain framebuffers destroyed.");

    // Уничтожение ui прохода визуализатора.
    vulkan_renderpass_destroy(context, &context->ui_renderpass);
    ktrace("Vulkan ui renderpass destroyed.");

    // Уничтожение world прохода визуализатора.
    vulkan_renderpass_destroy(context, &context->main_renderpass);
    ktrace("Vulkan world renderpass destroyed.");

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
    context->frame_delta_time = delta_time;
    
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
    VkResult result = vkWaitForFences(context->device.logical, 1, &context->in_flight_fences[context->current_frame], true, U64_MAX);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failure to wait in-flight fence with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

    // Получить следующее изображение из цепочки обмена. Передать семафор, который должен сигнализировать о завершении.
    // Этот же семафор позже будет ожидаться отправкой очереди, чтобы гарантировать доступность этого изображения.
    if(!vulkan_swapchain_acquire_next_image_index(
            context, &context->swapchain, U64_MAX, context->image_available_semaphores[context->current_frame], 
            null, &context->image_index
    ))
    {
        kerror("Function '%s': Failed to acquire next image index, booting.", __FUNCTION__);
        return false;
    }

    // Начало записи команд.
    vulkan_command_buffer* command_buffer = &context->graphics_command_buffers[context->image_index];
    vulkan_command_buffer_reset(command_buffer);
    vulkan_command_buffer_begin(command_buffer, false, false, false);

    // Область просмотра.
    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = (f32)context->framebuffer_height;
    viewport.width = (f32)context->framebuffer_width;
    viewport.height = -(f32)context->framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Область отсечения.
    VkRect2D scissor = {0};
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context->framebuffer_width;
    scissor.extent.height = context->framebuffer_height;

    // NOTE: Т.к. на pipeline включено динамическое состояние, т.е. нужно указывать при каждом проходе.
    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    context->main_renderpass.render_area.width = context->framebuffer_width;
    context->main_renderpass.render_area.height = context->framebuffer_height;
    context->ui_renderpass.render_area.width = context->framebuffer_width;
    context->ui_renderpass.render_area.height = context->framebuffer_height;

    return true;
}

bool vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time)
{
    VkResult result = VK_ERROR_UNKNOWN;
    vulkan_command_buffer* command_buffer = &context->graphics_command_buffers[context->image_index];

    // Конец записи команд.
    vulkan_command_buffer_end(command_buffer);

    // Проверка, что предыдущий кадр не использует этот кадр (т.е. его fence находится в режиме ожидания).
    if(context->images_in_flight[context->image_index] != VK_NULL_HANDLE)
    {
        result = vkWaitForFences(context->device.logical, 1, &context->images_in_flight[context->image_index], true, U64_MAX);
        if(!vulkan_result_is_success(result))
        {
            kerror("Function '%s': Failed to wait fence with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        }
    }

    // Делаем fence изображения как используемый в этом кадре.
    context->images_in_flight[context->image_index] = context->in_flight_fences[context->current_frame];

    // Сбрасываем fence для следующего кадра.
    result = vkResetFences(context->device.logical, 1, &context->in_flight_fences[context->current_frame]);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to reset fence with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

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

    result = vkQueueSubmit(
        context->device.graphics_queue.handle, 1, &submitinfo, context->in_flight_fences[context->current_frame]
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

bool vulkan_renderer_begin_renderpass(renderer_backend* backend, builtin_renderpass renderpass_id)
{
    vulkan_renderpass* renderpass = null;
    VkFramebuffer framebuffer = null;
    u32 image_index = context->image_index;
    vulkan_command_buffer* command_buffer = &context->graphics_command_buffers[image_index];

    switch(renderpass_id)
    {
        case BUILTIN_RENDERPASS_WORLD:
            renderpass = &context->main_renderpass;
            framebuffer = context->world_framebuffers[image_index];
            break;
        case BUILTIN_RENDERPASS_UI:
            renderpass = &context->ui_renderpass;
            framebuffer = context->swapchain.framebuffers[image_index];
            break;
        default:
            kerror("Function '%s' called on unrecognized renderpass id: %#02x", __FUNCTION__, renderpass_id);
            return false;
    }

    vulkan_renderpass_begin(command_buffer, renderpass, framebuffer);

    return true;
}

bool vulkan_renderer_end_renderpass(renderer_backend* backend, builtin_renderpass renderpass_id)
{
    vulkan_renderpass* renderpass = null;
    u32 image_index = context->image_index;
    vulkan_command_buffer* command_buffer = &context->graphics_command_buffers[image_index];

    switch(renderpass_id)
    {
        case BUILTIN_RENDERPASS_WORLD:
            renderpass = &context->main_renderpass;
            break;
        case BUILTIN_RENDERPASS_UI:
            renderpass = &context->ui_renderpass;
            break;
        default:
            kerror("Function '%s' called on unrecognized renderpass id: %#02x", __FUNCTION__, renderpass_id);
            return false;
    }

    vulkan_renderpass_end(command_buffer, renderpass);

    return true;
}

void vulkan_renderer_draw_geometry(geometry_render_data data)
{
    // Игнорирование не загруженных геометрий.
    if(!data.geometry || data.geometry->internal_id == INVALID_ID)
    {
        return;
    }

    vulkan_geometry_data* buffer_data = &context->geometries[data.geometry->internal_id];
    vulkan_command_buffer* command_buffer = &context->graphics_command_buffers[context->image_index];

    // material* m = data.geometry->material;
    // if(!m)
    // {
    //     m = material_system_get_default();
    // }

    // switch(m->type)
    // {
    //     case MATERIAL_TYPE_WORLD:
    //         vulkan_shader_bind_instance(&context->material_shader, m->internal_id);
    //         vulkan_shader_set_uniform_vec4(&context->material_shader, context->material_shader_diffuse_color_location, m->diffuse_color);
    //         vulkan_shader_set_sampler(&context->material_shader, context->material_shader_diffuse_texture_location, m->diffuse_map.texture);
    //         vulkan_shader_set_uniform_mat4(&context->material_shader, context->material_shader_mode_location, data.model);
    //         vulkan_shader_apply_instance(&context->material_shader);
    //         break;
    //     case MATERIAL_TYPE_UI:
    //         vulkan_shader_bind_instance(&context->ui_shader, m->internal_id);
    //         vulkan_shader_set_uniform_vec4(&context->ui_shader, context->ui_shader_diffuse_color_location, m->diffuse_color);
    //         vulkan_shader_set_sampler(&context->ui_shader, context->ui_shader_diffuse_texture_location, m->diffuse_map.texture);
    //         vulkan_shader_set_uniform_mat4(&context->ui_shader, context->ui_shader_mode_location, data.model);
    //         vulkan_shader_apply_instance(&context->ui_shader);
    //         break;
    //     default:
    //         kerror("Function '%s': Unknown material type.", __FUNCTION__);
    //         return;
    // }

    // Привязка буфера вершин co смещением.
    VkDeviceSize offset[1] = { buffer_data->vertex_buffer_offset };
    vkCmdBindVertexBuffers(command_buffer->handle, 0, 1, &context->object_vertex_buffer.handle, offset);

    if(buffer_data->index_count > 0)
    {
        // Привязка буфера индексов.
        vkCmdBindIndexBuffer(
            command_buffer->handle, context->object_index_buffer.handle, buffer_data->index_buffer_offset,
            VK_INDEX_TYPE_UINT32
        );
        // Рисовать.
        // TODO: VUID-vkCmdDrawIndexed-None-08114
        vkCmdDrawIndexed(command_buffer->handle, buffer_data->index_count, 1, 0, 0, 0);
    }
    else
    {
        // Рисовать.
        vkCmdDraw(command_buffer->handle, buffer_data->vertex_count, 1, 0, 0);
    }
}

void vulkan_renderer_create_texture(texture* texture, const void* pixels)
{
    // Создание data.
    // TODO: Используется распределитель памяти тут.
    texture->internal_data = kallocate_tc(vulkan_texture_data, 1, MEMORY_TAG_TEXTURE);
    vulkan_texture_data* data = texture->internal_data;
    VkDeviceSize image_size = texture->width * texture->height * texture->channel_count;

    // NOTE: Предполагается 8 бит на канал.
    VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

    // Создание промежуточного буфера и загрузка данных в него.
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkan_buffer staging;
    vulkan_buffer_create(context, image_size, usage, memory_flags, true, &staging);

    vulkan_buffer_load_data(context, &staging, 0, image_size, 0, pixels);

    // NOTE: Здесь много предположений, разные типы текстур потребуют разных параметров.
    vulkan_image_create(
        context, VK_IMAGE_TYPE_2D, texture->width, texture->height, image_format, VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_COLOR_BIT,
        &data->image
    );

    vulkan_command_buffer command_buffer;
    VkCommandPool pool = context->device.graphics_queue.command_pool;
    VkQueue queue = context->device.graphics_queue.handle;

    vulkan_command_buffer_allocate_and_begin_single_use(context, pool, &command_buffer);

    // Изменение текущий макета на оптимальный для приема данных.
    vulkan_image_transition_layout(
        context, &command_buffer, &data->image, &image_format, VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    // Копирование данных из буфера.
    vulkan_image_copy_from_buffer(context, &data->image, staging.handle, &command_buffer);

    // Переход от оптимальной компоновки для получения данных к оптимальной компоновке только для чтения шейдеров.
    vulkan_image_transition_layout(
        context, &command_buffer, &data->image, &image_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vulkan_command_buffer_end_single_use(context, pool, &command_buffer, queue);

    // Уничтожение толко после записи буфера команд, потому что во время выполнение может оказаться нулевыми данными!
    vulkan_buffer_destroy(context, &staging);

    // Создание фильтрации для текстуры.
    VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    // TODO: Эти фильтры должны быть настраиваемыми.
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    VkResult result = vkCreateSampler(context->device.logical, &sampler_info, context->allocator, &data->sampler);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Error creating texture sampler: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return;
    }

    texture->generation++;
}

void vulkan_renderer_destroy_texture(texture* texture)
{
    vkDeviceWaitIdle(context->device.logical);

    vulkan_texture_data* data = texture->internal_data;

    if(data)
    {
        vulkan_image_destroy(context, &data->image);
        kzero_tc(&data->image, struct vulkan_image, 1);
        vkDestroySampler(context->device.logical, data->sampler, context->allocator);
        kfree_tc(texture->internal_data, vulkan_texture_data, 1, MEMORY_TAG_TEXTURE);
    }
    else
    {
        if(texture->generation != INVALID_ID)
        {
            kerror("Function '%s': Failed to get vulkan specific data of texture.", __FUNCTION__);
        }
        else
        {
            kwarng("Function '%s': Texture was not created. Skipping...", __FUNCTION__);
        }
    }

    kzero_tc(texture, struct texture, 1);
}

bool vulkan_renderer_create_geometry(
    geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count,
    const void* indices
)
{
    if(!vertex_count || !vertices)
    {
        kerror(
            "Function '%s' requires vertex data, and none was supplied. vertex_count=%d, vertices=%p",
            __FUNCTION__, vertex_count, vertices
        );
        return false;
    }

    // Проверка на повторную загрузку. Если это так, необходимо освободить старые данные после этого.
    bool is_reupload = geometry->internal_id != INVALID_ID;
    vulkan_geometry_data old_range;

    vulkan_geometry_data* internal_data = null;
    if(is_reupload)
    {
        internal_data = &context->geometries[geometry->internal_id];
        // Копия старого диапазона.
        kcopy_tc(&old_range, internal_data, vulkan_geometry_data, 1);
    }
    else
    {
        for(u32 i = 0; i < VULKAN_SHADER_MAX_GEOMETRY_COUNT; ++i)
        {
            if(context->geometries[i].id == INVALID_ID)
            {
                // Поиск свободного индекса.
                geometry->internal_id = i;
                context->geometries[i].id = i;
                internal_data = &context->geometries[i];
                break;
            }
        }
    }

    if(!internal_data)
    {
        kerror(
            "Function '%s': Failed to find a free index for a new geometry upload. Adjust config to allow for more.",
            __FUNCTION__
        );
        return false;
    }

    VkCommandPool pool = context->device.graphics_queue.command_pool;
    VkQueue queue = context->device.graphics_queue.handle;
    u32 total_size = 0;

    // Данные вершин.
    internal_data->vertex_count = vertex_count;
    internal_data->vertex_element_size = vertex_size;
    total_size = vertex_count * vertex_size;

    if(!upload_data_range(
        pool, null, queue, &context->object_vertex_buffer, &internal_data->vertex_buffer_offset, total_size, vertices
    ))
    {
        kerror("Function '%s': Failed to upload to the vertex buffer.", __FUNCTION__);
        return false;
    }

    // Данные индексов, если поддерживается.
    if(index_count && indices)
    {
        internal_data->index_count = index_count;
        internal_data->index_element_size = index_size;
        total_size = index_count * index_size;

        if(!upload_data_range(
            pool, null, queue, &context->object_index_buffer, &internal_data->index_buffer_offset, total_size, indices
        ))
        {
            kerror("Function '%s': Failed to upload to the index buffer.", __FUNCTION__);
            return false;
        }
    }

    if(internal_data->generation == INVALID_ID)
    {
        internal_data->generation = 0;
    }
    else
    {
        internal_data->generation++;
    }

    if(is_reupload)
    {
        // Освобождение данных вершин.
        total_size = old_range.vertex_element_size * old_range.vertex_count;
        free_data_range(&context->object_vertex_buffer, old_range.vertex_buffer_offset, total_size);

        // Освобождение данных индексов, если доступно.
        total_size = old_range.index_element_size * old_range.index_count;
        if(total_size > 0)
        {
            free_data_range(&context->object_index_buffer, old_range.index_buffer_offset, total_size);
        }
    }

    return true;
}

void vulkan_renderer_destroy_geometry(geometry* geometry)
{
    if(!geometry || geometry->internal_id == INVALID_ID)
    {
        return;
    }

    vkDeviceWaitIdle(context->device.logical);
    vulkan_geometry_data* internal_data = &context->geometries[geometry->internal_id];

    // Освобождение данных вершин.
    u32 total_size = internal_data->vertex_element_size * internal_data->vertex_count;
    free_data_range(&context->object_vertex_buffer, internal_data->vertex_buffer_offset, total_size);

    // Освобождение данных индексов, если доступно.
    total_size = internal_data->index_element_size * internal_data->index_count;
    if(total_size > 0)
    {
        free_data_range(&context->object_index_buffer, internal_data->index_buffer_offset, total_size);
    }

    // Освобождение диапазона для нового использования.
    kzero_tc(internal_data, vulkan_geometry_data, 1);
    internal_data->id = INVALID_ID;
    internal_data->generation = INVALID_ID;
}

const u32 DESC_SET_INDEX_GLOBAL   = 0;
const u32 DESC_SET_INDEX_INSTANCE = 1;

const u32 BINDING_INDEX_UBO       = 0;
const u32 BINDING_INDEX_SAMPLER   = 1;

bool vulkan_renderer_shader_create(shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages)
{
    if(!shader || !stage_filenames || !stages)
    {
        kerror("Function '%s' requires a valid pointer to shader, stage_filenames and stages. Creation failed.", __FUNCTION__);
        return false;
    }

    shader->internal_data = kallocate_tc(vulkan_shader, 1, MEMORY_TAG_RENDERER);
    kzero_tc(shader->internal_data, vulkan_shader, 1);
    vulkan_shader* vk_shader = shader->internal_data;

    // TODO: Динамический проход рендера.
    vulkan_renderpass* renderpass = null;
    switch(renderpass_id)
    {
        case BUILTIN_RENDERPASS_WORLD:
            renderpass = &context->main_renderpass;
            break;
        case BUILTIN_RENDERPASS_UI:
            renderpass = &context->ui_renderpass;
            break;
        default:
            kerror("");
            return false;
    }

    // Трансляция стадий шейдера -> Vulkan.
    VkShaderStageFlags vk_stages[VULKAN_SHADER_MAX_STAGES];
    for(u8 i = 0; i < stage_count; ++i)
    {
        switch (stages[i])
        {
            case SHADER_STAGE_FRAGMENT:
                vk_stages[i] = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case SHADER_STAGE_VERTEX:
                vk_stages[i] = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case SHADER_STAGE_GEOMETRY:
                kwarng("Function '%s': VK_SHADER_STAGE_GEOMETRY_BIT is set but not yet supported.", __FUNCTION__);
                vk_stages[i] = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case SHADER_STAGE_COMPUTE:
                kwarng("Function '%s': SHADER_STAGE_COMPUTE is set but not yet supported.", __FUNCTION__);
                vk_stages[i] = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                kerror("Function '%s': Unsupported stage type: %d.", __FUNCTION__, stages[i]);
                break;
        }
    }

    // TODO: настраиваемое максимальное количество выделенных дескрипторов.
    u32 max_descriptor_allocate_count = 1024;

    vk_shader->renderpass = renderpass;
    vk_shader->config.max_descriptor_set_count = max_descriptor_allocate_count;
    vk_shader->config.stage_count = 0;

    // Iterate provided stages.
    for(u32 i = 0; i < stage_count; i++)
    {
        // Проверка доступных слотов стадий шейдера.
        if(vk_shader->config.stage_count + 1 > VULKAN_SHADER_MAX_STAGES)
        {
            kerror("Function '%s': Shaders may have a maximum of %d stages", __FUNCTION__, VULKAN_SHADER_MAX_STAGES);
            return false;
        }

        // Проверка поддерживаемых шейдеров.
        VkShaderStageFlagBits stage_flag;
        switch(stages[i])
        {
            case SHADER_STAGE_VERTEX:
                stage_flag = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case SHADER_STAGE_FRAGMENT:
                stage_flag = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            default:
                kerror("Function '%s': Unsupported shader stage flagged: %d. Stage ignored.", __FUNCTION__, stages[i]);
                continue;
        }

        vk_shader->config.stages[vk_shader->config.stage_count].stage = stage_flag;
        string_ncopy(vk_shader->config.stages[vk_shader->config.stage_count].file_name, stage_filenames[i], 255);
        vk_shader->config.stage_count++;
    }

    // HACK: Максимальное число ubo дескрипторных наборов.
    vk_shader->config.pool_sizes[0] = (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024};
    // HACK: Максимальное число image sampler дескрипторных наборов.
    vk_shader->config.pool_sizes[1] = (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096};

    // Глобальный набор дескрипторов (UBO).
    vulkan_descriptor_set_config* global_descriptor_set_config = &vk_shader->config.descriptor_sets[DESC_SET_INDEX_GLOBAL];
    global_descriptor_set_config->bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
    global_descriptor_set_config->bindings[BINDING_INDEX_UBO].descriptorCount = 1;
    global_descriptor_set_config->bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_descriptor_set_config->bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    global_descriptor_set_config->binding_count++;
    vk_shader->config.descriptor_set_count++;

    // При изпользовании экземпляров, добавляется второй набор дескрипторов (UBO).
    if(shader->use_instances)
    {
        vulkan_descriptor_set_config* instance_descriptor_set_config = &vk_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE];
        instance_descriptor_set_config->bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
        instance_descriptor_set_config->bindings[BINDING_INDEX_UBO].descriptorCount = 1;
        instance_descriptor_set_config->bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        instance_descriptor_set_config->bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        instance_descriptor_set_config->binding_count++;
        vk_shader->config.descriptor_set_count++;
    }

    // Делает недействительными экземпляры.
    // TODO: Динамически.
    for(u32 i = 0; i < 1024; ++i)
    {
        vk_shader->instance_states[i].id = INVALID_ID;
    }

    return true;
}

bool vulkan_renderer_shader_status_valid(shader* shader, const char* func_name)
{
    if(!shader || !shader->internal_data)
    {
        if(func_name)
        {
            kerror("Function '%s' requires a valid pointer to shader.", __FUNCTION__);
        }
        return false;
    }
    return true;
}

void vulkan_renderer_shader_destroy(shader* shader)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__))
    {
        return;
    }

    VkDevice logical = context->device.logical;
    VkAllocationCallbacks* vk_allocator = context->allocator;
    vulkan_shader* vk_shader = shader->internal_data;

    for(u32 i = 0; i < vk_shader->config.descriptor_set_count; ++i)
    {
        if(vk_shader->descriptor_set_layouts[i])
        {
            vkDestroyDescriptorSetLayout(logical, vk_shader->descriptor_set_layouts[i], vk_allocator);
            vk_shader->descriptor_set_layouts[i] = null;
        }
    }

    if(vk_shader->descriptor_pool)
    {
        vkDestroyDescriptorPool(logical, vk_shader->descriptor_pool, vk_allocator);
    }

    vulkan_buffer_unlock_memory(context, &vk_shader->uniform_buffer);
    vk_shader->uniform_buffer_mapped_block = null;
    vulkan_buffer_destroy(context, &vk_shader->uniform_buffer);

    vulkan_pipeline_destroy(context, &vk_shader->pipeline);

    for(u32 i = 0; i < vk_shader->config.stage_count; ++i)
    {
        // TODO: Их можно уничтожить после создания pipeline!
        vkDestroyShaderModule(logical, vk_shader->stages[i].handle, vk_allocator);
    }

    kfree_tc(vk_shader, vulkan_shader, 1, MEMORY_TAG_RENDERER);
    shader->internal_data = null;
}

bool vulkan_renderer_shader_create_module(vulkan_shader* shader, vulkan_shader_stage_config config, vulkan_shader_stage* shader_stage)
{
    resource binary_resource;
    if (!resource_system_load(config.file_name, RESOURCE_TYPE_BINARY, &binary_resource))
    {
        kerror("Function '%s': Unable to read shader module '%s'.", __FUNCTION__, config.file_name);
        return false;
    }

    kzero_tc(&shader_stage->create_info, VkShaderModuleCreateInfo, 1);
    shader_stage->create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_stage->create_info.codeSize = binary_resource.data_size;
    shader_stage->create_info.pCode = (u32*)binary_resource.data;

    VK_CHECK(vkCreateShaderModule(
        context->device.logical, &shader_stage->create_info, context->allocator, &shader_stage->handle
    ));

    resource_system_unload(&binary_resource);

    kzero_tc(&shader_stage->shader_stage_create_info, VkPipelineShaderStageCreateInfo, 1);
    shader_stage->shader_stage_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage->shader_stage_create_info.stage  = config.stage;
    shader_stage->shader_stage_create_info.module = shader_stage->handle;
    shader_stage->shader_stage_create_info.pName  = "main";

    return true;
}

bool vulkan_renderer_shader_initialize(shader* shader)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__))
    {
        return false;
    }

    VkDevice logical = context->device.logical;
    VkAllocationCallbacks* vk_allocator = context->allocator;
    vulkan_shader* vk_shader = shader->internal_data;

    // Создание модулей шейдеров.
    for(u32 i = 0; i < vk_shader->config.stage_count; ++i)
    {
        if(!vulkan_renderer_shader_create_module(vk_shader, vk_shader->config.stages[i], &vk_shader->stages[i]))
        {
            kerror(
                "Function '%s': Unable to create %s shader module for '%s'. Shader will be destroyed.",
                __FUNCTION__, vk_shader->config.stages[i].file_name, shader->name
            );
            return false;
        }
    }

    // Статическая таблица для трансляции определений движка -> Vulkan.
    static const VkFormat attribute_types[11] = {
        [SHADER_ATTRIB_TYPE_FLOAT32]   = VK_FORMAT_R32_SFLOAT,
        [SHADER_ATTRIB_TYPE_FLOAT32_2] = VK_FORMAT_R32G32_SFLOAT,
        [SHADER_ATTRIB_TYPE_FLOAT32_3] = VK_FORMAT_R32G32B32_SFLOAT,
        [SHADER_ATTRIB_TYPE_FLOAT32_4] = VK_FORMAT_R32G32B32A32_SFLOAT,
        [SHADER_ATTRIB_TYPE_INT8]      = VK_FORMAT_R8_SINT,
        [SHADER_ATTRIB_TYPE_UINT8]     = VK_FORMAT_R8_UINT,
        [SHADER_ATTRIB_TYPE_INT16]     = VK_FORMAT_R16_SINT,
        [SHADER_ATTRIB_TYPE_UINT16]    = VK_FORMAT_R16_UINT,
        [SHADER_ATTRIB_TYPE_INT32]     = VK_FORMAT_R32_SINT,
        [SHADER_ATTRIB_TYPE_UINT32]    = VK_FORMAT_R32_UINT
    };

    // Получение атрибутов.
    u32 attribute_count = darray_length(shader->attributes);
    u32 attribute_offset = 0;
    VkVertexInputAttributeDescription* attrs = vk_shader->config.attributes;
    for(u32 i = 0; i < attribute_count; ++i)
    {
        attrs[i].location = i;
        attrs[i].binding = 0; // NOTE: binding 0 описывает пачуку атрибутов как единый пакет данных (т.е. единый элемент данных).
        attrs[i].offset = attribute_offset;
        attrs[i].format = attribute_types[shader->attributes[i].type];
        attribute_offset += shader->attributes[i].size;
    }

    // Получение uniform переменных.
    u32 uniform_count = darray_length(shader->uniforms);
    for(u32 i = 0; i < uniform_count; ++i)
    {
        // Обновление привязки дескрипторов для сэмлеров!
        if(shader->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER)
        {
            const u32 set_index = shader->uniforms[i].scope == SHADER_SCOPE_GLOBAL ? DESC_SET_INDEX_GLOBAL : DESC_SET_INDEX_INSTANCE;
            vulkan_descriptor_set_config* set_config = &vk_shader->config.descriptor_sets[set_index];

            if(set_config->binding_count < 2)
            {
                // Привязка для первого сэмплера.
                set_config->bindings[BINDING_INDEX_SAMPLER].binding = BINDING_INDEX_SAMPLER;
                set_config->bindings[BINDING_INDEX_SAMPLER].descriptorCount = 1;
                set_config->bindings[BINDING_INDEX_SAMPLER].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                set_config->bindings[BINDING_INDEX_SAMPLER].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                set_config->binding_count++;
            }
            else
            {
                // Для остальных сэмплеров просто увеличиваем индекс.
                set_config->bindings[BINDING_INDEX_SAMPLER].descriptorCount++;
            }
        }
    }

    // Пул дескрипторов.
    VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = vk_shader->config.pool_sizes;
    pool_info.maxSets = vk_shader->config.max_descriptor_set_count;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // Создание пула дескрипторов.
    VkResult result = vkCreateDescriptorPool(logical, &pool_info, vk_allocator, &vk_shader->descriptor_pool);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed creating descriptor pool: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

    // Создание вкладок дескрипторов.
    for(u32 i = 0; i < vk_shader->config.descriptor_set_count; ++i)
    {
        VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layout_info.bindingCount = vk_shader->config.descriptor_sets[i].binding_count;
        layout_info.pBindings = vk_shader->config.descriptor_sets[i].bindings;
        result = vkCreateDescriptorSetLayout(logical, &layout_info, vk_allocator, &vk_shader->descriptor_set_layouts[i]);
        if(!vulkan_result_is_success(result))
        {
            kerror("Function '%s': Failed creating descriptor pool: '%s'", __FUNCTION__, vulkan_result_get_string(result, true));
            return false;
        }
    }

    // Область просмотра: как будет растянуто изображение во фрэймбуфере.
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context->framebuffer_height;
    viewport.width = (f32)context->framebuffer_width;
    viewport.height = -(f32)context->framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Область отсечения: какие пиксели будут сохранены, во время растеризации.
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context->framebuffer_width;
    scissor.extent.height = context->framebuffer_height;

    VkPipelineShaderStageCreateInfo stage_create_infos[VULKAN_SHADER_MAX_STAGES];
    kzero_tc(stage_create_infos, VkPipelineShaderStageCreateInfo, VULKAN_SHADER_MAX_STAGES);
    for(u32 i = 0; i < vk_shader->config.stage_count; ++i)
    {
        stage_create_infos[i] = vk_shader->stages[i].shader_stage_create_info;
    }

    bool pipeline_result = vulkan_graphics_pipeline_create(
        context, vk_shader->renderpass, shader->attribute_stride, attribute_count, vk_shader->config.attributes,
        vk_shader->config.descriptor_set_count, vk_shader->descriptor_set_layouts, vk_shader->config.stage_count,
        stage_create_infos, viewport, scissor, false, true, shader->push_constant_range_count,
        shader->push_constant_ranges, &vk_shader->pipeline
    );

    if(!pipeline_result)
    {
        kerror("Function '%s': Failed to load graphics pipeline for object shader.", __FUNCTION__);
        return false;
    }

    // TODO: Компиляция и линковка байт-кода SPIR-V в машинный код не произойдет до тех пор, пока не будет создан
    //       графический конвейер. Это значит, что мы можем уничтожить шейдерные модули сразу после создания
    //       конвейера.
    //                                                             Источник: https://habr.com/ru/articles/547576/

    // Требования для выравнивания UBO для физического устройства.
    shader->required_ubo_alignment = context->device.properties.limits.minUniformBufferOffsetAlignment;

    // Получение требуемых величин выравнивания.
    shader->global_ubo_stride = get_aligned(shader->global_ubo_size, shader->required_ubo_alignment);
    shader->ubo_stride = get_aligned(shader->ubo_size, shader->required_ubo_alignment);

    // Создание uniform буфера.
    u32 device_local_bit = context->device.memory_local_host_visible_support ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
    // TODO: Максимальное количество должно быть настраиваемым или должна быть долгосрочная поддержка изменения размера буфера.
    u64 total_buffer_size = shader->global_ubo_stride + (shader->ubo_stride * VULKAN_SHADER_MAX_MATERIAL_COUNT);
    if(!vulkan_buffer_create(
        context, total_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | device_local_bit, 
        true, &vk_shader->uniform_buffer
    ))
    {
        kerror("Function '%s': Failed to create vulkan buffer for object shader.", __FUNCTION__);
        return false;
    }

    // Выделение пространства для глобального UBO, которое должно занимать пространство шага, а не фактически используемый размер.
    if(!vulkan_buffer_allocate(&vk_shader->uniform_buffer, shader->global_ubo_stride, &shader->global_ubo_offset))
    {
        kerror("Function '%s': Failed to allocate space for the uniform buffer!", __FUNCTION__);
        return false;
    }
    
    // Отображение всей памяти буфера.
    vk_shader->uniform_buffer_mapped_block = vulkan_buffer_lock_memory(context, &vk_shader->uniform_buffer, 0, VK_WHOLE_SIZE /*total_buffer_size*/, 0);

    // Выделение глобальных наборов дескрипторов.
    VkDescriptorSetLayout global_layouts[5] = { // TODO: image_count == 5!
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL]
    };

    VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = vk_shader->descriptor_pool;
    allocate_info.descriptorSetCount = 5;       // TODO: image_count == 5!
    allocate_info.pSetLayouts = global_layouts;
    VK_CHECK(vkAllocateDescriptorSets(logical, &allocate_info, vk_shader->global_descriptor_sets));

    return true;
}

bool vulkan_renderer_shader_use(shader* shader)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__))
    {
        return false;
    }

    vulkan_shader* vk_shader = shader->internal_data;
    vulkan_pipeline_bind(
        &context->graphics_command_buffers[context->image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &vk_shader->pipeline
    );
    return true;
}

bool vulkan_renderer_shader_bind_globals(shader* shader)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__))
    {
        return false;
    }

    shader->bound_ubo_offset = shader->global_ubo_offset;
    return true;
}

bool vulkan_renderer_shader_bind_instance(shader* shader, u32 instance_id)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__))
    {
        return false;
    }

    vulkan_shader* vk_shader = shader->internal_data;
    shader->bound_instance_id = instance_id;
    vulkan_shader_instance_state* object_state = &vk_shader->instance_states[instance_id];
    shader->bound_ubo_offset = object_state->offset;
    return true;
}

bool vulkan_renderer_shader_apply_globals(shader* shader)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__))
    {
        return false;
    }

    u32 image_index = context->image_index;
    vulkan_shader* vk_shader = shader->internal_data;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;
    VkDescriptorSet global_descriptor = vk_shader->global_descriptor_sets[image_index];

    // Применние UBO первым.
    VkDescriptorBufferInfo buffer_info;
    buffer_info.buffer = vk_shader->uniform_buffer.handle;
    buffer_info.offset = shader->global_ubo_offset;
    buffer_info.range  = shader->global_ubo_stride;

    // Для обновления наборов дескрипторов.
    VkWriteDescriptorSet ubo_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    ubo_write.dstSet = vk_shader->global_descriptor_sets[image_index];
    ubo_write.dstBinding = 0;
    ubo_write.dstArrayElement = 0;
    ubo_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_write.descriptorCount = 1;
    ubo_write.pBufferInfo = &buffer_info;

    VkWriteDescriptorSet descriptor_writes[2];
    descriptor_writes[0] = ubo_write;

    u32 global_set_binding_count = vk_shader->config.descriptor_sets[DESC_SET_INDEX_GLOBAL].binding_count;
    if(global_set_binding_count > 1)
    {
        // TODO: Написать поддержку сэмплеров.
        global_set_binding_count = 1;
        kerror("Function '%s': Global image samplers are not yet supported.", __FUNCTION__);

        // VkWriteDescriptorSet sampler_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        // descriptor_writes[1] = ...
    }

    vkUpdateDescriptorSets(context->device.logical, global_set_binding_count, descriptor_writes, 0, null);

    // Привязывание глобального набор дескрипторов для обновления.
    vkCmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 0, 1, &global_descriptor, 0, null
    );

    return true;
}

bool vulkan_renderer_shader_apply_instance(shader* shader)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__))
    {
        return false;
    }

    if(!shader->use_instances)
    {
        kerror("Function '%s': This shader does not use instances.", __FUNCTION__);
        return false;
    }

    u32 image_index = context->image_index;
    vulkan_shader* vk_shader = shader->internal_data;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;

    // Получение данных экземпляра.
    vulkan_shader_instance_state* object_state = &vk_shader->instance_states[shader->bound_instance_id];
    VkDescriptorSet object_descriptor_set = object_state->descriptor_set_state.descriptor_sets[image_index];

    // TODO: Если необходимо обновить?
    VkWriteDescriptorSet descriptor_writes[2]; // Всегда максимум 2 набора дескрипторов.
    kzero_tc(descriptor_writes, VkWriteDescriptorSet, 2);
    u32 descriptor_count = 0;
    u32 descriptor_index = 0;

    // Дескриптор 0 - Uniform буфер.
    // Выполнять только если это дескриптор еще не был обновлен.
    u8* instance_ubo_generation = &object_state->descriptor_set_state.descriptor_states[descriptor_index].generations[image_index];
    if(*instance_ubo_generation == INVALID_ID_U8 /* || *global_ubo_generation != material->generation */)
    {
        VkDescriptorBufferInfo buffer_info;
        buffer_info.buffer = vk_shader->uniform_buffer.handle;
        buffer_info.offset = object_state->offset;
        buffer_info.range = shader->ubo_stride;

        VkWriteDescriptorSet ubo_descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        ubo_descriptor.dstSet = object_descriptor_set;
        ubo_descriptor.dstBinding = descriptor_index;
        ubo_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_descriptor.descriptorCount = 1;
        ubo_descriptor.pBufferInfo = &buffer_info;

        descriptor_writes[descriptor_count] = ubo_descriptor;
        descriptor_count++;

        // Обновление генерации.
        *instance_ubo_generation = 1;
    }
    descriptor_index++;

    // Сэмплеры всегда будут в привязке. Если количество привязок меньше 2, то сэмплеров нет.
    if(vk_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].binding_count > 1)
    {
        u32 total_sampler_count = vk_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
        u32 update_sampler_count = 0;
        VkDescriptorImageInfo image_infos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];

        for(u32 i = 0; i < total_sampler_count; ++i)
        {
            texture* t = vk_shader->instance_states[shader->bound_instance_id].instance_textures[i];
            vulkan_texture_data* internal_data = t->internal_data;

            image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[i].imageView = internal_data->image.view;
            image_infos[i].sampler = internal_data->sampler;

            // TODO: Изменить состояние дескриптора.
            // Синхронизируем генерацию кадров, если не используется текстура по умолчанию.
            // if(t->generation != INVALID_ID)
            // {
            //     *descriptor_generation = t->generation;
            //     *descriptor_id = t->id;
            // }

            update_sampler_count++;
        }

        VkWriteDescriptorSet sampler_descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        sampler_descriptor.dstSet = object_descriptor_set;
        sampler_descriptor.dstBinding = descriptor_index;
        sampler_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_descriptor.descriptorCount = update_sampler_count;
        sampler_descriptor.pImageInfo = image_infos;

        descriptor_writes[descriptor_count] = sampler_descriptor;
        descriptor_count++;
    }

    if(descriptor_count > 0)
    {
        vkUpdateDescriptorSets(context->device.logical, descriptor_count, descriptor_writes, 0, null);
    }

    // Привязывание наборов дескрипторов для обновления или в случае изменения шейдера.
    vkCmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 1, 1, &object_descriptor_set, 0, null
    );

    return true;
}

bool vulkan_renderer_shader_acquire_instance_resources(shader* shader, u32* out_instance_id)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__) || !out_instance_id)
    {
        return false;
    }

    vulkan_shader* vk_shader = shader->internal_data;

    *out_instance_id = INVALID_ID;
    for(u32 i = 0; i < 1024; ++i)
    {
        if(vk_shader->instance_states[i].id == INVALID_ID)
        {
            vk_shader->instance_states[i].id = i;
            *out_instance_id = i;
            break;
        }
    }

    if(*out_instance_id == INVALID_ID)
    {
        kerror("Function '%s': Failed to acquire new id.", __FUNCTION__);
        return false;
    }

    vulkan_shader_instance_state* instance_state = &vk_shader->instance_states[*out_instance_id];
    u32 instance_texture_count = vk_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
    instance_state->instance_textures = kallocate_tc(texture*, shader->instance_texture_count, MEMORY_TAG_ARRAY);

    // Устанавка всех указателей текстур на значения по умолчанию, пока они не назначены.
    texture* default_texture = texture_system_get_default_diffuse_texture();
    for(u32 i = 0; i < instance_texture_count; ++i)
    {
        instance_state->instance_textures[i] = default_texture;
    }

    // Выделение места в UBO по шагу, а не по размеру.
    u64 size = shader->ubo_stride;
    if(!vulkan_buffer_allocate(&vk_shader->uniform_buffer, size, &instance_state->offset))
    {
        kerror("Function '%': Failed t oacuire ubo space.", __FUNCTION__);
        return false;
    }

    vulkan_shader_descriptor_set_state* set_state = &instance_state->descriptor_set_state;

    // Привязка набора дескрипторов.
    u32 binding_count = vk_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].binding_count;
    kzero_tc(set_state->descriptor_states, vulkan_descriptor_state, VULKAN_SHADER_MAX_BINDINGS);

    for(u32 i = 0; i < binding_count; ++i)
    {
        for(u32 j = 0; j < 5; ++j)
        {
            set_state->descriptor_states[i].generations[j] = INVALID_ID_U8;
            set_state->descriptor_states[i].ids[j] = INVALID_ID;
        }
    }

    // Выделение набора дескрипторов на кадр.
    VkDescriptorSetLayout layouts[5] = {
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        vk_shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE]
    };

    VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = vk_shader->descriptor_pool;
    allocate_info.descriptorSetCount = 5; // TODO: image_count = 5!
    allocate_info.pSetLayouts = layouts;

    VkResult result = vkAllocateDescriptorSets(
        context->device.logical, &allocate_info, instance_state->descriptor_set_state.descriptor_sets
    );

    if(result != VK_SUCCESS)
    {
        kerror(
            "Function '%s': Failed to allocate instance descriptor sets in shader: '%s'",
            __FUNCTION__, vulkan_result_get_string(result, true)
        );
        return false;
    }

    return true;
}

bool vulkan_renderer_shader_release_instance_resources(shader* shader, u32 instance_id)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__))
    {
        return false;
    }

    vulkan_shader* vk_shader = shader->internal_data;
    vulkan_shader_instance_state* instance_state = &vk_shader->instance_states[instance_id];

    // Ожидание завершения всех операций, использующих набор дескрипторов.
    vkDeviceWaitIdle(context->device.logical);

    // Освобождение 5 набора дескрипторов (по одному на кадр).
    VkResult result = vkFreeDescriptorSets(
        context->device.logical, vk_shader->descriptor_pool, 5, instance_state->descriptor_set_state.descriptor_sets
    );

    if(result != VK_SUCCESS)
    {
        kerror("Function '%s': Failed to free object shader descriptor sets!", __FUNCTION__);
    }

    kzero_tc(instance_state->descriptor_set_state.descriptor_sets, vulkan_descriptor_state, VULKAN_SHADER_MAX_BINDINGS);

    if(instance_state->instance_textures)
    {
        kfree_tc(instance_state->instance_textures, texture*, shader->instance_texture_count, MEMORY_TAG_ARRAY);
        instance_state->instance_textures = null;
    }

    vulkan_buffer_free(&vk_shader->uniform_buffer, shader->ubo_stride, instance_state->offset);
    instance_state->offset = INVALID_ID;
    instance_state->id = INVALID_ID;

    return true;
}

bool vulkan_renderer_shader_set_uniform(shader* shader, struct shader_uniform* uniform, const void* value)
{
    if(!vulkan_renderer_shader_status_valid(shader, __FUNCTION__) || !uniform || !value)
    {
        return false;
    }

    vulkan_shader* vk_shader = shader->internal_data;

    if(uniform->type == SHADER_UNIFORM_TYPE_SAMPLER)
    {
        if(uniform->scope == SHADER_SCOPE_GLOBAL)
        {
            shader->global_textures[uniform->location] = (texture*)value;
        }
        else
        {
            vk_shader->instance_states[shader->bound_instance_id].instance_textures[uniform->location] = (texture*)value;
        }
    }
    else
    {
        if(uniform->scope == SHADER_SCOPE_LOCAL)
        {
            VkCommandBuffer command_buffer = context->graphics_command_buffers[context->image_index].handle;
            vkCmdPushConstants(
                command_buffer, vk_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                uniform->offset, uniform->size, value
            );
        }
        else
        {
            // Отображение соответствующей области памяти и скопирование данных.
            u64 uniform_offset = shader->bound_ubo_offset + uniform->offset;
            void* addr = POINTER_GET_OFFSET(vk_shader->uniform_buffer_mapped_block, uniform_offset);
            kcopy(addr, value, uniform->size);
        }
    }
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
            // TODO: Для целей отладки ошибок!
            kdebug_break();
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

void framebuffers_regenerate(bool first_destroy_buffers)
{
    VkResult result = VK_ERROR_UNKNOWN;
    u32 image_count = context->swapchain.image_count;
    for(u32 i = 0; i < image_count; ++i)
    {
        VkFramebuffer* current_world_framebuffer = &context->world_framebuffers[i];
        VkFramebuffer* current_swapchain_framebuffer = &context->swapchain.framebuffers[i];

        if(first_destroy_buffers)
        {
            vkDestroyFramebuffer(context->device.logical, *current_world_framebuffer, context->allocator);
            vkDestroyFramebuffer(context->device.logical, *current_swapchain_framebuffer, context->allocator);
        }

        VkImageView world_attachments[2] = { context->swapchain.views[i], context->swapchain.depth_attachment.view };
        VkFramebufferCreateInfo world_framebufferinfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        world_framebufferinfo.renderPass = context->main_renderpass.handle;
        world_framebufferinfo.attachmentCount = 2;
        world_framebufferinfo.pAttachments = world_attachments;
        world_framebufferinfo.width = context->framebuffer_width;
        world_framebufferinfo.height = context->framebuffer_height;
        world_framebufferinfo.layers = 1;

        result = vkCreateFramebuffer(context->device.logical, &world_framebufferinfo, context->allocator, current_world_framebuffer);
        if(!vulkan_result_is_success(result))
        {
            kfatal("Failed to create world framebuffer with result: %s", vulkan_result_get_string(result, true));
        }

        // Кардровые буферы цепочки обмена (UI pass).
        VkImageView ui_attachments[1] = { context->swapchain.views[i] };
        VkFramebufferCreateInfo ui_framebufferinfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        ui_framebufferinfo.renderPass = context->ui_renderpass.handle;
        ui_framebufferinfo.attachmentCount = 1;
        ui_framebufferinfo.pAttachments = ui_attachments;
        ui_framebufferinfo.width = context->framebuffer_width;
        ui_framebufferinfo.height = context->framebuffer_height;
        ui_framebufferinfo.layers = 1;

        result = vkCreateFramebuffer(context->device.logical, &ui_framebufferinfo, context->allocator, current_swapchain_framebuffer);
        if(!vulkan_result_is_success(result))
        {
            kfatal("Failed to create swapchain framebuffer with result: %s", vulkan_result_get_string(result, true));
        }
    }
}

void framebuffers_destroy()
{
    u32 image_count = context->swapchain.image_count;
    for(u32 i = 0; i < image_count; ++i)
    {
        vkDestroyFramebuffer(context->device.logical, context->world_framebuffers[i], context->allocator);
        vkDestroyFramebuffer(context->device.logical, context->swapchain.framebuffers[i], context->allocator);
    }
}

void sync_objects_create()
{
    context->image_available_semaphores = darray_reserve(VkSemaphore, context->swapchain.max_frames_in_flight);
    context->queue_complete_semaphores = darray_reserve(VkSemaphore, context->swapchain.max_frames_in_flight);

    for(u8 i = 0; i < context->swapchain.max_frames_in_flight; ++i)
    {
        VkSemaphoreCreateInfo semaphoreinfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        vkCreateSemaphore(
            context->device.logical, &semaphoreinfo, context->allocator, &context->image_available_semaphores[i]
        );

        vkCreateSemaphore(
            context->device.logical, &semaphoreinfo, context->allocator, &context->queue_complete_semaphores[i]
        );

        // Создание fence в сигнальном состоянии, указывающее, что первый кадр уже «отрисован».
        // Это предотвратит бесконечное ожидание приложением отрисовки первого кадра, поскольку
        // он не может быть отрисован, пока не будет «отрисован» кадр до него.
        VkFenceCreateInfo fenceinfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult result = vkCreateFence(context->device.logical, &fenceinfo, context->allocator, &context->in_flight_fences[i]);
        if(!vulkan_result_is_success(result))
        {
            kerror("Function '%s': Failed to create fence with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        }
    }

    // В полете ограждения еще не должны существовать на этом этапе, поэтому очистите список указателей.
    // TODO: Проверить, очищен ли список созданием и очисткой контекста вулкана.
    kzero_tc(context->images_in_flight, VkFence, context->swapchain.image_count);
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

        vkDestroyFence(context->device.logical, context->in_flight_fences[i], context->allocator);
    }

    darray_destroy(context->image_available_semaphores);
    context->image_available_semaphores = null;

    darray_destroy(context->queue_complete_semaphores);
    context->queue_complete_semaphores = null;
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
    kzero_tc(context->images_in_flight, VkFence, context->swapchain.image_count);

    vulkan_swapchain_recreate(context, cached_framebuffer_width, cached_framebuffer_height, &context->swapchain);

    // Синхронизация размера буферов с кэшем.
    context->framebuffer_width = cached_framebuffer_width;
    context->framebuffer_height = cached_framebuffer_height;
    context->main_renderpass.render_area.x = 0;
    context->main_renderpass.render_area.y = 0;
    context->main_renderpass.render_area.width = cached_framebuffer_width;
    context->main_renderpass.render_area.height = cached_framebuffer_height;
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

    // Пересоздание буферов.
    framebuffers_regenerate(true);

    command_buffers_create(backend);

    // Сброс флага пересборки цепочки обмена.
    context->recreating_swapchain = false;
    return true;
}

bool buffers_create(vulkan_context* context)
{
    VkMemoryPropertyFlagBits memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlagBits vertex_usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkBufferUsageFlagBits index_usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    // Буфер вершин.
    const u64 vertex_buffer_size = sizeof(struct vertex_3d) * 1024 * 1024;
    if(!vulkan_buffer_create(context, vertex_buffer_size, vertex_usage_flags, memory_property_flags, true, &context->object_vertex_buffer))
    {
        kerror("Function '%s': Failed to create vertex buffer.", __FUNCTION__);
        return false;
    }

    // Буфер индексов.
    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if(!vulkan_buffer_create(context, index_buffer_size, index_usage_flags, memory_property_flags, true, &context->object_index_buffer))
    {
        kerror("Function '%s': Failed to create index buffer.", __FUNCTION__);
        return false;
    }

    // Отметить все геометрии как недействительные.
    for(u32 i = 0; i < VULKAN_SHADER_MAX_GEOMETRY_COUNT; ++i)
    {
        context->geometries[i].id = INVALID_ID;
    }

    return true;
}

void buffers_destroy(vulkan_context* context)
{
    vulkan_buffer_destroy(context, &context->object_vertex_buffer);
    vulkan_buffer_destroy(context, &context->object_index_buffer);
}

bool upload_data_range(
    VkCommandPool pool, VkFence fence, VkQueue queue, vulkan_buffer* buffer, u64* out_offset, u64 size, const void* data
)
{
    // Выделение памяти в буфере.
    if(!vulkan_buffer_allocate(buffer, size, out_offset))
    {
        kerror("Function '%s': Failed to allocate from the given buffer!", __FUNCTION__);
        return false;
    }
    
    // Создание host-видимого промежуточный буфер для загрузки на устройство.
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkan_buffer staging;
    vulkan_buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, &staging);

    // Загрузка данных в промежуточный буфер.
    vulkan_buffer_load_data(context, &staging, 0, size, 0, data);

    // Загрузка из промежуточного буфера в локальный буфер устройства.
    vulkan_buffer_copy_to(context, pool, fence, queue, staging.handle, 0, buffer->handle, *out_offset, size);

    // Уничтожение промежуточного буфера.
    vulkan_buffer_destroy(context, &staging);

    return true;
}

void free_data_range(vulkan_buffer* buffer, u64 offset, u64 size)
{
    if(!buffer || !size)
    {
        kerror("Function '%s' requires a valid pointer to buffer and szie greater than zero.", __FUNCTION__);
        return;
    }

    vulkan_buffer_free(buffer, size, offset);
}
