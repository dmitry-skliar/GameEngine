// Собственные подключения.
#include "renderer/vulkan/vulkan_shader.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_buffer.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "containers/hashtable.h"
#include "systems/resource_system.h"
#include "systems/texture_system.h"

const u32 DESC_SET_INDEX_GLOBAL   = 0;
const u32 DESC_SET_INDEX_INSTANCE = 1;

const u32 BINDING_INDEX_UBO       = 0;
const u32 BINDING_INDEX_SAMPLER   = 1;

// Освободить шейдер и вернуть false.
#define FAIL_DESTROY(shader)        \
    vulkan_shader_destroy(shader);  \
    return false;

bool shader_module_create(vulkan_shader* shader, vulkan_shader_stage_config config, vulkan_shader_stage* shader_stage);
bool shader_uniform_name_valid(vulkan_shader* shader, const char* uniform_name);
bool shader_uniform_add_state_valid(vulkan_shader* shader);
bool shader_uniform_add(vulkan_shader* shader, const char* uniform_name, u32 size, shader_scope scope, u32* out_location, bool is_sampler);
bool shader_uniform_check_size(vulkan_shader* shader, u32 location, u32 expected_size);
bool shader_uniform_set(vulkan_shader* shader, u32 location, void* value, u64 size);

bool vulkan_shader_create(
    vulkan_context* context, const char* name, vulkan_renderpass* renderpass, VkShaderStageFlags stages,
    u16 max_descriptor_set_count, bool use_instances, bool use_local, vulkan_shader* out_shader
)
{
    if(!context || !name || !out_shader)
    {
        kerror("Function '%s' requires a valid pointer to context, name and out_shader. Creation failed.", __FUNCTION__);
        return false;
    }

    if(stages == 0)
    {
        kerror("Function '%s' requires stages to be non-zero.", __FUNCTION__);
        return false;
    }

    // TODO: Проверку max_descriptor_set_count, сейчас она равна VULKAN_SHADER_MAX_DESCRIPTOR_SETS.

    kzero_tc(out_shader, vulkan_shader, 1);
    string_ncopy(out_shader->name, name, VULKAN_SHADER_MAX_NAME_LENGTH);
    out_shader->state = VULKAN_SHADER_STATE_NOT_CREATED;
    out_shader->context = context;
    out_shader->use_instances = use_instances;
    out_shader->use_push_constants = use_local;
    out_shader->renderpass = renderpass;
    out_shader->bound_instance_id = INVALID_ID;
    // NOTE: Для nVidia GPU требуется выравнивание в 256-байт для uniform buffers.
    out_shader->required_ubo_aligment = 256;
    out_shader->push_constant_stride = 128;
    out_shader->config.max_descriptor_set_count = max_descriptor_set_count;

    // Полуение указателья на стадию шейдера для заполнения конфигурации.
    vulkan_shader_stage_config* stage_config = &out_shader->config.stages[out_shader->config.stage_count];

    // Формирование конфигурации стадий шейдера. 
    for(u32 i = VK_SHADER_STAGE_VERTEX_BIT; i < VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM; i <<= 1)
    {
        if((stages & i) == i)
        {
            // Запись флага стадии.
            stage_config->stage = i;

            // Запись строкового наименования стадии.
            switch(i)
            {
                case VK_SHADER_STAGE_VERTEX_BIT:
                    string_ncopy(stage_config->stage_str, "vert", 7);
                    break;
                case VK_SHADER_STAGE_FRAGMENT_BIT:
                    string_ncopy(stage_config->stage_str, "frag", 7);
                    break;
                default:
                    kerror("Function '%s': Unsupported shader stage flagged: %d. Stage ignored.", __FUNCTION__, i);
                    continue; // Поиск следующего типа.
            }

            // Проверка выхода за пределы стадий шейдера.
            if(out_shader->config.stage_count + 1 > VULKAN_SHADER_MAX_STAGES)
            {
                kerror("Function '%s': Shader may have a maximum of %d stages.", __FUNCTION__, VULKAN_SHADER_MAX_STAGES);
                return false;
            }

            // Полуение указателья на следующую стадию шейдера для заполнения конфигурации.
            out_shader->config.stage_count++;
            stage_config = &out_shader->config.stages[out_shader->config.stage_count];
        }

        // Окончить перебор, если максимальное количество стадий записано.
        if(out_shader->config.stage_count >= VULKAN_SHADER_MAX_STAGES)
        {
            break;
        }
    }

    // Создание хэш таблицы для хранения uniform array indexes.
    hashtable_config hcfg = { sizeof(u32), 1024 };
    hashtable_create(&out_shader->uniform_lookup_memory_requirement, null, &hcfg, null);
    out_shader->uniform_lookup_memory = kallocate(out_shader->uniform_lookup_memory_requirement, MEMORY_TAG_HASHTABLE);
    hashtable_create(
        &out_shader->uniform_lookup_memory_requirement, out_shader->uniform_lookup_memory, &hcfg,
        &out_shader->uniform_lookup
    );

    // HACK: Максимальное число ubo дескрипторных наборов.
    out_shader->config.pool_sizes[0] = (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024};
    // HACK: Максимальное число image sampler дескрипторных наборов.
    out_shader->config.pool_sizes[1] = (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096};

    vulkan_descriptor_set_config* global_descriptor_set_config = &out_shader->config.descriptor_sets[DESC_SET_INDEX_GLOBAL];
    global_descriptor_set_config->bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
    global_descriptor_set_config->bindings[BINDING_INDEX_UBO].descriptorCount = 1;
    global_descriptor_set_config->bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_descriptor_set_config->bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    global_descriptor_set_config->binding_count++;
    out_shader->config.descriptor_set_count++;

    if(out_shader->use_instances)
    {
        vulkan_descriptor_set_config* instance_descriptor_set_config = &out_shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE];
        instance_descriptor_set_config->bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
        instance_descriptor_set_config->bindings[BINDING_INDEX_UBO].descriptorCount = 1;
        instance_descriptor_set_config->bindings[BINDING_INDEX_UBO].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        instance_descriptor_set_config->bindings[BINDING_INDEX_UBO].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        instance_descriptor_set_config->binding_count++;
        out_shader->config.descriptor_set_count++;
    }

    for(u32 i = 0; i < VULKAN_SHADER_MAX_MATERIAL_COUNT; ++i)
    {
        out_shader->instance_states[i].id = INVALID_ID;
    }

    out_shader->state = VULKAN_SHADER_STATE_UNINITIALIZED;
    return true;
}

