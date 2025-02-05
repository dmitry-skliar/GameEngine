// Собственные подключения.
#include "renderer/vulkan/shaders/vulkan_material_shader.h"
#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_pipeline.h"

// Внутренние подключения.
#include "logger.h"
#include "math/math_types.h"
#include "memory/memory.h"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.MaterialShader"

bool vulkan_material_shader_create(vulkan_context* context, vulkan_material_shader* out_shader)
{
    // Инициализация шейдерного модуля на каждом этапе.
    char stage_type_strings[MATERIAL_SHADER_STAGE_COUNT][5] = {"vert", "frag"};

    VkShaderStageFlagBits stage_types[MATERIAL_SHADER_STAGE_COUNT] = { 
        VK_SHADER_STAGE_VERTEX_BIT, 
        VK_SHADER_STAGE_FRAGMENT_BIT
    };

    for(u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i)
    {
        bool result = vulkan_shader_module_create(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_strings[i], stage_types[i], i, out_shader->stages);
        if(!result)
        {
            kerror("Unable to create %s shader model for '%s'.", stage_type_strings[i], BUILTIN_SHADER_NAME_OBJECT);
            return false;
        }
    }

    // Global descriptors.
    // VkDescriptorSetLayoutBinding global_ubo_layout_binding;
    // global_ubo_layout_binding.binding = 0;
    // global_ubo_layout_binding.descriptorCount = 1;
    // global_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // global_ubo_layout_binding.pImmutableSamplers = NULL;
    // global_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // VkDescriptorSetLayoutCreateInfo global_layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    // global_layout_info.bindingCount = 1;
    // global_layout_info.pBindings = &global_ubo_layout_binding;

    // VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical, &global_layout_info, context->allocator, 
    //                                      &out_shader->global_descriptor_set_layout));

    // VkDescriptorPoolSize global_pool_size;
    // global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // global_pool_size.descriptorCount = context->swapchain.image_count;

    // VkDescriptorPoolCreateInfo global_pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    // global_pool_info.poolSizeCount = 1;
    // global_pool_info.pPoolSizes = &global_pool_size;
    // global_pool_info.maxSets = context->swapchain.image_count;

    // // Global descriptor pool: Used for global items such as view/projection matrix.
    // VK_CHECK(vkCreateDescriptorPool(context->device.logical, &global_pool_info, context->allocator, 
    //                                 &out_shader->global_descriptor_pool));

    // // Local/Object descriptors.
    // const u32 local_sampler_count = 1;
    // VkDescriptorType desciptor_types[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT] = {
    //     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,            // Binding 0 - Uniform buffer.
    //     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,    // Binding 1 - Diffuse sampler layout.
    // };

    // VkDescriptorSetLayoutBinding bindings[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
    // kmzero_tc(&bindings, VkDescriptorSetLayoutBinding, VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);

    // for(u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i)
    // {
    //     bindings[i].binding = i;
    //     bindings[i].descriptorCount = 1;
    //     bindings[i].descriptorType = desciptor_types[i];
    //     bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // }

    // VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    // layout_info.bindingCount = VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT;
    // layout_info.pBindings = bindings;

    // VK_CHECK(vkCreateDescriptorSetLayout(
    //     context->device.logical, &layout_info, context->allocator, &out_shader->object_descriptor_set_layout
    // ));

    // // Local/Object descriptor pool: Used for object-specific items like diffuse color.
    // VkDescriptorPoolSize object_pool_sizes[2];
    // // The first section will be used for uniform buffers.
    // object_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // object_pool_sizes[0].descriptorCount = VULKAN_OBJECT_MAX_OBJECT_COUNT;
    // // The second section will be used for image samplers.
    // object_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // object_pool_sizes[1].descriptorCount = local_sampler_count * VULKAN_OBJECT_MAX_OBJECT_COUNT;


    // VkDescriptorPoolCreateInfo object_pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    // object_pool_info.poolSizeCount = 2;
    // object_pool_info.pPoolSizes = object_pool_sizes;
    // object_pool_info.maxSets = VULKAN_OBJECT_MAX_OBJECT_COUNT;

    // VK_CHECK(vkCreateDescriptorPool(
    //     context->device.logical, &object_pool_info, context->allocator, &out_shader->object_descriptor_pool
    // ));

    // Область экрана.
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context->framebuffer_height;
    viewport.width = (f32)context->framebuffer_width;
    viewport.height = -(f32)context->framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor.
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context->framebuffer_width;
    scissor.extent.height = context->framebuffer_height;

    // Атрибуты.
    #define ATTRIBUTE_COUNT 1
    VkVertexInputAttributeDescription attribute_descriptions[ATTRIBUTE_COUNT];
    
    // Позиция, текстурные координаты.
    VkFormat formats[ATTRIBUTE_COUNT] = {
        VK_FORMAT_R32G32B32_SFLOAT,
        // VK_FORMAT_R32G32_SFLOAT,
    };

    u64 sizes[ATTRIBUTE_COUNT] = {
        sizeof(vec3),
        // sizeof(vec2)
    };

    u32 offset = 0;
    // В шейдере это строка layout(location = 0) in vec3 in_position; - атрибут!
    for(u32 i = 0; i < ATTRIBUTE_COUNT; ++i)
    {
        attribute_descriptions[i].binding = 0;    // индекс привязки - должен соответствовать описанию привязки.
        attribute_descriptions[i].location = i;   // расположение атрибута - location.
        attribute_descriptions[i].format = formats[i];
        attribute_descriptions[i].offset = offset;
        offset += sizes[i];
    }

    // // Descriptor set layout.
    // const i32 descriptor_sey_layout_count = 2;
    // VkDescriptorSetLayout layouts[2] = {
    //     out_shader->global_descriptor_set_layout,
    //     out_shader->object_descriptor_set_layout
    // };

    // NOTE: Должно соответствовать количеству стадий шейдеров (shader->stages).
    VkPipelineShaderStageCreateInfo stage_infos[MATERIAL_SHADER_STAGE_COUNT];
    kzero_tc(stage_infos, stage_infos, 1);

    for(u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i)
    {
        stage_infos[i] = out_shader->stages[i].pipelineinfo;
    }

    // Создание графического конвейера.
    if(!vulkan_graphics_pipeline_create(
            context, &context->main_renderpass, ATTRIBUTE_COUNT, attribute_descriptions, 0, null,
            MATERIAL_SHADER_STAGE_COUNT, stage_infos, viewport, scissor, false, &out_shader->pipeline
    ))
    {
        kerror("Function '%s': Failed to load graphics pipeline for material shader.", __FUNCTION__);
        return false;
    }

    // // Create global uniform buffer.
    // u32 device_local_bit = context->device.support_device_local_host_visible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
    // if(!vulkan_buffer_create(
    //     context, sizeof(global_uniform_object), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
    //     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | device_local_bit, 
    //     true, &out_shader->global_uniform_buffer
    // ))
    // {
    //     KERROR("Vulkan buffer creation failed for object shader.");
    //     return false;
    // }

    // // Allocate global descriptor sets.
    // VkDescriptorSetLayout global_layouts[5] = { // FIX: Because minImageCount is 4 + 1 = 5!
    //     out_shader->global_descriptor_set_layout,
    //     out_shader->global_descriptor_set_layout,
    //     out_shader->global_descriptor_set_layout,
    //     out_shader->global_descriptor_set_layout,
    //     out_shader->global_descriptor_set_layout
    // };

    // VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    // allocate_info.descriptorPool = out_shader->global_descriptor_pool;
    // allocate_info.descriptorSetCount = 5; // FIX: Because minImageCount is 4 + 1 = 5!
    // allocate_info.pSetLayouts = global_layouts;

    // VK_CHECK(vkAllocateDescriptorSets(context->device.logical, &allocate_info, out_shader->global_descriptor_sets));

    // // Create the object uniform buffer.
    // if(!vulkan_buffer_create( //* MAX_MATERIAL_INSTANCE_COUNT,
    //     context, sizeof(object_uniform_object), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    //     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //     true, &out_shader->object_uniform_buffer
    // ))
    // {
    //     KERROR("Material instance buffer creation failed for shader.");
    //     return false;
    // }

    return true;
}

