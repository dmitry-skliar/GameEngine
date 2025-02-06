// Собственные подключения.
#include "renderer/vulkan/shaders/vulkan_material_shader.h"
#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_buffer.h"

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

    // Глобальные дескрипторы.
    VkDescriptorSetLayoutBinding global_ubo_layout_binding;
    global_ubo_layout_binding.binding = 0;
    global_ubo_layout_binding.descriptorCount = 1;
    global_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_ubo_layout_binding.pImmutableSamplers = NULL;
    global_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo global_layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    global_layout_info.bindingCount = 1;
    global_layout_info.pBindings = &global_ubo_layout_binding;

    VkResult result = vkCreateDescriptorSetLayout(
        context->device.logical, &global_layout_info, context->allocator, &out_shader->global_descriptor_set_layout
    );
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to create descriptor set layout with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

    // Глобальный пул дескрипторов: используется для глобальных элементов, таких как матрица вида/проекции.
    VkDescriptorPoolSize global_pool_size;
    global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_pool_size.descriptorCount = context->swapchain.image_count;

    VkDescriptorPoolCreateInfo global_pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    global_pool_info.poolSizeCount = 1;
    global_pool_info.pPoolSizes = &global_pool_size;
    global_pool_info.maxSets = context->swapchain.image_count;

    result = vkCreateDescriptorPool(
        context->device.logical, &global_pool_info, context->allocator, &out_shader->global_descriptor_pool
    );
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to create descriptor pool with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

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

    // Область просмотра.
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context->framebuffer_height;
    viewport.width = (f32)context->framebuffer_width;
    viewport.height = -(f32)context->framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Область отсечения.
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

    // Набор дескрипторов.
    #define DESCRIPTOR_SET_LAYOUT_COUNT 1
    VkDescriptorSetLayout layouts[DESCRIPTOR_SET_LAYOUT_COUNT] = {
        out_shader->global_descriptor_set_layout,
    };

    // NOTE: Должно соответствовать количеству стадий шейдеров (shader->stages).
    VkPipelineShaderStageCreateInfo stage_infos[MATERIAL_SHADER_STAGE_COUNT];
    kzero_tc(stage_infos, stage_infos, 1);

    for(u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i)
    {
        stage_infos[i] = out_shader->stages[i].pipelineinfo;
    }

    // Создание графического конвейера.
    if(!vulkan_graphics_pipeline_create(
        context, &context->main_renderpass, ATTRIBUTE_COUNT, attribute_descriptions, DESCRIPTOR_SET_LAYOUT_COUNT,
        layouts, MATERIAL_SHADER_STAGE_COUNT, stage_infos, viewport, scissor, false, &out_shader->pipeline
    ))
    {
        kerror("Function '%s': Failed to load graphics pipeline for material shader.", __FUNCTION__);
        return false;
    }

    // Создание глобального uniform buffer.
    u32 device_local_bit = context->device.memory_local_host_visible_support ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
    if(!vulkan_buffer_create( // TODO: image_index = 5, позже сделать универсальный метод!
        context, sizeof(global_uniform_object)/* * 5 */, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | device_local_bit, 
        true, &out_shader->global_uniform_buffer
    ))
    {
        kerror("Function '%s': Failed to create vulkan buffer.", __FUNCTION__);
        return false;
    }

    // Выделяем глобальные наборы дескрипторов.
    VkDescriptorSetLayout global_layouts[5] = { // TODO: image_count = 5!
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout
    };

    VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = out_shader->global_descriptor_pool;
    allocate_info.descriptorSetCount = 5; // TODO: image_count = 5!
    allocate_info.pSetLayouts = global_layouts;

    result = vkAllocateDescriptorSets(context->device.logical, &allocate_info, out_shader->global_descriptor_sets);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed to allocate descriptor sets with result: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

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

    // Уничтожение глобального uniform buffer.
    vulkan_buffer_destroy(context, &shader->global_uniform_buffer);
    // vulkan_buffer_destroy(context, &shader->object_uniform_buffer);

    // Уничтоженрие графического конвейера.
    vulkan_graphics_pipeline_destroy(context, &shader->pipeline);

    // // Destroy object descriptor pool and descriptor set layout.
    // vkDestroyDescriptorPool(context->device.logical, shader->object_descriptor_pool, context->allocator);
    // vkDestroyDescriptorSetLayout(context->device.logical, shader->object_descriptor_set_layout, context->allocator);

    // Уничтожение глобального пула дескрипторов и набора дескрипторов.
    vkDestroyDescriptorPool(context->device.logical, shader->global_descriptor_pool, context->allocator);
    vkDestroyDescriptorSetLayout(context->device.logical, shader->global_descriptor_set_layout, context->allocator);
    
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
    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;
    VkDescriptorSet global_descriptor = shader->global_descriptor_sets[image_index];

    // Настройка дескрипторов для указанного индекса.
    u32 range = sizeof(global_uniform_object);
    u64 offset = 0; // sizeof(global_uniform_object) * image_index;

    // Копирование данных в буфер.
    vulkan_buffer_load_data(context, &shader->global_uniform_buffer, offset, range, 0, &shader->global_ubo);

    VkDescriptorBufferInfo buffer_info;
    buffer_info.buffer = shader->global_uniform_buffer.handle;
    buffer_info.offset = offset;
    buffer_info.range  = range;

    // Обновление набора дескрипторов.
    VkWriteDescriptorSet descriptor_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descriptor_write.dstSet = global_descriptor;
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(context->device.logical, 1, &descriptor_write, 0, null);

    // Связывание глобального набор дескрипторов для обновления.
    vkCmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.layout, 0, 1, &global_descriptor, 0, null
    );
}