bool vulkan_shader_destroy(vulkan_shader* shader)
{
    if(!shader)
    {
        kerror("Function '%s' requires a valid pointer to shader.", __FUNCTION__);
        return false;
    }

    VkDevice logical = shader->context->device.logical;
    VkAllocationCallbacks* vk_allocator = shader->context->allocator;

    shader->state = VULKAN_SHADER_STATE_NOT_CREATED;

    hashtable_destroy(shader->uniform_lookup);
    kfree(shader->uniform_lookup_memory, shader->uniform_lookup_memory_requirement, MEMORY_TAG_HASHTABLE);

    for(u32 i = 0; i < shader->config.descriptor_set_count; ++i)
    {
        if(shader->descriptor_set_layouts[i])
        {
            vkDestroyDescriptorSetLayout(logical, shader->descriptor_set_layouts[i], vk_allocator);
            shader->descriptor_set_layouts[i] = null;
        }
    }

    if(shader->descriptor_pool)
    {
        vkDestroyDescriptorPool(logical, shader->descriptor_pool, vk_allocator);
    }

    vulkan_buffer_unlock_memory(shader->context, &shader->uniform_buffer);
    shader->uniform_buffer_mapped_block = null;

    vulkan_buffer_destroy(shader->context, &shader->uniform_buffer);

    vulkan_pipeline_destroy(shader->context, &shader->pipeline);

    for(u32 i = 0; i < shader->config.stage_count; ++i)
    {
        // TODO: Их можно уничтожить после создания pipeline!
        vkDestroyShaderModule(logical, shader->stages[i].handle, vk_allocator);
    }

    kzero_tc(&shader->config, vulkan_shader_config, 1);

    return true;
}

typedef struct vulkan_format_size {
    VkFormat format;
    u32 size;
} vulkan_format_size;

