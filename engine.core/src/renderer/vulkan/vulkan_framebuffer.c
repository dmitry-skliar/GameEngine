// Собственные подключения.
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние подключения.
#include "logger.h"
#include "containers/darray.h"
#include "memory/memory.h"

void vulkan_framebuffer_create(
    vulkan_context* context, vulkan_renderpass* renderpass, u32 width, u32 height, u32 attachment_count,
    VkImageView* attachments, vulkan_framebuffer* out_framebuffer
)
{
    // Делаем копию вложений.
    out_framebuffer->attachments = darray_reserve(VkImageView, attachment_count);
    out_framebuffer->attachment_count = attachment_count;
    kmcopy_tc(out_framebuffer->attachments, attachments, VkImageView, attachment_count);

    // Делаем копию указателя визуализатора.
    out_framebuffer->renderpass = renderpass;

    // Создание кадрового буфера.
    VkFramebufferCreateInfo framebufferinfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    framebufferinfo.renderPass = renderpass->handle;
    framebufferinfo.attachmentCount = attachment_count;
    framebufferinfo.pAttachments = out_framebuffer->attachments;
    framebufferinfo.width = width;
    framebufferinfo.height = height;
    framebufferinfo.layers = 1;

    VkResult result = vkCreateFramebuffer(context->device.logical, &framebufferinfo, context->allocator, &out_framebuffer->handle);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to create framebuffer with result: %s", vulkan_result_get_string(result, true));
    }
}

void vulkan_framebuffer_destroy(vulkan_context* context, vulkan_framebuffer* framebuffer)
{
    vkDestroyFramebuffer(context->device.logical, framebuffer->handle, context->allocator);
    framebuffer->handle = null;
    
    if(framebuffer->attachments)
    {
        darray_destroy(framebuffer->attachments);
        framebuffer->attachments = null;
    }

    framebuffer->attachment_count = 0;
    framebuffer->renderpass = null;
}
