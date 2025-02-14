// Cобственные подключения.
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_platform.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "containers/darray.h"
#include "kstring.h"

// Объявления функций.
VkResult vulkan_create_devices_info(renderer_backend* backend, vulkan_context* context, vulkan_device** out_devices);
void vulkan_destroy_devices_info(renderer_backend* backend, vulkan_context* context, vulkan_device* devices);
void vulkan_logging_devices_info(vulkan_context* context, vulkan_device* devices);
VkResult vulkan_select_device(vulkan_context* context, vulkan_device* devices, vulkan_device_requirements* requirements);

VkResult vulkan_device_create(renderer_backend* backend, vulkan_context* context)
{
    // Начальная инициализация.
    context->device.graphics_queue.index = INVALID_ID;
    context->device.compute_queue.index  = INVALID_ID;
    context->device.present_queue.index  = INVALID_ID;
    context->device.transfer_queue.index = INVALID_ID;

    // TODO: Вынести на верхние уровни и сделать настраиваемым.
    vulkan_device_requirements requirements = {0};
    requirements.sampler_anisotropy = true;
    requirements.device_type        = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    requirements.extensions         = darray_create(const char*);
    darray_push(requirements.extensions, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // Поиск всех устройств и сбор по ним информации.
    vulkan_device* devices = null;
    VkResult result = vulkan_create_devices_info(backend, context, &devices);
    if(!vulkan_result_is_success(result))
    {
        return result;
    }

    // Выбор физического устройства в соответствии с требованиями.
    result = vulkan_select_device(context, devices, &requirements);
    if(!vulkan_result_is_success(result))
    {
        return result;
    }
    ktrace("Vulkan physical device obtained.");

    // Вывести отладочную информацию по найденым устройствам.
    vulkan_logging_devices_info(context, devices);

    // NOTE: Для пропуска совместных очередей с общим индексом.
    bool present_shares_graphics_queue = context->device.graphics_queue.index == context->device.present_queue.index;
    bool transfer_shares_graphics_queue = context->device.graphics_queue.index == context->device.transfer_queue.index;
    bool present_must_share_graphics = false;

    // NOTE: Технически возможный предел 32.
    #define QUEUE_INDEX_COUNT 4

    u32 indices[QUEUE_INDEX_COUNT];
    u8 index = 0;

    indices[index++] = context->device.graphics_queue.index;

    if(!present_shares_graphics_queue)
    {
        indices[index++] = context->device.present_queue.index;
    }

    if(!transfer_shares_graphics_queue)
    {
        indices[index++] = context->device.transfer_queue.index;
    }

    // TODO: Сделать конфигурируемой!
    f32 queue_priorities[2] = { 0.9f, 1.0f };
    VkDeviceQueueCreateInfo queueinfo[QUEUE_INDEX_COUNT] = {0};

    for(u32 i = 0; i < index; ++i)
    {
        queueinfo[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueinfo[i].queueFamilyIndex = indices[i];
        queueinfo[i].queueCount = 1;

        if(present_shares_graphics_queue && indices[i] == context->device.graphics_queue.index)
        {
            if(context->device.graphics_queue.count > 1)
            {
                // Совместное использование очередями графики и показа.
                queueinfo[i].queueCount = 2;
            }
            else
            {
                // Нет доступных очередей.
                present_must_share_graphics = true;
            }
        }
        
        // if(indices[i] == context->device.graphics_queue_index)
        // {
        //     queueinfo[i].queueCount = 2;
        // }
        queueinfo[i].flags = 0;
        queueinfo[i].pNext = null;
        queueinfo[i].pQueuePriorities = queue_priorities;
    }

    // Запрос функций устройства.
    // TODO: Сделать настраиваемым конфигурацией.
    VkPhysicalDeviceFeatures features = {0};
    features.samplerAnisotropy = requirements.sampler_anisotropy ? VK_TRUE : VK_FALSE;

    VkDeviceCreateInfo deviceinfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceinfo.queueCreateInfoCount = index;
    deviceinfo.pQueueCreateInfos = queueinfo;
    deviceinfo.pEnabledFeatures = &features;
    deviceinfo.enabledExtensionCount = darray_length(requirements.extensions);
    deviceinfo.ppEnabledExtensionNames = requirements.extensions;

    // Устарело и не используется!
    deviceinfo.enabledLayerCount = 0;
    deviceinfo.ppEnabledLayerNames = null;

    // Создание логического устройства.
    result = vkCreateDevice(context->device.physical, &deviceinfo, context->allocator, &context->device.logical);
    if(!vulkan_result_is_success(result))
    {
        return result;
    }
    ktrace("Vulkan logical device created.");

    // Освобождение используемой памяти.
    darray_destroy(requirements.extensions); // NOTE: Обнуление указателей и данных не нужно, они в стеке!
    vulkan_destroy_devices_info(backend, context, devices);

    u32 present_queue_index = present_must_share_graphics ? 0 : (present_shares_graphics_queue ? 1 : 0);
    vkGetDeviceQueue(context->device.logical, context->device.graphics_queue.index, 0, &context->device.graphics_queue.handle);
    vkGetDeviceQueue(context->device.logical, context->device.present_queue.index, present_queue_index, &context->device.present_queue.handle);
    vkGetDeviceQueue(context->device.logical, context->device.transfer_queue.index, 0, &context->device.transfer_queue.handle);
    ktrace("Vulkan queues obtained.");

    // Создание пула команд для очереди графики.
    VkCommandPoolCreateInfo poolinfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolinfo.queueFamilyIndex = context->device.graphics_queue.index;
    poolinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    // TODO: Создать другие пулы команд.
    result = vkCreateCommandPool(context->device.logical, &poolinfo, context->allocator, &context->device.graphics_queue.command_pool);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to create qraphics command pool with result: %s", vulkan_result_get_string(result, true));
    }
    ktrace("Vulkan command pools created (Now only graphics!).");

    return VK_SUCCESS;
}

void vulkan_device_destroy(renderer_backend* backend, vulkan_context* context)
{
    // Уничтожение пулов команд.
    vkDestroyCommandPool(context->device.logical, context->device.graphics_queue.command_pool, context->allocator);
    context->device.graphics_queue.command_pool = null;
    ktrace("Vulkan command pools destroyed.");

    // Освобождение указателей на очереди.
    context->device.graphics_queue.handle = null;
    context->device.present_queue.handle = null;
    context->device.transfer_queue.handle = null;
    ktrace("Vulkan queues released.");

    // Уничтожение логического устройства.
    if(context->device.logical)
    {
        vkDestroyDevice(context->device.logical, context->allocator);
        context->device.logical = null;
        ktrace("Vulkan logical device destroyed.");
    }

    // Освобождение ресурсов физического устройства.
    if(context->device.physical)
    {
        // Уничтожение информации о цепочке обмена!
        vulkan_device_destroy_swapchian_support(&context->device.swapchain_support);

        // Уничтожение информации об устройстве.
        kzero_tc(&context->device, vulkan_device, 1);
        context->device.graphics_queue.index = INVALID_ID;
        context->device.compute_queue.index  = INVALID_ID;
        context->device.present_queue.index  = INVALID_ID;
        context->device.transfer_queue.index = INVALID_ID;
        ktrace("Vulkan physical device released.");
    }
}

void vulkan_device_query_swapchain_support(VkPhysicalDevice physical, VkSurfaceKHR surface, vulkan_device_swapchain_support* out_swapchain_support)
{
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &out_swapchain_support->capabilities);
    if(!vulkan_result_is_success(result))
    {
        kerror("Failed to get surface capabilities with code: %s", vulkan_result_get_string(result, true));
        return;
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &out_swapchain_support->format_count, null);
    if(!vulkan_result_is_success(result))
    {
        kerror("Failed to get surface formats with code: %s", vulkan_result_get_string(result, true));
        return;
    }

    if(out_swapchain_support->format_count == 0)
    {
        kerror("Vulkan surface does not support formats!");
        return;
    }

    if(out_swapchain_support->formats)
    {
        kfatal("Vulkan device swapchain support formats array is not null!");
    }

    out_swapchain_support->formats = darray_reserve(VkSurfaceFormatKHR, out_swapchain_support->format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &out_swapchain_support->format_count, out_swapchain_support->formats);

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &out_swapchain_support->present_mode_count, null);
    if(!vulkan_result_is_success(result))
    {
        kerror("Failed to get surface present modes with code: %s", vulkan_result_get_string(result, true));
        return;
    }

    if(out_swapchain_support->present_mode_count == 0)
    {
        kerror("Vulkan surface does not support present modes!");
        return;
    }

    if(out_swapchain_support->present_modes)
    {
        kfatal("Vulkan device swapchain support present modes array is not null!");
    }

    out_swapchain_support->present_modes = darray_reserve(VkPresentModeKHR, out_swapchain_support->present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &out_swapchain_support->present_mode_count, out_swapchain_support->present_modes);
}