bool vulkan_shader_add_attribute(vulkan_shader* shader, const char* name, shader_attribute_type type)
{
    if(!shader || !name)
    {
        kerror("Function '%s' requires a valid pointer to shader and name.", __FUNCTION__);
        return false;
    }

    static const vulkan_format_size types[29] = {
        [SHADER_ATTRIB_TYPE_FLOAT32]   = (vulkan_format_size){VK_FORMAT_R32_SFLOAT, 4},
        [SHADER_ATTRIB_TYPE_FLOAT32_2] = (vulkan_format_size){VK_FORMAT_R32G32_SFLOAT, 8},
        [SHADER_ATTRIB_TYPE_FLOAT32_3] = (vulkan_format_size){VK_FORMAT_R32G32B32_SFLOAT, 12},
        [SHADER_ATTRIB_TYPE_FLOAT32_4] = (vulkan_format_size){VK_FORMAT_R32G32B32A32_SFLOAT, 16},
        [SHADER_ATTRIB_TYPE_INT8]      = (vulkan_format_size){VK_FORMAT_R8_SINT, 1},
        [SHADER_ATTRIB_TYPE_INT8_2]    = (vulkan_format_size){VK_FORMAT_R8G8_SINT, 2},
        [SHADER_ATTRIB_TYPE_INT8_3]    = (vulkan_format_size){VK_FORMAT_R8G8B8_SINT, 3},
        [SHADER_ATTRIB_TYPE_INT8_4]    = (vulkan_format_size){VK_FORMAT_R8G8B8A8_SINT, 4},
        [SHADER_ATTRIB_TYPE_UINT8]     = (vulkan_format_size){VK_FORMAT_R8_UINT, 1},
        [SHADER_ATTRIB_TYPE_UINT8_2]   = (vulkan_format_size){VK_FORMAT_R8G8_UINT, 2},
        [SHADER_ATTRIB_TYPE_UINT8_3]   = (vulkan_format_size){VK_FORMAT_R8G8B8_UINT, 3},
        [SHADER_ATTRIB_TYPE_UINT8_4]   = (vulkan_format_size){VK_FORMAT_R8G8B8A8_UINT, 4},
        [SHADER_ATTRIB_TYPE_INT16]     = (vulkan_format_size){VK_FORMAT_R16_SINT, 2},
        [SHADER_ATTRIB_TYPE_INT16_2]   = (vulkan_format_size){VK_FORMAT_R16G16_SINT, 4},
        [SHADER_ATTRIB_TYPE_INT16_3]   = (vulkan_format_size){VK_FORMAT_R16G16B16_SINT, 6},
        [SHADER_ATTRIB_TYPE_INT16_4]   = (vulkan_format_size){VK_FORMAT_R16G16B16A16_SINT, 8},
        [SHADER_ATTRIB_TYPE_UINT16]    = (vulkan_format_size){VK_FORMAT_R16_UINT, 2},
        [SHADER_ATTRIB_TYPE_UINT16_2]  = (vulkan_format_size){VK_FORMAT_R16G16_UINT, 4},
        [SHADER_ATTRIB_TYPE_UINT16_3]  = (vulkan_format_size){VK_FORMAT_R16G16B16_UINT, 6},
        [SHADER_ATTRIB_TYPE_UINT16_4]  = (vulkan_format_size){VK_FORMAT_R16G16B16A16_UINT, 8},
        [SHADER_ATTRIB_TYPE_INT32]     = (vulkan_format_size){VK_FORMAT_R32_SINT, 4},
        [SHADER_ATTRIB_TYPE_INT32_2]   = (vulkan_format_size){VK_FORMAT_R32G32_SINT, 8},
        [SHADER_ATTRIB_TYPE_INT32_3]   = (vulkan_format_size){VK_FORMAT_R32G32B32_SINT, 12},
        [SHADER_ATTRIB_TYPE_INT32_4]   = (vulkan_format_size){VK_FORMAT_R32G32B32A32_SINT, 16},
        [SHADER_ATTRIB_TYPE_UINT32]    = (vulkan_format_size){VK_FORMAT_R32_UINT, 4},
        [SHADER_ATTRIB_TYPE_UINT32_2]  = (vulkan_format_size){VK_FORMAT_R32G32_UINT, 8},
        [SHADER_ATTRIB_TYPE_UINT32_3]  = (vulkan_format_size){VK_FORMAT_R32G32B32_UINT, 12},
        [SHADER_ATTRIB_TYPE_UINT32_4]  = (vulkan_format_size){VK_FORMAT_R32G32B32A32_UINT, 16}
    };

    // TODO: Проверка на превышения максимального значения атрибутов!
    VkVertexInputAttributeDescription* attribute = &shader->config.attributes[shader->config.attribute_count];
    attribute->binding = 0;                               // индекс привязки TODO: должен соответствовать описанию привязки.
    attribute->location = shader->config.attribute_count; // расположение атрибута - location.
    attribute->format = types[type].format;
    attribute->offset = shader->config.attribute_stride;

    shader->config.attribute_stride += types[type].size;
    shader->config.attribute_count++;

    return true;
}

bool vulkan_shader_add_sampler(vulkan_shader* shader, const char* sampler_name, shader_scope scope, u32* out_location)
{
    if(scope == SHADER_SCOPE_INSTANCE && !shader->use_instances)
    {
        kerror("Function '%s' cannot add an instance sampler for a shader that does not use instances.", __FUNCTION__);
        return false;
    }

    if(scope == SHADER_SCOPE_LOCAL)
    {
        kerror("Function '%s' cannot add a sampler at local scope.", __FUNCTION__);
        return false;
    }

    // Имя должно быть уникально.
    if(!shader_uniform_name_valid(shader, sampler_name) || !shader_uniform_add_state_valid(shader))
    {
        return false;
    }

    if(scope == SHADER_SCOPE_GLOBAL)
    {
        if(shader->global_texture_count + 1 > VULKAN_SHADER_MAX_GLOBAL_TEXTURES)
        {
            kerror(
                "Function '%s': Shader global texture count %u exceeds max of %u",
                __FUNCTION__, shader->global_texture_count, VULKAN_SHADER_MAX_GLOBAL_TEXTURES
            );
            return false;
        }
        
        shader->global_textures[shader->global_texture_count] = texture_system_get_default_texture();
        shader->global_texture_count++;
    }
    else
    {
        if(shader->instance_texture_count + 1 > VULKAN_SHADER_MAX_INSTANCE_TEXTURES)
        {
            kerror(
                "Function '%s': Shader instance texture count %u exceeds max of %u",
                __FUNCTION__, shader->instance_texture_count, VULKAN_SHADER_MAX_INSTANCE_TEXTURES
            );
            return false;
        }

        shader->instance_texture_count++;
    }

    const u32 set_index = (scope == SHADER_SCOPE_GLOBAL ? DESC_SET_INDEX_GLOBAL : DESC_SET_INDEX_INSTANCE);
    vulkan_descriptor_set_config* set_config = &shader->config.descriptor_sets[set_index];

    if(set_config->binding_count < 2)
    {
        set_config->bindings[BINDING_INDEX_SAMPLER].binding = BINDING_INDEX_SAMPLER;
        set_config->bindings[BINDING_INDEX_SAMPLER].descriptorCount = 1;
        set_config->bindings[BINDING_INDEX_SAMPLER].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        set_config->bindings[BINDING_INDEX_SAMPLER].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        set_config->binding_count++;

        *out_location = 0;
    }
    else
    {
        *out_location = set_config->bindings[BINDING_INDEX_SAMPLER].descriptorCount;
        set_config->bindings[BINDING_INDEX_SAMPLER].descriptorCount++;
    }

    if(!shader_uniform_add(shader, sampler_name, 0, scope, out_location, true))
    {
        kerror("Function '%s': Unable to add sampler uniform.", __FUNCTION__);
        return false;
    }

    return true;
}