void vulkan_material_shader_update_object(vulkan_context* context, vulkan_material_shader* shader, geometry_render_data data)
{
    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;

    vkCmdPushConstants(command_buffer, shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &data.model);

    // // Obtain material data.
    // vulkan_object_shader_object_state* object_state = &shader->object_states[data.object_id];
    // VkDescriptorSet object_descriptor_set = object_state->descriptor_sets[image_index];

    // // TODO: if needs update.
    // VkWriteDescriptorSet descriptor_writes[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
    // kmzero_tc(descriptor_writes, VkWriteDescriptorSet, VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);
    // u32 descriptor_count = 0;
    // u32 descriptor_index = 0;

    // // Descriptor 0 - Uniform buffer.
    // u32 range  = sizeof(object_uniform_object);
    // u64 offset = sizeof(object_uniform_object) * data.object_id; // Also the index into the array.
    // object_uniform_object obo;

    // // TODO: Get diffuse color from a material.
    // static f32 accumulator = 0.0f;
    // accumulator += context->frame_delta_time;
    // f32 s = (ksin(accumulator) + 1.0f) / 2.0f;  // Scale from -1, 1 to 0, 1.
    // obo.diffuse_color = vec4_create(s, s, s, 1.0f);

    // // Load the data into the buffer.
    // vulkan_buffer_load_data(context, &shader->object_uniform_buffer, offset, range, 0, &obo);

    // // Only do this if the descriptor has not yet been updated.
    // if(object_state->descriptor_states[descriptor_index].generations[image_index] == INVALID_ID)
    // {
    //     VkDescriptorBufferInfo buffer_info;
    //     buffer_info.buffer = shader->object_uniform_buffer.handle;
    //     buffer_info.offset = offset;
    //     buffer_info.range  = range;

    //     VkWriteDescriptorSet descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    //     descriptor.dstSet = object_descriptor_set;
    //     descriptor.dstBinding = descriptor_index;
    //     descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //     descriptor.descriptorCount = 1;
    //     descriptor.pBufferInfo = &buffer_info;

    //     descriptor_writes[descriptor_count] = descriptor;
    //     descriptor_count++;

    //     // Update the frame generation. In this case it is only needed once since this is a buffer.
    //     object_state->descriptor_states[descriptor_index].generations[image_index] = 1;
    // }
    // descriptor_index++;

    // // TODO: Samplers.
    // const u32 sampler_count = 1;
    // VkDescriptorImageInfo image_infos[1];
    // for(u32 sampler_index = 0; sampler_index < sampler_count; ++sampler_index)
    // {
    //     texture* t = data.textures[sampler_index];
    //     u32* descriptor_generation = &object_state->descriptor_states[descriptor_index].generations[image_index];

    //     // If the texture hasn't been loaded yet, use the default.
    //     // TODO: Determine which use the texture has and pull appropriate default based on that.
    //     if(t->generation == INVALID_ID)
    //     {
    //         t = shader->default_diffuse;

    //         // Reset the descriptor generation if using the dafault texture.
    //         *descriptor_generation = INVALID_ID;
    //     }

    //     // Check if the descriptor needs update first.
    //     if(t && (*descriptor_generation != t->generation || *descriptor_generation == INVALID_ID))
    //     {
    //         vulkan_texture_data* tdata = (vulkan_texture_data*)t->data;

    //         // Assign view and sampler.
    //         image_infos[sampler_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //         image_infos[sampler_index].imageView = tdata->image.view;
    //         image_infos[sampler_index].sampler = tdata->sampler;

    //         VkWriteDescriptorSet descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    //         descriptor.dstSet = object_descriptor_set;
    //         descriptor.dstBinding = descriptor_index;
    //         descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //         descriptor.descriptorCount = 1;
    //         descriptor.pImageInfo = &image_infos[sampler_index];

    //         descriptor_writes[descriptor_count] = descriptor;
    //         descriptor_count++;

    //         // Sync frame generation if not using a default texture.
    //         if(t->generation != INVALID_ID)
    //         {
    //             *descriptor_generation = t->generation;
    //         }
    //         descriptor_index++;
    //     }
    // }

    // if(descriptor_count > 0)
    // {
    //     vkUpdateDescriptorSets(context->device.logical, descriptor_count, descriptor_writes, 0, NULL);
    // }

    // // Bind the descriptor set to be updated, or in case the shader changed.
    // vkCmdBindDescriptorSets(
    //     command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.layout, 1, 1, &object_descriptor_set, 0, NULL
    // );
}

