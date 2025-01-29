// Собственные подключения.
#include "renderer/vulkan/vulkan_command_buffer.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

void vulkan_command_buffer_allocate(
    vulkan_context* context, VkCommandPool pool, bool is_primary, vulkan_command_buffer* out_command_buffer
)
{
    if(out_command_buffer)
    {
        kmzero_tc(out_command_buffer, vulkan_command_buffer, 1);
    }

    VkCommandBufferAllocateInfo allocateinfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateinfo.commandPool = pool;
    allocateinfo.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocateinfo.commandBufferCount = 1;
    allocateinfo.pNext = null;

    out_command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED;

    VkResult result = vkAllocateCommandBuffers(context->device.logical, &allocateinfo, &out_command_buffer->handle);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to allocate command buffer memory with result: %s", vulkan_result_get_string(result, true));
    }

    out_command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_free(vulkan_context* context, VkCommandPool pool, vulkan_command_buffer* command_buffer)
{
    vkFreeCommandBuffers(context->device.logical, pool, 1, &command_buffer->handle);
    command_buffer->handle = null;
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void vulkan_command_buffer_begin(
    vulkan_command_buffer* command_buffer, bool is_single_use, bool is_renderpass_continue, bool is_simultaneous_use
)
{
    VkCommandBufferBeginInfo begininfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begininfo.flags = 0;
    begininfo.pNext = null;
    begininfo.pInheritanceInfo = null;

    if(is_single_use)
    {
        begininfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }

    if(is_renderpass_continue)
    {
        begininfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }

    if(is_simultaneous_use)
    {
        begininfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VkResult result = vkBeginCommandBuffer(command_buffer->handle, &begininfo);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to start execution of buffer command with result: %s", vulkan_result_get_string(result, true));
    }

    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_RECORDING;
}

void vulkan_command_buffer_end(vulkan_command_buffer* command_buffer)
{
    VkResult result = vkEndCommandBuffer(command_buffer->handle);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to stop execution of buffer command with result: %s", vulkan_result_get_string(result, true));
    }
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_RECORDING_ENDED;
}

void vulkan_command_buffer_update_submitted(vulkan_command_buffer* command_buffer)
{
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_SUBMITTED;
}

void vulkan_command_buffer_reset(vulkan_command_buffer* command_buffer)
{
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_allocate_and_begin_single_use(
    vulkan_context* context, VkCommandPool pool, vulkan_command_buffer* out_command_buffer
)
{
    vulkan_command_buffer_allocate(context, pool, true, out_command_buffer);
    vulkan_command_buffer_begin(out_command_buffer, true, false, false);
}

void vulkan_command_buffer_end_single_use(
    vulkan_context* context, VkCommandPool pool, vulkan_command_buffer* command_buffer, VkQueue queue
)
{
    vulkan_command_buffer_end(command_buffer);

    VkSubmitInfo submitinfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitinfo.commandBufferCount = 1;
    submitinfo.pCommandBuffers = &command_buffer->handle;

    VkResult result = vkQueueSubmit(queue, 1, &submitinfo, null);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to sumbit queue with result: %s", vulkan_result_get_string(result, true));
    }

    // Ожидание окончания операции.
    result = vkQueueWaitIdle(queue);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to wait idle of queue with result: %s", vulkan_result_get_string(result, true));
    }

    vulkan_command_buffer_free(context, pool, command_buffer);
}