#define VERIFY_UNIFORM(shader, uniform_name, out_location) \
    if(!out_location || !shader_uniform_add_state_valid(shader) || !shader_uniform_name_valid(shader, uniform_name)) return false;

bool vulkan_shader_add_uniform_i8(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(i8), scope, out_location, false);
}

bool vulkan_shader_add_uniform_i16(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(i16), scope, out_location, false);
}

bool vulkan_shader_add_uniform_i32(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(i32), scope, out_location, false);
}

bool vulkan_shader_add_uniform_u8(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(u8), scope, out_location, false);
}
bool vulkan_shader_add_uniform_u16(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(u16), scope, out_location, false);
}
bool vulkan_shader_add_uniform_u32(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(u32), scope, out_location, false);
}

bool vulkan_shader_add_uniform_f32(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(f32), scope, out_location, false);
}

bool vulkan_shader_add_uniform_vec2(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(vec2), scope, out_location, false);
}

bool vulkan_shader_add_uniform_vec3(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(vec3), scope, out_location, false);
}

bool vulkan_shader_add_uniform_vec4(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(vec4), scope, out_location, false);
}

bool vulkan_shader_add_uniform_mat4(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, sizeof(mat4), scope, out_location, false);
}

bool vulkan_shader_add_uniform_custom(vulkan_shader* shader, const char* uniform_name, u32 size, shader_scope scope, u32* out_location)
{
    VERIFY_UNIFORM(shader, uniform_name, out_location);
    return shader_uniform_add(shader, uniform_name, size, scope, out_location, false);
}

bool vulkan_shader_initialize(vulkan_shader* shader)
{
    if(!shader)
    {
        kerror("Function '%s' requires a valid pointer to shader.", __FUNCTION__);
        return false;
    }

    vulkan_context* context = shader->context;
    VkDevice logical = context->device.logical;
    VkAllocationCallbacks* vk_allocator = context->allocator;

    for(u32 i = 0; i < shader->config.stage_count; ++i)
    {
        if(!shader_module_create(shader, shader->config.stages[i], &shader->stages[i]))
        {
            kerror(
                "Function '%s': Unable to create %s shader module for '%s'. Shader will be destroyed.",
                __FUNCTION__, shader->config.stages[i].stage_str, shader->name
            );
            FAIL_DESTROY(shader);
        }
    }

    // Пул дескрипторов.
    VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = shader->config.pool_sizes;
    pool_info.maxSets = shader->config.max_descriptor_set_count;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // Создание пула дескрипторов.
    VkResult result = vkCreateDescriptorPool(logical, &pool_info, vk_allocator, &shader->descriptor_pool);
    if(!vulkan_result_is_success(result))
    {
        kerror("Function '%s': Failed creating descriptor pool: '%s'", __FUNCTION__, vulkan_result_get_string(result, true));
        FAIL_DESTROY(shader);
    }

    for(u32 i = 0; i < shader->config.descriptor_set_count; ++i)
    {
        VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layout_info.bindingCount = shader->config.descriptor_sets[i].binding_count;
        layout_info.pBindings = shader->config.descriptor_sets[i].bindings;
        result = vkCreateDescriptorSetLayout(logical, &layout_info, vk_allocator, &shader->descriptor_set_layouts[i]);
        if(!vulkan_result_is_success(result))
        {
            kerror("Function '%s': Failed creating descriptor pool: '%s'", __FUNCTION__, vulkan_result_get_string(result, true));
            FAIL_DESTROY(shader);
        }
    }

    // TODO: Конфигурируемым.
    // Область просмотра: как будет растянуто изображение во фрэймбуфере.
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context->framebuffer_height;
    viewport.width = (f32)context->framebuffer_width;
    viewport.height = -(f32)context->framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Область отсечения: какие пиксели будут сокранены, во время растеризации.
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context->framebuffer_width;
    scissor.extent.height = context->framebuffer_height;

    // NOTE: Должно соответствовать количеству стадий шейдеров (shader->stages).
    VkPipelineShaderStageCreateInfo stage_create_infos[VULKAN_SHADER_MAX_STAGES];
    kzero_tc(stage_create_infos, VkPipelineShaderStageCreateInfo, VULKAN_SHADER_MAX_STAGES);
    for(u32 i = 0; i < shader->config.stage_count; ++i)
    {
        stage_create_infos[i] = shader->stages[i].shader_stage_create_info;
    }

    bool pipeline_result = vulkan_graphics_pipeline_create(
        context, shader->renderpass, shader->config.attribute_stride, shader->config.attribute_count,
        shader->config.attributes, shader->config.descriptor_set_count, shader->descriptor_set_layouts,
        shader->config.stage_count, stage_create_infos, viewport, scissor, false, true, shader->config.push_constant_range_count,
        shader->config.push_constant_ranges, &shader->pipeline
    );

    if(!pipeline_result)
    {
        kerror("Function '%s': Failed to load graphics pipeline for object shader.", __FUNCTION__);
        return false;
    }

    // Получение ближайшего допустимого шага (экземпляра).
    shader->global_ubo_stride = 0;
    while(shader->global_ubo_stride < shader->global_ubo_size)
    {
        shader->global_ubo_stride += shader->required_ubo_aligment;
    }

    // Получение ближайшего допустимого шага (экземпляра).
    if(shader->use_instances)
    {
        shader->ubo_stride = 0;
        while(shader->ubo_stride < shader->ubo_size)
        {
            shader->ubo_stride += shader->required_ubo_aligment;
        }
    }

    // Создание uniform буфера.
    u32 device_local_bit = context->device.memory_local_host_visible_support ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
    // TODO: Максимальное количество должно быть настраиваемым или должна быть долгосрочная поддержка изменения размера буфера.
    u64 total_buffer_size = shader->global_ubo_stride + (shader->ubo_stride * VULKAN_SHADER_MAX_MATERIAL_COUNT);

    if(!vulkan_buffer_create(
        context, total_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | device_local_bit, 
        true, &shader->uniform_buffer
    ))
    {
        kerror("Function '%s': Failed to create vulkan buffer for object shader.", __FUNCTION__);
        return false;
    }

    if(!vulkan_buffer_allocate(&shader->uniform_buffer, shader->global_ubo_stride, &shader->global_ubo_offset))
    {
        kerror("Function '%s': Failed to allocate space for the uniform buffer!", __FUNCTION__);
        return false;
    }

    shader->uniform_buffer_mapped_block = vulkan_buffer_lock_memory(context, &shader->uniform_buffer, 0, total_buffer_size, 0);

    // Выделяем глобальные наборы дескрипторов.
    VkDescriptorSetLayout global_layouts[5] = { // TODO: image_count == 5!
        shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL],
        shader->descriptor_set_layouts[DESC_SET_INDEX_GLOBAL]
    };

    VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = shader->descriptor_pool;
    allocate_info.descriptorSetCount = 5;       // TODO: image_count == 5!
    allocate_info.pSetLayouts = global_layouts;

    result = vkAllocateDescriptorSets(logical, &allocate_info, shader->global_descriptor_sets);
    if(result != VK_SUCCESS)
    {
        kerror("Function '%s': Failed to allocate descriptor sets: %s", __FUNCTION__, vulkan_result_get_string(result, true));
        return false;
    }

    shader->state = VULKAN_SHADER_STATE_INITIALIZED;
    return true;
}

