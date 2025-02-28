// Собственные подключения.
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_utils.h"

// Internal includies.
#include "logger.h"
#include "memory/memory.h"

bool vulkan_graphics_pipeline_create(
    vulkan_context* context, vulkan_renderpass* renderpass, u32 stride, u32 attribute_count, 
    VkVertexInputAttributeDescription* attributes, u32 descriptor_set_layout_count, 
    VkDescriptorSetLayout* descriptor_set_layouts, u32 stage_count,
    VkPipelineShaderStageCreateInfo* stages, VkViewport viewport, VkRect2D scissor, bool is_wireframe,
    bool depth_test_enabled, u32 push_constant_range_count, range* push_constant_ranges, vulkan_pipeline* out_pipeline
)
{
    // Область экрана.
    VkPipelineViewportStateCreateInfo viewport_stage = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewport_stage.viewportCount = 1;
    viewport_stage.pViewports = &viewport;
    viewport_stage.scissorCount = 1;
    viewport_stage.pScissors = &scissor;

    // Растерезующий шейдер (растерезатор).
    VkPipelineRasterizationStateCreateInfo rasterizer_info = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer_info.depthClampEnable = VK_FALSE; // Отсекать фрагменты за пределами дальней и ближней плоскости.
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE; // Выполнять растерезацию и передавать во фреймбуфер.
    rasterizer_info.polygonMode = is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL; // Ребра полигона отрезки или полигон полностью заполняет фрагмент.
    rasterizer_info.lineWidth = 1.0f;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT; // Тип отсечения.
    rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Порядок обхода вершин против часовой стрелки.
    rasterizer_info.depthBiasEnable = VK_FALSE; // Для изменения значения глубины.
    rasterizer_info.depthBiasConstantFactor = 0.0f;
    rasterizer_info.depthBiasClamp = 0.0f;
    rasterizer_info.depthBiasSlopeFactor = 0.0f;

    // Множественное сглаживание.
    VkPipelineMultisampleStateCreateInfo multisampling_info = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_info.minSampleShading = 1.0f;
    multisampling_info.pSampleMask = null;
    multisampling_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_info.alphaToOneEnable = VK_FALSE;

    // Тестирование глубины и трафарета.
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    if(depth_test_enabled)
    {
        depth_stencil_info.depthTestEnable = VK_TRUE;
        depth_stencil_info.depthWriteEnable = VK_TRUE;
        depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_info.stencilTestEnable = VK_FALSE;
    }

    // Смешивание цветов: цвета из фрагментного шейдера смешиваются с цветом из буфера.
    VkPipelineColorBlendAttachmentState color_blend_attachment_state;
    kzero_tc(&color_blend_attachment_state, VkPipelineColorBlendAttachmentState, 1);
    color_blend_attachment_state.blendEnable = VK_TRUE; // Цвет из фрагментного шейдера передается с изменениями.
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                                | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment_state;

    // Динамическое состояние: позволяет менять состояние графического конвейера не создавая занаво.
    // В результате эти настройки нужно указывать прямо во время отрисовки.
    #define DYNAMIC_STATE_COUNT 3
    VkDynamicState dynamic_states[DYNAMIC_STATE_COUNT] = {
        VK_DYNAMIC_STATE_VIEWPORT,   // Изменение вьюпорта.
        VK_DYNAMIC_STATE_SCISSOR,    // Изменение отсечение вьюпорта.
        VK_DYNAMIC_STATE_LINE_WIDTH, // Изменение ширины отрезков (см. растеризатор).
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic_state_info.dynamicStateCount = DYNAMIC_STATE_COUNT;
    dynamic_state_info.pDynamicStates = dynamic_states;

    // Первая стадия конвейера (вход вертексов).
    // В шейдере это строка layout(location = 0) in vec3 in_position; - атрибут!
    VkVertexInputBindingDescription binding_description = {0};
    binding_description.binding = 0;      // Индекс привязки к буферу данных.
    binding_description.stride = stride;  // Описывает расстояние между элементами данных буфера.
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Переход к следующей записи данных для каждой вершины.

    // Выршинный шейдер: передаваемые атрибуты и привязки.
    VkPipelineVertexInputStateCreateInfo vertex_input_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = attribute_count;
    vertex_input_info.pVertexAttributeDescriptions = attributes; // Данные передаваемые в вершинный шейдер.

    // Сборочный шейдер.
    // NOTE: Если в поле primitiveRestartEnable задать значение VK_TRUE, можно прервать отрезки и треугольники с
    // топологией VK_PRIMITIVE_TOPOLOGY_LINE_STRIP и VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP и начать рисовать новые
    // примитивы, используя специальный индекс 0xFFFF или 0xFFFFFFFF.
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Как отрисовывать вершины! Т.е. в данном случае как треугольники!
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    // Cоздание схемы конвейера (Layout конвейера): для использования uniform в шейдерах.
    VkPipelineLayoutCreateInfo pipeline_layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipeline_layout_info.setLayoutCount = descriptor_set_layout_count;
    pipeline_layout_info.pSetLayouts = descriptor_set_layouts;

    // Передача констант.
    if(push_constant_range_count > 0)
    {
        if(push_constant_range_count > 32)
        {
            kerror("Function '%s' cannot have more that", __FUNCTION__);
            return false;
        }

        VkPushConstantRange ranges[32];
        kzero_tc(ranges, VkPushConstantRange, 32);

        for(u32 i = 0; i < push_constant_range_count; ++i)
        {
            ranges[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            ranges[i].offset = push_constant_ranges[i].offset;
            ranges[i].size = push_constant_ranges[i].size;
        }

        pipeline_layout_info.pushConstantRangeCount = push_constant_range_count;
        pipeline_layout_info.pPushConstantRanges = ranges;
    }
    else
    {
        pipeline_layout_info.pushConstantRangeCount = 0;
        pipeline_layout_info.pPushConstantRanges = null;
    }

    VkResult result = vkCreatePipelineLayout(context->device.logical, &pipeline_layout_info, context->allocator, &out_pipeline->layout);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to create pipeline layour with result: %s", vulkan_result_get_string(result, true));
        return false;
    }

    // Создание графического конвейера.
    VkGraphicsPipelineCreateInfo pipeline_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipeline_info.stageCount = stage_count,
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_stage;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.pDepthStencilState = depth_test_enabled ? &depth_stencil_info : null;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.pTessellationState = null;
    pipeline_info.layout = out_pipeline->layout;
    pipeline_info.renderPass = renderpass->handle;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = INVALID_ID;

    result = vkCreateGraphicsPipelines(context->device.logical, VK_NULL_HANDLE, 1, &pipeline_info, context->allocator, &out_pipeline->handle);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to create graphics pipeline with result: %s", vulkan_result_get_string(result, true));
        return false;
    }

    return true;
}

void vulkan_pipeline_destroy(vulkan_context* context, vulkan_pipeline* pipeline)
{
    if(!context || !pipeline)
    {
        kerror("Function '%' requires a vulkan context and vulkan pipeline.", __FUNCTION__);
        return;
    }

    // Уничтожение конвейера.
    if(pipeline->handle)
    {
        vkDestroyPipeline(context->device.logical, pipeline->handle, context->allocator);
        pipeline->handle = null;
    }

    // Уничтожение схемы конвейера.
    if(pipeline->layout)
    {
        vkDestroyPipelineLayout(context->device.logical, pipeline->layout, context->allocator);
        pipeline->layout = null;
    }
}

void vulkan_pipeline_bind(
    vulkan_command_buffer* command_buffer, VkPipelineBindPoint bind_point, vulkan_pipeline* pipeline
)
{
    vkCmdBindPipeline(command_buffer->handle, bind_point, pipeline->handle);
}
