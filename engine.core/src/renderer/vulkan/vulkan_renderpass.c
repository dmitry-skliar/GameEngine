// Собственные подключения.
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

void vulkan_renderpass_create(
    vulkan_context* context, vulkan_renderpass* out_renderpass, vec4 render_area, vec4 clear_color, f32 depth,
    u32 stencil, renderpass_clear_flag clear_flags, bool has_prev_pass, bool has_next_pass
)
{
    out_renderpass->render_area = render_area;
    out_renderpass->clear_color = clear_color;
    out_renderpass->depth = depth;
    out_renderpass->stencil = stencil;
    out_renderpass->has_next_pass = has_next_pass;
    out_renderpass->has_prev_pass = has_prev_pass;
    out_renderpass->do_clear_color = (clear_flags & RENDERPASS_CLEAR_COLOR_BUFFER_FLAG) != 0;
    out_renderpass->do_clear_depth = (clear_flags & RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG) != 0;
    out_renderpass->do_clear_stencil = (clear_flags & RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG) != 0;

    // Главный подпроход визуализации.
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Вложения и ссылки на них.
    #define ATTACHMENT_COUNT 2
    #define ATTACHMENT_COLOR_INDEX 0
    #define ATTACHMENT_DEPTH_INDEX 1

    u32 attachment_description_count = 0;
    VkAttachmentDescription attachment_descriptions[ATTACHMENT_COUNT];
    VkAttachmentReference attachment_references[ATTACHMENT_COUNT];

    // Вложение 1: буфер цвета.
    VkAttachmentDescription* color_attachment = &attachment_descriptions[attachment_description_count];
    color_attachment->format = context->swapchain.image_format.format;
    color_attachment->samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment->loadOp = out_renderpass->do_clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;       // Буфер цвета при операции загрузки (до отрисовки).
    color_attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;                                                                   // Буфер цвета при операции сохранения (после отрисовки).
    color_attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                                                          // Буфер трафарета при операции загрузки.
    color_attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                                                        // Буфер трафарета при операции сохранения.
    color_attachment->initialLayout = has_prev_pass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;     // Буфер перед проходм визуализатора.
    color_attachment->finalLayout = has_next_pass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Буфер после прохода визуализатора.
    color_attachment->flags = 0;
    attachment_description_count++;

    // Ссылка 1: буфер цвета.
    attachment_references[ATTACHMENT_COLOR_INDEX].attachment = ATTACHMENT_COLOR_INDEX;                                          // Порядковый индекс буфера в массиве вложений.
    attachment_references[ATTACHMENT_COLOR_INDEX].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;                            // Указание используемого буфера.

    // NOTE: layout(location = 0) out vec4 out_color ссылается на порядковый номер буфера в массиве pColorAttachments.
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_references[ATTACHMENT_COLOR_INDEX]; // TODO: Не совсем коректно!

    // Вложение 2: буфер глубины.
    if(out_renderpass->do_clear_depth)
    {
        VkAttachmentDescription* depth_attachment = &attachment_descriptions[attachment_description_count];
        depth_attachment->format = context->device.depth_format;
        depth_attachment->samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment->loadOp = out_renderpass->do_clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        depth_attachment->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment->finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment->flags = 0;
        attachment_description_count++;

        // Ссылка 2: буфер глубины.
        attachment_references[ATTACHMENT_DEPTH_INDEX].attachment = ATTACHMENT_DEPTH_INDEX;
        attachment_references[ATTACHMENT_DEPTH_INDEX].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.pDepthStencilAttachment = &attachment_references[ATTACHMENT_DEPTH_INDEX];
    }
    else
    {
        // kzero_tc(&attachment_descriptions[ATTACHMENT_DEPTH_INDEX], VkAttachmentDescription, 1);
        subpass.pDepthStencilAttachment = null;
    }

    // TODO: Другие типы вложений и ссылки на них (ввод, показ, ...).
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = null;
    subpass.pResolveAttachments = null;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = null;

    // Зависимости прохода визуализатора.
    // TODO: Сделать настраиваемым.
    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;                                                       // Указывает на неявный подпроход перед визуализатором.
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;                           // Дождаться считывания image цепочкой обмена.
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    // Создания прохода визуализации.
    VkRenderPassCreateInfo renderpassinfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderpassinfo.attachmentCount = attachment_description_count;
    renderpassinfo.pAttachments = attachment_descriptions;
    renderpassinfo.subpassCount = 1;
    renderpassinfo.pSubpasses = &subpass;
    renderpassinfo.dependencyCount = 1;
    renderpassinfo.pDependencies = &dependency;
    renderpassinfo.pNext = null;
    renderpassinfo.flags = 0;

    VkResult result = vkCreateRenderPass(context->device.logical, &renderpassinfo, context->allocator, &out_renderpass->handle);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to create renderpass with result: %s", vulkan_result_get_string(result, true));
    }
}

void vulkan_renderpass_destroy(vulkan_context* context, vulkan_renderpass* renderpass)
{
    if(renderpass && renderpass->handle)
    {
        vkDestroyRenderPass(context->device.logical, renderpass->handle, context->allocator);
        renderpass->handle = null;
    }
}

void vulkan_renderpass_begin(
    vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass, VkFramebuffer frame_buffer
)
{
    VkRenderPassBeginInfo begininfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    begininfo.renderPass = renderpass->handle;
    begininfo.framebuffer = frame_buffer;
    begininfo.renderArea.offset.x = renderpass->render_area.x;
    begininfo.renderArea.offset.y = renderpass->render_area.y;
    begininfo.renderArea.extent.width = renderpass->render_area.width;
    begininfo.renderArea.extent.height = renderpass->render_area.height;

    begininfo.clearValueCount = 0;
    begininfo.pClearValues = 0;

    VkClearValue clear_values[2];
    kzero_tc(clear_values, VkClearValue, 2);

    if(renderpass->do_clear_color)
    {
        kcopy(clear_values[begininfo.clearValueCount].color.float32, renderpass->clear_color.elements, sizeof(f32) * 4);
        begininfo.clearValueCount++;
    }

    if(renderpass->do_clear_depth)
    {
        kcopy(clear_values[begininfo.clearValueCount].color.float32, renderpass->clear_color.elements, sizeof(f32) * 4);
        clear_values[begininfo.clearValueCount].depthStencil.depth = renderpass->depth;
        clear_values[begininfo.clearValueCount].depthStencil.stencil = renderpass->do_clear_stencil ? renderpass->stencil : 0;
        begininfo.clearValueCount++;
    }

    begininfo.pClearValues = begininfo.clearValueCount > 0 ? clear_values : null;

    vkCmdBeginRenderPass(command_buffer->handle, &begininfo, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_IN_RENDERPASS;
}

void vulkan_renderpass_end(vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass)
{
    vkCmdEndRenderPass(command_buffer->handle);
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_RECORDING;
}