bool vulkan_shader_use(vulkan_shader* shader)
{
    if(!shader)
    {
        kerror("Function '%s' requires a vaild pointer to shader.", __FUNCTION__);
        return false;
    }

    vulkan_context* context = shader->context;
    u32 image_index = context->image_index;

    vulkan_pipeline_bind(&context->graphics_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
    return true;
}

bool vulkan_shader_bind_globals(vulkan_shader* shader)
{
    if(!shader)
    {
        kerror("Function '%s' requires a vaild pointer to shader.", __FUNCTION__);
        return false;
    }

    shader->bound_ubo_offset = shader->global_ubo_offset;
    return true;
}

bool vulkan_shader_bind_instance(vulkan_shader* shader, u32 instance_id)
{
    if(!shader)
    {
        kerror("Function '%s' requires a vaild pointer to shader.", __FUNCTION__);
        return false;
    }

    shader->bound_instance_id = instance_id;
    vulkan_shader_instance_state* object_state = &shader->instance_states[instance_id];
    shader->bound_ubo_offset = object_state->offset;
    return true;
}

bool vulkan_shader_apply_globals(vulkan_shader* shader)
{
    vulkan_context* context = shader->context;
    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;
    VkDescriptorSet global_descriptor = shader->global_descriptor_sets[image_index];

    VkDescriptorBufferInfo buffer_info;
    buffer_info.buffer = shader->uniform_buffer.handle;
    buffer_info.offset = shader->global_ubo_offset;
    buffer_info.range  = shader->global_ubo_stride;

    VkWriteDescriptorSet ubo_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    ubo_write.dstSet = shader->global_descriptor_sets[image_index];
    ubo_write.dstBinding = 0;
    ubo_write.dstArrayElement = 0;
    ubo_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_write.descriptorCount = 1;
    ubo_write.pBufferInfo = &buffer_info;

    VkWriteDescriptorSet descriptor_writes[2];
    descriptor_writes[0] = ubo_write;

    u32 global_set_binding_count = shader->config.descriptor_sets[DESC_SET_INDEX_GLOBAL].binding_count;
    if(global_set_binding_count > 1)
    {
        global_set_binding_count = 1;
        kerror("Function '%s': Global image samplers are not yet supported.", __FUNCTION__);

        // TODO:
        // VkWriteDescriptorSet sampler_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        // descriptor_writes[1] = ...
    }

    vkUpdateDescriptorSets(context->device.logical, global_set_binding_count, descriptor_writes, 0, null);

    vkCmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.layout, 0, 1, &global_descriptor, 0, null
    );

    return true;
}

