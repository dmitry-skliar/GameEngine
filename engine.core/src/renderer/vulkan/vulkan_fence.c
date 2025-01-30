// Собственные подключения.
#include "renderer/vulkan/vulkan_fence.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние подключения.
#include "logger.h"

void vulkan_fence_create(vulkan_context* context, bool create_signaled, vulkan_fence* out_fence)
{
    // Делаем по умолчанию в сигнальное состояние.
    out_fence->is_signaled = create_signaled;

    VkFenceCreateInfo fenceinfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceinfo.pNext = null;
    fenceinfo.flags = create_signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkResult result = vkCreateFence(context->device.logical, &fenceinfo, context->allocator, &out_fence->handle);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to create fence with result: %s", vulkan_result_get_string(result, true));
    }
}

void vulkan_fence_destroy(vulkan_context* context, vulkan_fence* fence)
{
    if(fence->handle)
    {
        vkDestroyFence(context->device.logical, fence->handle, context->allocator);
        fence->handle = null;
    }

    fence->is_signaled = false;
}

bool vulkan_fence_wait(vulkan_context* context, vulkan_fence* fence, u64 timeout_ns)
{
    if(!fence->is_signaled)
    {
        VkResult result = vkWaitForFences(context->device.logical, 1, &fence->handle, true, timeout_ns);

        switch(result)
        {
            case VK_SUCCESS:
                fence->is_signaled = true;
                return true;
            case VK_TIMEOUT:
                kwarng("FenceWait - Timeout.");
                break;
            case VK_ERROR_DEVICE_LOST:
                kerror("FenceWait - Error device lost.");
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                kerror("FenceWait - Error out of host memory.");
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                kerror("FenceWait - Error out of device memory.");
                break;
            default:
                kerror("FenceWait - An unknown error has occurred.");
                break;
        }
    }
    else
    {
        // Если уже сигнальное состояние, ждать не нужно.
        return true;
    }

    return false;
}

void vulkan_fence_reset(vulkan_context* context, vulkan_fence* fence)
{
    if(fence->is_signaled)
    {
        VkResult result = vkResetFences(context->device.logical, 1, &fence->handle);
        if(!vulkan_result_is_success(result))
        {
            kfatal("Failed to reset fence with result: %s", vulkan_result_get_string(result, true));
        }

        fence->is_signaled = false;
    }
}
