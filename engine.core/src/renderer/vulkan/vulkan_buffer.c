// Собственные подключения.
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_command_buffer.h"
#include "renderer/vulkan/vulkan_utils.h"


// Internal includies.
#include "logger.h"
#include "memory/memory.h"

bool vulkan_buffer_create(
    vulkan_context* context, u64 size, VkBufferUsageFlagBits usage, u32 memory_property_flags,
    bool bind_on_create, vulkan_buffer* out_buffer
)
{
    kzero_tc(out_buffer, vulkan_buffer, 1);
    out_buffer->total_size = size;
    out_buffer->usage = usage;
    out_buffer->memory_property_flags = memory_property_flags;

    VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // NOTE: Используется только в одной очереди.

    VkResult result = vkCreateBuffer(context->device.logical, &buffer_info, context->allocator, &out_buffer->handle);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to create buffer with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

    // Получение требований памяти.
    VkMemoryRequirements requirements = {0};
    vkGetBufferMemoryRequirements(context->device.logical, out_buffer->handle, &requirements);
    out_buffer->memory_index = context->find_memory_index(requirements.memoryTypeBits, out_buffer->memory_property_flags);
    if(out_buffer->memory_index == INVALID_ID)
    {
        kerror("Function '%s': Failed to find required memory type index.", __FUNCTION__);
        return false;
    }

    // Выделение памяти.
    VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = (u32)out_buffer->memory_index;

    result = vkAllocateMemory(context->device.logical, &allocate_info, context->allocator, &out_buffer->memory);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to allocate memory with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

    if(bind_on_create)
    {
        vulkan_buffer_bind(context, out_buffer, 0);
    }

    return true;
}

void vulkan_buffer_destroy(vulkan_context* context, vulkan_buffer* buffer)
{
    if(buffer->memory)
    {
        vkFreeMemory(context->device.logical, buffer->memory, context->allocator);
    }

    if(buffer->handle)
    {
        vkDestroyBuffer(context->device.logical, buffer->handle, context->allocator);
    }

    kzero_tc(buffer, struct vulkan_buffer, 1);
}

bool vulkan_buffer_resize(vulkan_context* context, u64 new_size, vulkan_buffer* buffer, VkQueue queue, VkCommandPool pool)
{
    // Создание нового буфера.
    VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size = new_size;
    buffer_info.usage = buffer->usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // NOTE: Используется только в одной очереди.

    VkBuffer new_buffer = null;
    VkResult result = vkCreateBuffer(context->device.logical, &buffer_info, context->allocator, &new_buffer);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to create new buffer with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

    // Получение требований памяти.
    VkMemoryRequirements requirements = {0};
    vkGetBufferMemoryRequirements(context->device.logical, new_buffer, &requirements);

    // Выделение новой памяти.
    VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = (u32)buffer->memory_index;

    VkDeviceMemory new_memory = null;
    result = vkAllocateMemory(context->device.logical, &allocate_info, context->allocator, &new_memory);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to allocate new memory with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

    // Привязывание новой памяти.
    result = vkBindBufferMemory(context->device.logical, new_buffer, new_memory, 0);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to bind buffer memory with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

    // Копирование данных.
    vulkan_buffer_copy_to(context, pool, null, queue, buffer->handle, 0, new_buffer, 0, buffer->total_size);

    // Ожидание, новый буфер используется.
    vkDeviceWaitIdle(context->device.logical);

    // Уничтожение старого буфера.
    if(buffer->memory)
    {
        vkFreeMemory(context->device.logical, buffer->memory, context->allocator);
        buffer->memory = null;
    }

    if(buffer->handle)
    {
        vkDestroyBuffer(context->device.logical, buffer->handle, context->allocator);
        buffer->handle = null;
    }

    // Установка новых значений.
    buffer->total_size = new_size;
    buffer->memory = new_memory;
    buffer->handle = new_buffer;

    return true;
}

void vulkan_buffer_bind(vulkan_context* context, vulkan_buffer* buffer, u64 offset)
{
    VkResult result = vkBindBufferMemory(context->device.logical, buffer->handle, buffer->memory, offset);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to bind buffer memory with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
    }
}

void* vulkan_buffer_lock_memory(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags)
{
    void* data = null;
    VkResult result = vkMapMemory(context->device.logical, buffer->memory, offset, size, flags, &data);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to map memory with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
    }
    return data;
}

void vulkan_buffer_unlock_memory(vulkan_context* context, vulkan_buffer* buffer)
{
    vkUnmapMemory(context->device.logical, buffer->memory);    
}

void vulkan_buffer_load_data(
    vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags, const void* data
)
{
    void* data_ptr = null;
    VkResult result = vkMapMemory(context->device.logical, buffer->memory, offset, size, flags, &data_ptr);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to map memory with result %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return;
    }
    kcopy(data_ptr, data, size);
    vkUnmapMemory(context->device.logical, buffer->memory);
}

void vulkan_buffer_copy_to(
    vulkan_context* context, VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer source, u64 source_offset,
    VkBuffer destination, u64 destination_offset, u64 size
)
{
    vkQueueWaitIdle(queue);

    // Создать одноразовый командный буфер.
    vulkan_command_buffer command_buffer = {0};
    vulkan_command_buffer_allocate_and_begin_single_use(context, pool, &command_buffer);

    // Подготовка команды копирования и добавление ее в командный буфер.
    VkBufferCopy copy_region = {0};
    copy_region.srcOffset = source_offset;
    copy_region.dstOffset = destination_offset;
    copy_region.size = size;

    vkCmdCopyBuffer(command_buffer.handle, source, destination, 1, &copy_region);

    // Отправка буфер для выполнения и ожидание его завершения.
    vulkan_command_buffer_end_single_use(context, pool, &command_buffer, queue);
}