bool vulkan_shader_apply_instance(vulkan_shader* shader)
{
    if(!shader->use_instances)
    {
        kerror("Function '%s': This shader does not use instances.", __FUNCTION__);
        return false;
    }

    vulkan_context* context = shader->context;
    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;

    vulkan_shader_instance_state* object_state = &shader->instance_states[shader->bound_instance_id];
    VkDescriptorSet object_descriptor_set = object_state->descriptor_set_state.descriptor_sets[image_index];

    VkWriteDescriptorSet descriptor_writes[2]; // Всегда максимум 2 набора дескрипторов.
    kzero_tc(descriptor_writes, VkWriteDescriptorSet, 2);
    u32 descriptor_count = 0;
    u32 descriptor_index = 0;

    // Дескриптор 0 - Uniform буфер.
    // Выполнять только если это дескриптор еще не был обновлен.
    u8* instance_ubo_generation = &object_state->descriptor_set_state.descriptor_states[descriptor_index].generations[image_index];
    if(*instance_ubo_generation == INVALID_ID_U8 /* || *global_ubo_generation != material->generation */)
    {
        VkDescriptorBufferInfo buffer_info;
        buffer_info.buffer = shader->uniform_buffer.handle;
        buffer_info.offset = object_state->offset;
        buffer_info.range = shader->ubo_stride;

        VkWriteDescriptorSet ubo_descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        ubo_descriptor.dstSet = object_descriptor_set;
        ubo_descriptor.dstBinding = descriptor_index;
        ubo_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_descriptor.descriptorCount = 1;
        ubo_descriptor.pBufferInfo = &buffer_info;

        descriptor_writes[descriptor_count] = ubo_descriptor;
        descriptor_count++;

        *instance_ubo_generation = 1;
    }
    descriptor_index++;

    if(shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].binding_count > 1)
    {
        u32 total_sampler_count = shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
        u32 update_sampler_count = 0;
        VkDescriptorImageInfo image_infos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];

        for(u32 i = 0; i < total_sampler_count; ++i)
        {
            texture* t = shader->instance_states[shader->bound_instance_id].instance_textures[i];
            vulkan_texture_data* internal_data = t->internal_data;

            image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[i].imageView = internal_data->image.view;
            image_infos[i].sampler = internal_data->sampler;

            // TODO:
            // if(t->generation != INVALID_ID)
            // {
            //     *descriptor_generation = t->generation;
            //     *descriptor_id = t->id;
            // }

            update_sampler_count++;
        }

        VkWriteDescriptorSet sampler_descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        sampler_descriptor.dstSet = object_descriptor_set;
        sampler_descriptor.dstBinding = descriptor_index;
        sampler_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_descriptor.descriptorCount = update_sampler_count;
        sampler_descriptor.pImageInfo = image_infos;

        descriptor_writes[descriptor_count] = sampler_descriptor;
        descriptor_count++;
    }

    if(descriptor_count > 0)
    {
        vkUpdateDescriptorSets(context->device.logical, descriptor_count, descriptor_writes, 0, null);
    }

    vkCmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.layout, 1, 1, &object_descriptor_set, 0, null
    );

    return true;
}

bool vulkan_shader_acquire_instance_resources(vulkan_shader* shader, u32* out_instance_id)
{
    *out_instance_id = INVALID_ID;
    for(u32 i = 0; i < 1024; ++i)
    {
        if(shader->instance_states[i].id == INVALID_ID)
        {
            shader->instance_states[i].id = i;
            *out_instance_id = i;
            break;
        }
    }

    if(*out_instance_id == INVALID_ID)
    {
        kerror("Function '%s': Failed to acuire new id.", __FUNCTION__);
        return false;
    }

    vulkan_shader_instance_state* instance_state = &shader->instance_states[*out_instance_id];
    u32 instance_texture_count = shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
    instance_state->instance_textures = kallocate_tc(texture*, shader->instance_texture_count, MEMORY_TAG_ARRAY);

    texture* default_texture = texture_system_get_default_texture();
    for(u32 i = 0; i < instance_texture_count; ++i)
    {
        instance_state->instance_textures[i] = default_texture;
    }

    u64 size = shader->ubo_stride;
    if(!vulkan_buffer_allocate(&shader->uniform_buffer, size, &instance_state->offset))
    {
        kerror("Function '%': Failed t oacuire ubo space. ", __FUNCTION__);
        return false;
    }

    vulkan_shader_descriptor_set_state* set_state = &instance_state->descriptor_set_state;

    u32 binding_count = shader->config.descriptor_sets[DESC_SET_INDEX_INSTANCE].binding_count;
    kzero_tc(set_state->descriptor_states, vulkan_descriptor_state, VULKAN_SHADER_MAX_BINDINGS);

    for(u32 i = 0; i < binding_count; ++i)
    {
        for(u32 j = 0; j < 5; ++j)
        {
            set_state->descriptor_states[i].generations[j] = INVALID_ID_U8;
            set_state->descriptor_states[i].ids[j] = INVALID_ID;
        }
    }

    VkDescriptorSetLayout layouts[5] = {
        shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE],
        shader->descriptor_set_layouts[DESC_SET_INDEX_INSTANCE]
    };

    VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = shader->descriptor_pool;
    allocate_info.descriptorSetCount = 5; // TODO: image_count = 5!
    allocate_info.pSetLayouts = layouts;

    VkResult result = vkAllocateDescriptorSets(
        shader->context->device.logical, &allocate_info, instance_state->descriptor_set_state.descriptor_sets
    );
    if(result != VK_SUCCESS)
    {
        kerror(
            "Function '%s': Failed to allocate instance descriptor sets in shader: '%s'",
            __FUNCTION__, vulkan_result_get_string(result, true)
        );
        return false;
    }

    return true;
}