bool vulkan_material_shader_acquire_resources(vulkan_context* context, vulkan_material_shader* shader, u32* out_object_id)
{
    // // TODO: Free list.
    // *out_object_id = shader->object_uniform_buffer_index;
    // shader->object_uniform_buffer_index++;

    // u32 object_id = *out_object_id;
    // vulkan_object_shader_object_state* object_state = &shader->object_states[object_id];
    // for(u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i)
    // {
    //     for(u32 j = 0; j < 5; ++j)
    //     {
    //         object_state->descriptor_states[i].generations[j] = INVALID_ID;
    //     }
    // }

    // // Allocate descriptor sets.
    // VkDescriptorSetLayout layouts[5] = {
    //     shader->object_descriptor_set_layout,
    //     shader->object_descriptor_set_layout,
    //     shader->object_descriptor_set_layout,
    //     shader->object_descriptor_set_layout,
    //     shader->object_descriptor_set_layout
    // };

    // VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    // allocate_info.descriptorPool = shader->object_descriptor_pool;
    // allocate_info.descriptorSetCount = 5; // One per frame.
    // allocate_info.pSetLayouts = layouts;

    // VkResult result = vkAllocateDescriptorSets(context->device.logical, &allocate_info, object_state->descriptor_sets);
    // if(result != VK_SUCCESS)
    // {
    //     KERROR("Error allocating descriptor sets in shader!");
    //     return false;
    // }

    return true;
}

void vulkan_material_shader_release_resources(vulkan_context* context, vulkan_material_shader* shader, u32 object_id)
{
    // vulkan_object_shader_object_state* object_state = &shader->object_states[object_id];

    // const u32 descriptor_set_count = 5;

    // // Release object descriptor sets.
    // VkResult result = vkFreeDescriptorSets(
    //     context->device.logical, shader->object_descriptor_pool, descriptor_set_count, object_state->descriptor_sets
    // );

    // if(result != VK_SUCCESS)
    // {
    //     KERROR("Error freeing object shader descriptor sets!");
    // }

    // for(u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i)
    // {
    //     for(u32 j = 0; j < 5; ++j)
    //     {
    //         object_state->descriptor_states[i].generations[j] = INVALID_ID;
    //     }
    // }

    // // TODO: add the object_id to the free list.
}
