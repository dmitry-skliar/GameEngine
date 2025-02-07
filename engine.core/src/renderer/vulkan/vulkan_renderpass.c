// Собственные подключения.
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

void vulkan_renderpass_create(
    vulkan_context* context, vulkan_renderpass* out_renderpass, f32 x, f32 y, f32 w, f32 h, f32 r, f32 g, f32 b,
    f32 a, f32 depth, u32 stencil
)
{
    // Инициализация размера.
    out_renderpass->x = x;
    out_renderpass->y = y;
    out_renderpass->w = w;
    out_renderpass->h = h;

    // Инициализация цвета.
    out_renderpass->r = r;
    out_renderpass->g = g;
    out_renderpass->b = b;
    out_renderpass->a = a;
    
    out_renderpass->depth = depth;
    out_renderpass->stencil = stencil;

    #define ATTACHMENT_COUNT 2
    #define ATTACHMENT_COLOR_INDEX 0
    #define ATTACHMENT_DEPTH_INDEX 1

    // Вложения.
    // TODO: Сделать настраиваемым.
    VkAttachmentDescription attachment_descriptions[ATTACHMENT_COUNT] = {0};

    // Вложение 1: буфер цвета.
    // TODO: Сделать настраиваемым.
    attachment_descriptions[ATTACHMENT_COLOR_INDEX].format = context->swapchain.image_format.format;
    attachment_descriptions[ATTACHMENT_COLOR_INDEX].samples = VK_SAMPLE_COUNT_1_BIT;                   // Количество бит сэмпла.
    attachment_descriptions[ATTACHMENT_COLOR_INDEX].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;              // Очистить буфер цвета при операции загрузки (до отрисовки).
    attachment_descriptions[ATTACHMENT_COLOR_INDEX].storeOp = VK_ATTACHMENT_STORE_OP_STORE;            // Сохранить буфер цвета при операции сохранения (после отрисовки).
    attachment_descriptions[ATTACHMENT_COLOR_INDEX].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // Не интересуте буфер трафарета при операции загрузки.
    attachment_descriptions[ATTACHMENT_COLOR_INDEX].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Не интересуте буфер трафарета при операции сохранения.
    attachment_descriptions[ATTACHMENT_COLOR_INDEX].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;         // Состояние буфера перед проходм визуализатора не интересует!
    attachment_descriptions[ATTACHMENT_COLOR_INDEX].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;     // Переходить к показу на экран после визуализации!
    attachment_descriptions[ATTACHMENT_COLOR_INDEX].flags = 0;

    // Вложение 2: буфер глубины.
    attachment_descriptions[ATTACHMENT_DEPTH_INDEX].format = context->device.depth_format;
    attachment_descriptions[ATTACHMENT_DEPTH_INDEX].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descriptions[ATTACHMENT_DEPTH_INDEX].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descriptions[ATTACHMENT_DEPTH_INDEX].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[ATTACHMENT_DEPTH_INDEX].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descriptions[ATTACHMENT_DEPTH_INDEX].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[ATTACHMENT_DEPTH_INDEX].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descriptions[ATTACHMENT_DEPTH_INDEX].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachment_descriptions[ATTACHMENT_DEPTH_INDEX].flags = 0;

    // TODO: Другие типы вложений (ввод, показ, ...).

    // Ссылки на вложения.
    VkAttachmentReference attachment_references[ATTACHMENT_COUNT] = {0};

    // Ссылка 1: буфер цвета.
    attachment_references[ATTACHMENT_COLOR_INDEX].attachment = ATTACHMENT_COLOR_INDEX;                 // Порядковый индекс буфера в массиве вложений.
    attachment_references[ATTACHMENT_COLOR_INDEX].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   // Указание используемого буфера (высокая производительность).

    // Ссылка 2: буфер глубины.
    attachment_references[ATTACHMENT_DEPTH_INDEX].attachment = ATTACHMENT_DEPTH_INDEX;
    attachment_references[ATTACHMENT_DEPTH_INDEX].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 

    // TODO: Другие ссылки на вложения (ввод, показ, ...).

    // Главный подпроход визуализации.
    // NOTE: layout(location = 0) out vec4 out_color ссылается на порядковый номер буфера в массиве pColorAttachments.
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_references[ATTACHMENT_COLOR_INDEX];                        // Буфер цветов. 
    subpass.pDepthStencilAttachment = &attachment_references[ATTACHMENT_DEPTH_INDEX];                  // Буфер глубины.
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = null;                                                                  // Буферы содержимого из шейдера.
    subpass.pResolveAttachments = null;                                                                // Буферы цветов с мультисэмплингом.
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
    renderpassinfo.attachmentCount = ATTACHMENT_COUNT;
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
    begininfo.renderArea.offset.x = renderpass->x;
    begininfo.renderArea.offset.y = renderpass->y;
    begininfo.renderArea.extent.width = renderpass->w;
    begininfo.renderArea.extent.height = renderpass->h;

    VkClearValue clear_values[2];
    kzero_tc(clear_values, VkClearValue, 2);
    // Color.
    clear_values[0].color.float32[0] = renderpass->r;
    clear_values[0].color.float32[1] = renderpass->g;
    clear_values[0].color.float32[2] = renderpass->b;
    clear_values[0].color.float32[3] = renderpass->a;
    // Depth.
    clear_values[1].depthStencil.depth = renderpass->depth;
    clear_values[1].depthStencil.stencil = renderpass->stencil;

    begininfo.clearValueCount = 2;
    begininfo.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer->handle, &begininfo, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_IN_RENDERPASS;
}

void vulkan_renderpass_end(vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass)
{
    vkCmdEndRenderPass(command_buffer->handle);
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_RECORDING;
}