bool vulkan_shader_release_instance_resources(vulkan_shader* shader, u32 instance_id)
{
    vulkan_shader_instance_state* instance_state = &shader->instance_states[instance_id];

    vkDeviceWaitIdle(shader->context->device.logical);

    VkResult result = vkFreeDescriptorSets(
        shader->context->device.logical, shader->descriptor_pool, 5, instance_state->descriptor_set_state.descriptor_sets
    );

    if(result != VK_SUCCESS)
    {
        kerror("Function '%s': Failed to free object shader descriptor sets!", __FUNCTION__);
    }

    kzero_tc(instance_state->descriptor_set_state.descriptor_sets, vulkan_descriptor_state, VULKAN_SHADER_MAX_BINDINGS);

    if(instance_state->instance_textures)
    {
        kfree_tc(instance_state->instance_textures, texture*, shader->instance_texture_count, MEMORY_TAG_ARRAY);
        instance_state->instance_textures = null;
    }

    vulkan_buffer_free(&shader->uniform_buffer, shader->ubo_stride, instance_state->offset);
    instance_state->offset = INVALID_ID;
    instance_state->id = INVALID_ID;

    return true;
}

bool vulkan_shader_set_sampler(vulkan_shader* shader, u32 location, texture* t)
{
    vulkan_uniform_lookup_entry* entry = &shader->uniforms[location];

    if(entry->scope == SHADER_SCOPE_GLOBAL)
    {
        shader->global_textures[entry->location] = t;
    }
    else
    {
        shader->instance_states[shader->bound_instance_id].instance_textures[entry->location] = t;
    }

    return true;
}

u32 vulkan_shader_uniform_location(vulkan_shader* shader, const char* uniform_name)
{
    u32 location = INVALID_ID;
    if(!hashtable_get(shader->uniform_lookup, uniform_name, &location) || location == INVALID_ID)
    {
        kerror(
            "Function '%s': Shader '%s' does not have a registered uniform named '%s'",
            __FUNCTION__, shader->name, uniform_name
        );
        return INVALID_ID;
    }
    return location;
}