void vulkan_material_shader_destroy(vulkan_context* context, vulkan_material_shader* shader)
{

    // // Destroy global and object uniform buffer.
    // vulkan_buffer_destroy(context, &shader->object_uniform_buffer);
    // vulkan_buffer_destroy(context, &shader->global_uniform_buffer);

    // Уничтоженрие графического конвейера.
    vulkan_graphics_pipeline_destroy(context, &shader->pipeline);

    // // Destroy object descriptor pool and descriptor set layout.
    // vkDestroyDescriptorPool(context->device.logical, shader->object_descriptor_pool, context->allocator);
    // vkDestroyDescriptorSetLayout(context->device.logical, shader->object_descriptor_set_layout, context->allocator);

    // // Destroy global descriptor pool and descriptor set layout.
    // vkDestroyDescriptorPool(context->device.logical, shader->global_descriptor_pool, context->allocator);
    // vkDestroyDescriptorSetLayout(context->device.logical, shader->global_descriptor_set_layout, context->allocator);
    
    // Уничтожение модуля шейдера.
    for(u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i)
    {
        vkDestroyShaderModule(context->device.logical, shader->stages[i].handle, context->allocator);
        shader->stages[i].handle = NULL;
    }
}

void vulkan_material_shader_use(vulkan_context* context, vulkan_material_shader* shader)
{
    u32 image_index = context->image_index;
    vulkan_pipeline_bind(&context->graphics_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}

void vulkan_material_shader_update_global_state(vulkan_context* context, vulkan_material_shader* shader, f32 delta_time)
{
    // u32 image_index = context->image_index;
    // VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;
    // VkDescriptorSet global_descriptor = shader->global_descriptor_sets[image_index];

    // // Bind the global descriptor set to be updated.
    // vkCmdBindDescriptorSets(
    //     command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.layout, 0, 1, &global_descriptor, 0, NULL
    // );

    // // Configure the descriptors for the given index.
    // u32 range = sizeof(global_uniform_object);
    // u64 offset = 0;

    // // Copy data to buffer.
    // vulkan_buffer_load_data(context, &shader->global_uniform_buffer, offset, range, 0, &shader->global_ubo);

    // VkDescriptorBufferInfo buffer_info;
    // buffer_info.buffer = shader->global_uniform_buffer.handle;
    // buffer_info.offset = offset;
    // buffer_info.range  = range;

    // // Update descriptor sets.
    // VkWriteDescriptorSet descriptor_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    // descriptor_write.dstSet = global_descriptor;
    // descriptor_write.dstBinding = 0;
    // descriptor_write.dstArrayElement = 0;
    // descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // descriptor_write.descriptorCount = 1;
    // descriptor_write.pBufferInfo = &buffer_info;

    // vkUpdateDescriptorSets(context->device.logical, 1, &descriptor_write, 0, NULL);
}