void vulkan_device_destroy_swapchian_support(vulkan_device_swapchain_support* swapchain_support)
{
    if(swapchain_support->formats)
    {
        darray_destroy(swapchain_support->formats);
    }

    if(swapchain_support->present_modes)
    {
        darray_destroy(swapchain_support->present_modes);
    }

    kzero_tc(swapchain_support, vulkan_device_swapchain_support, 1);
}

VkResult vulkan_create_devices_info(renderer_backend* backend, vulkan_context* context, vulkan_device** out_devices)
{
    u32 physical_device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(context->instance, &physical_device_count, null);
    if(!vulkan_result_is_success(result))
    {
        return result;
    }

    if(physical_device_count == 0)
    {
        kerror("Failed to found devices which support Vulkan!");
        return VK_ERROR_UNKNOWN;
    }

    VkPhysicalDevice* physical_devices = darray_reserve(VkPhysicalDevice, physical_device_count);
    vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices);

    // Создание массива с информацие об устройствах.
    vulkan_device* devices = darray_reserve(vulkan_device, physical_device_count);
    kzero_tc(devices, vulkan_device, physical_device_count);

    // Сбор информации по каждому устройству.
    for(u32 i = 0; i < physical_device_count; ++i)
    {
        // NOTE: Внимание заполнение динамического массива таким образом не изменяет его длинну!
        devices[i].physical = physical_devices[i];
        vkGetPhysicalDeviceProperties(physical_devices[i], &devices[i].properties);
        vkGetPhysicalDeviceFeatures(physical_devices[i], &devices[i].features);
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &devices[i].memory);

        // Создание массива с информацией по очередям устройства.
        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, null);
        VkQueueFamilyProperties* queue_families = darray_reserve(VkQueueFamilyProperties, queue_family_count);
        kzero_tc(queue_families, VkQueueFamilyProperties, queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_families);

        u32 graphics_family_index = INVALID_ID;
        u32 present_family_index  = INVALID_ID;
        u32 compute_family_index  = INVALID_ID;
        u32 transfer_family_index = INVALID_ID;
        u32 graphics_family_count =  0;
        u32 present_family_count  =  0;
        u32 compute_family_count  =  0;
        u32 transfer_family_count =  0;
        u8  min_transfer_score    = INVALID_ID;

        // Просмотр поддерживаемых очередей.
        // TODO: Пересмотреть алгоритм!
        for(u32 j = 0; j < queue_family_count; ++j)
        {
            u8 current_transfer_score = 0;
            u32 family_count = queue_families[j].queueCount;

            // Графическая очередь?
            if(graphics_family_index == INVALID_ID && queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphics_family_index = j;
                graphics_family_count = family_count;
                ++current_transfer_score;

                // Очередь представления изображений?
                if(platform_window_get_vulkan_presentation_support(backend->window_state, physical_devices[i], j))
                {
                    present_family_index = j;
                    present_family_count = family_count;
                    ++current_transfer_score;
                }
            }

            // Вычислительная очередь?
            if(queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                compute_family_index = j;
                compute_family_count = family_count;
                ++current_transfer_score;
            }

            // Очередь операций копирования?
            if(queue_families[j].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                // NOTE: Минимальный индекс увеличит шанс, что это будет выделенная очередь!
                if(current_transfer_score <= min_transfer_score)
                {
                    min_transfer_score = current_transfer_score;
                    transfer_family_index = j;
                    transfer_family_count = family_count;
                }
            }

            // Очередь представления изображений?
            // NOTE: Если графическа очередь не поддерживает ищем первую попавшуюся!
            bool present_support_by_platform =
                platform_window_get_vulkan_presentation_support(backend->window_state, physical_devices[i], j);
            if(present_family_index == INVALID_ID && present_support_by_platform)
            {
                present_family_index = j;
                present_family_count = family_count;
            }
        }

        // Сохранение индексов.
        devices[i].graphics_queue.index = graphics_family_index;
        devices[i].compute_queue.index  = compute_family_index;
        devices[i].present_queue.index  = present_family_index;
        devices[i].transfer_queue.index = transfer_family_index;

        // Сохранения количества очередей.
        devices[i].graphics_queue.count = graphics_family_count;
        devices[i].compute_queue.count  = compute_family_count;
        devices[i].present_queue.count  = present_family_count;
        devices[i].transfer_queue.count = transfer_family_count;

        // Очистка используемой памяти очередью.
        darray_destroy(queue_families);

        // Поддержка видимости памяти графического устройства.
        for(u32 j = 0; j < devices[i].memory.memoryTypeCount; ++j)
        {
            // Проветка каждого типа памяти на видимость.
            if((devices[i].memory.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0
            && (devices[i].memory.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
            {
                devices[i].memory_local_host_visible_support = true;
                break;
            }
        }

        vulkan_device_query_swapchain_support(devices[i].physical, context->surface, &devices[i].swapchain_support);
    }

    // Очистка используемой памяти временным массивом устройств.
    darray_destroy(physical_devices);

    ktrace("Vulkan physical devices info created.");
    *out_devices = devices;
    return VK_SUCCESS;
}

void vulkan_destroy_devices_info(renderer_backend* backend, vulkan_context* context, vulkan_device* devices)
{
    u32 physical_device_count = darray_capacity(devices);

    for(u32 i = 0; i < physical_device_count; ++i)
    {
        if(context->device.physical == devices[i].physical)
        {
            continue;
        }

        vulkan_device_destroy_swapchian_support(&devices[i].swapchain_support);
    }

    // Уничтожение массива с информацией об устройствах.
    darray_destroy(devices);

    ktrace("Vulkan physical devices info destroyed.");
}

void vulkan_logging_devices_info(vulkan_context* context, vulkan_device* devices)
{
    // NOTE: Массив devices заполнялся без использования функций darray_push и darray_push_at!
    u32 device_count = darray_capacity(devices);
    const char* gpu_names[] = { "unknown", "integrated", "discrete", "virtual", "host cpu" };
    const char* is_support[] = { "YES", "NO" };
    u32 gpu_index = INVALID_ID;

    for(u32 i = 0; i < device_count; ++i)
    {
        u32 gpu_type_index = devices[i].properties.deviceType;
        if(gpu_type_index > 4) gpu_type_index = 0;

        const char* dev_name = devices[i].properties.deviceName;

        u32 drv_major = VK_VERSION_MAJOR(devices[i].properties.driverVersion);
        u32 drv_minor = VK_VERSION_MINOR(devices[i].properties.driverVersion);
        u32 drv_patch = VK_VERSION_PATCH(devices[i].properties.driverVersion);

        u32 qi_graphics = devices[i].graphics_queue.index;
        u32 qi_present  = devices[i].present_queue.index;
        u32 qi_compute  = devices[i].compute_queue.index;
        u32 qi_transfer = devices[i].transfer_queue.index;

        u32 qc_graphics = devices[i].graphics_queue.count;
        u32 qc_present  = devices[i].present_queue.count;
        u32 qc_compute  = devices[i].compute_queue.count;
        u32 qc_transfer = devices[i].transfer_queue.count;

        const char* graphics_support = qi_graphics != INVALID_ID ? is_support[0]: is_support[1];
        const char* present_support  = qi_present  != INVALID_ID ? is_support[0]: is_support[1];
        const char* compute_support  = qi_compute  != INVALID_ID ? is_support[0]: is_support[1];
        const char* transfer_support = qi_transfer != INVALID_ID ? is_support[0]: is_support[1];

        kdebug("[%2d] GPU Type: %s", i, gpu_names[gpu_type_index]);
        kdebug("[%2d] GPU Name: %s (driver ver. %d.%d.%d)", i, dev_name, drv_major, drv_minor, drv_patch);

        kdebug("[%2d] GPU Graphics queue %3s family: index %2d, count %2d", i, graphics_support, qi_graphics, qc_graphics);
        kdebug("[%2d] GPU Present queue  %3s family: index %2d, count %2d", i, present_support, qi_present, qc_present);
        kdebug("[%2d] GPU Compute queue  %3s family: index %2d, count %2d", i, compute_support, qi_compute, qc_compute);
        kdebug("[%2d] GPU Transfer queue %3s family: index %2d, count %2d", i, transfer_support, qi_transfer, qc_transfer);

        for(u32 j = 0; j < devices[i].memory.memoryHeapCount; ++j)
        {
            f32 mem_size = (((f32)devices[i].memory.memoryHeaps[j].size) / 1024.0f / 1024.0f);

            if(devices[i].memory.memoryHeaps[j].flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                kdebug("[%2d] GPU Local memory: %0.2f MiB", i, mem_size);
            }
            else
            {
                kdebug("[%2d] GPU Shared memory: %0.2f MiB", i, mem_size);
            }
        }

        if(devices[i].memory_local_host_visible_support)
        {
            kdebug("[%2d] GPU local host visible memory support", i);
        }

        if(devices[i].physical == context->device.physical)
        {
            gpu_index = i;
        }
    }

    ktrace("GPU index selected: %d", gpu_index);
}

VkResult vulkan_select_device(vulkan_context* context, vulkan_device* devices, vulkan_device_requirements* requirements)
{
    // NOTE: Массив devices заполнялся без использования функций darray_push и darray_push_at!
    u32 physical_device_count = darray_capacity(devices);
    u32 best_score_device     = 0;
    u32 index_physical_device = INVALID_ID;

    for(u32 i = 0; i < physical_device_count; ++i)
    {
        u32 current_score_device;

        switch(devices[i].properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                current_score_device = 1;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                current_score_device = 4;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                current_score_device = 5;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                current_score_device = 3;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                current_score_device = 2;
                break;
            default:
                kwarng("Vulkan device properties: unknown device type! Skipping...");
                continue;
        }

        // Проверка на соответствие типа предпочитаемой видеокарты.
        if((requirements->device_type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU   && current_score_device != 5)
        || (requirements->device_type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && current_score_device != 4))
        {
            kwarng("Vulkan device type does not meet requirements! Skipping...");
            continue;
        }

        // Проверка на соответствие поддержки запрашиваемых очередей.
        if(devices[i].graphics_queue.index == INVALID_ID
        || devices[i].present_queue.index  == INVALID_ID
        || devices[i].transfer_queue.index == INVALID_ID)
        {
            kwarng("Vulkan device queues does not meet requirements! Skipping...");
            continue;
        }

        // Проверка поддержки запрашиваемых расширений.
        if(requirements->extensions)
        {
            u32 available_extension_count = 0;
            VkResult result = vkEnumerateDeviceExtensionProperties(devices[i].physical, null, &available_extension_count, null);
            if(!vulkan_result_is_success(result))
            {
                return result;
            }

            if(available_extension_count == 0)
            {
                kwarng("Extensions for Vulkan devices are not provided. Skipping...");
                continue;
            }

            VkExtensionProperties* available_extensions = darray_reserve(VkExtensionProperties, available_extension_count);
            vkEnumerateDeviceExtensionProperties(devices[i].physical, null, &available_extension_count, available_extensions);

            u32 required_extension_count = darray_length(requirements->extensions);
            bool required_extension_not_supported = false;

            // TODO: Выделить в отдельную функцию сравнения динамических массивов и статических!
            // TODO: Использовать статический массив! Для vulkan_backend тоже!
            for(u32 j = 0; j < required_extension_count; ++j)
            {
                bool found = false;

                for(u32 k = 0; k < available_extension_count; ++k)
                {
                    if(string_equal(requirements->extensions[j], available_extensions[k].extensionName))
                    {
                        found = true;
                        break;
                    }
                }

                if(!found)
                {
                    kwarng("Vulkan device extension '%s' not supported. Skipping...", requirements->extensions[i]);
                    required_extension_not_supported = true;
                    break;
                }
            }

            if(required_extension_not_supported)
            {
                continue;
            }

            // Очистка используемой памяти.
            darray_destroy(available_extensions);
        }

        // Проверка поддержки анизотропной фильтрации.
        if(requirements->sampler_anisotropy && !devices[i].features.samplerAnisotropy)
        {
            kwarng("Vulkan device does not support sampler anisotropy. Skipping...");
            continue;
        }

        // Проверка поддерживаемых режимов показов и формата поверхнисти.
        if(devices[i].swapchain_support.format_count < 1 || devices[i].swapchain_support.present_mode_count < 1)
        {
            kwarng("Vulkan device does not support surface formats or present modes of swapchain. Skipping...");
            continue;
        }

        // Проверка на лучший выбор.
        if(current_score_device > best_score_device)
        {
            index_physical_device = i;
            best_score_device = current_score_device;
        }
    }

    if(index_physical_device != INVALID_ID)
    {
        kcopy_tc(&context->device, &devices[index_physical_device], vulkan_device, 1);
        return VK_SUCCESS;
    }

    kerror("No devices which support requirements were found!");
    return VK_ERROR_UNKNOWN;
}

bool vulkan_device_detect_depth_format(vulkan_device* device)
{
    #define CANDIDATE_COUNT 3

    // Предложенные форматы.
    VkFormat candidates[CANDIDATE_COUNT] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for(u64 i = 0; i < CANDIDATE_COUNT; ++i)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physical, candidates[i], &properties);

        if((properties.linearTilingFeatures & flags) == flags)
        {
            device->depth_format = candidates[i];
            return true;
        }
        else if((properties.optimalTilingFeatures & flags) == flags)
        {
            device->depth_format = candidates[i];
            return true;
        }
    }

    return false;
}