bool vulkan_shader_set_uniform_i8(vulkan_shader* shader, u32 location, i8 value)
{
    u32 size = sizeof(i8);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_i16(vulkan_shader* shader, u32 location, i16 value)
{
    u32 size = sizeof(i16);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_i32(vulkan_shader* shader, u32 location, i32 value)
{
    u32 size = sizeof(i32);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_u8(vulkan_shader* shader, u32 location, u8 value)
{
    u32 size = sizeof(u8);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_u16(vulkan_shader* shader, u32 location, u16 value)
{
    u32 size = sizeof(u16);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_u32(vulkan_shader* shader, u32 location, u32 value)
{
    u32 size = sizeof(u32);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_f32(vulkan_shader* shader, u32 location, f32 value)
{
    u32 size = sizeof(f32);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_vec2(vulkan_shader* shader, u32 location, vec2 value)
{
    u32 size = sizeof(vec2);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_vec2f(vulkan_shader* shader, u32 location, f32 value_0, f32 value_1)
{
    u32 size = sizeof(vec2);
    vec2 value = (vec2){{value_0, value_1}};
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_vec3(vulkan_shader* shader, u32 location, vec3 value)
{
    u32 size = sizeof(vec3);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_vec3f(vulkan_shader* shader, u32 location, f32 value_0, f32 value_1, f32 value_2)
{
    u32 size = sizeof(vec3);
    vec3 value = (vec3){{value_0, value_1, value_2}};
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_vec4(vulkan_shader* shader, u32 location, vec4 value)
{
    u32 size = sizeof(vec4);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_vec4f(vulkan_shader* shader, u32 location, f32 value_0, f32 value_1, f32 value_2, f32 value_3)
{
    u32 size = sizeof(vec4);
    vec4 value = (vec4){{value_0, value_1, value_2, value_3}};
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_mat4(vulkan_shader* shader, u32 location, mat4 value)
{
    u32 size = sizeof(mat4);
    return shader_uniform_set(shader, location, &value, size);
}

bool vulkan_shader_set_uniform_custom(vulkan_shader* shader, u32 location, void* value)
{
    return shader_uniform_set(shader, location, value, 0);
}

bool shader_module_create(vulkan_shader* shader, vulkan_shader_stage_config config, vulkan_shader_stage* shader_stage)
{
    char file_name[512];
    string_format(file_name, "shaders/%s.%s.spv", shader->name, config.stage_str);

    resource binary_resource;
    if(!resource_system_load(file_name, RESOURCE_TYPE_BINARY, &binary_resource))
    {
        kerror("Function '%s': Unable to read shader module '%s'.", __FUNCTION__, file_name);
        return false;
    }

    kzero_tc(&shader_stage->create_info, VkShaderModuleCreateInfo, 1);
    shader_stage->create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_stage->create_info.codeSize = binary_resource.data_size;
    shader_stage->create_info.pCode = (u32*)binary_resource.data;

    VkResult result = vkCreateShaderModule(
        shader->context->device.logical, &shader_stage->create_info, shader->context->allocator, &shader_stage->handle
    );
    if(!vulkan_result_is_success(result))
    {
        kerror(
            "Function '%s': Failed to create shader module '%s' with result: %s.",
            __FUNCTION__, file_name, vulkan_result_get_string(result, true)
        );
        return false;
    }

    resource_system_unload(&binary_resource);

    kzero_tc(&shader_stage->shader_stage_create_info, VkPipelineShaderStageCreateInfo, 1);
    shader_stage->shader_stage_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage->shader_stage_create_info.stage  = config.stage;
    shader_stage->shader_stage_create_info.module = shader_stage->handle;
    shader_stage->shader_stage_create_info.pName  = "main"; // TODO: Сделать настраиваемым.

    ktrace("Function '%s': Shader module '%s' loaded successfully.", __FUNCTION__, file_name);
    return true;
}

bool shader_uniform_name_valid(vulkan_shader* shader, const char* uniform_name)
{
    if(!uniform_name || !string_length(uniform_name))
    {
        kerror("Function '%s': Uniform name must exist.", __FUNCTION__);
        return false;
    }

    u32 location;

    if(hashtable_get(shader->uniform_lookup, uniform_name, &location)) // && location != INVALID_ID)
    {
        kerror(
            "Function '%s': A uniform by the name '%s' already exists on shader '%s'.",
            __FUNCTION__, uniform_name, shader->name
        );
        return false;
    }

    return true;
}

bool shader_uniform_add_state_valid(vulkan_shader* shader)
{
    if(shader->state != VULKAN_SHADER_STATE_UNINITIALIZED)
    {
        kerror("Function '%s': Uniforms may only be added to shaders before initialization.", __FUNCTION__);
        return false;
    }
    return true;
}

bool shader_uniform_add(
    vulkan_shader* shader, const char* uniform_name, u32 size, shader_scope scope, u32* out_location,
    bool is_sampler
)
{
    if(shader->uniform_count + 1 > VULKAN_SHADER_MAX_UNIFORMS)
    {
        kerror(
            "Function '%s': A shader can only accept a combined maximum of %d uniforms and samplers at global, instance and local scopes.",
            __FUNCTION__
        );
        return false;
    }

    vulkan_uniform_lookup_entry entry;
    entry.index = shader->uniform_count;
    entry.scope = scope;

    bool is_global = (scope == SHADER_SCOPE_GLOBAL);

    if(is_sampler)
    {
        entry.location = *out_location;
    }
    else
    {
        entry.location = entry.index;
    }

    if(scope != SHADER_SCOPE_LOCAL)
    {
        entry.set_index = (u8)scope;
        entry.offset = is_sampler ? 0 : is_global ? shader->global_ubo_size : shader->ubo_size;
        entry.size = is_sampler ? 0 : size;
    }
    else
    {
        if(entry.scope == SHADER_SCOPE_LOCAL && !shader->use_push_constants)
        {
            kerror(
                "Function '%s' Cannot add a locally-scoped uniform for a shader that does not support locals.",
                __FUNCTION__
            );
            return false;
        }

        // Вставка нового выровненного диапазона (выровнено по 4, как того требует спецификация Vulkan).
        entry.set_index = INVALID_ID_U8;
        range r = get_aligned_range(shader->push_constants_size, size, 4);
        entry.offset = r.offset;
        entry.size = r.size;

        shader->config.push_constant_ranges[shader->config.push_constant_range_count] = r;
        shader->config.push_constant_range_count++;

        shader->push_constant_count += r.size;
    }

    if(!hashtable_set(shader->uniform_lookup, uniform_name, &entry.index, true))
    {
        kerror("Function '%s': Failed to add uniform.", __FUNCTION__);
        return false;
    }

    shader->uniforms[shader->uniform_count] = entry;
    shader->uniform_count++;

    if(!is_sampler)
    {
        if(entry.scope == SHADER_SCOPE_GLOBAL)
        {
            shader->global_ubo_size += entry.size;
        }
        else if(entry.scope == SHADER_SCOPE_INSTANCE)
        {
            shader->ubo_size += entry.size;
        }
    }

    *out_location = entry.index;
    return true;
}

bool shader_uniform_check_size(vulkan_shader* shader, u32 location, u32 expected_size)
{
    if(expected_size == 0)
    {
        // Байпас для пропуска проверки.
        return true;
    }

    vulkan_uniform_lookup_entry* entry = &shader->uniforms[location];

    if(entry->size != expected_size)
    {
        kerror(
            "Function '%s': Uniform location '%d' on shader '%s' is a different size (%d B) than expected (%d B).",
            __FUNCTION__, location, shader->name, entry->size, expected_size
        );
        return false;
    }

    return true;
}

bool shader_uniform_set(vulkan_shader* shader, u32 location, void* value, u64 size)
{
    if(!shader_uniform_check_size(shader, location, size))
    {
        return false;
    }

    void* block = 0;
    vulkan_uniform_lookup_entry* entry = &shader->uniforms[location];
    if(entry->scope == SHADER_SCOPE_GLOBAL)
    {
        block = (void*)(shader->uniform_buffer_mapped_block + shader->global_ubo_offset + entry->offset);
    }
    else if(entry->scope == SHADER_SCOPE_INSTANCE)
    {
        block = (void*)(shader->uniform_buffer_mapped_block + shader->bound_ubo_offset + entry->offset);
    }
    else
    {
        VkCommandBuffer command_buffer = shader->context->graphics_command_buffers[shader->context->image_index].handle;
        vkCmdPushConstants(
            command_buffer, shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            entry->offset, entry->size, value
        );
        return true;
    }

    kcopy(block, value, size);
    return true;
}
