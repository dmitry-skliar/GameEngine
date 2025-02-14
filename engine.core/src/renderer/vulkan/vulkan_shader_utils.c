// Собственные подключения.
#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "systems/resource_system.h"

bool vulkan_shader_module_create(
    vulkan_context* context, const char* name, const char* type_str, VkShaderStageFlagBits stage_flag, u32 stage_index,
    vulkan_shader_stage* shader_stage
)
{
    char filename[512];
    string_format(filename, "shaders/%s.%s.spv", name, type_str);

    // Чтение ресурса.
    resource binary_resource;
    if(!resource_system_load(filename, RESOURCE_TYPE_BINARY, &binary_resource))
    {
        kerror("Function '%s': Unable to read shader module '%s'.", __FUNCTION__, filename);
        return false;
    }

    kzero_tc(&shader_stage[stage_index].handleinfo, VkShaderModuleCreateInfo, 1);
    shader_stage[stage_index].handleinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_stage[stage_index].handleinfo.codeSize = binary_resource.data_size;
    shader_stage[stage_index].handleinfo.pCode = (u32*)binary_resource.data;

    VkResult result = vkCreateShaderModule(
        context->device.logical, &shader_stage[stage_index].handleinfo, context->allocator, &shader_stage[stage_index].handle
    );
    if(!vulkan_result_is_success(result))
    {
        kerror(
            "Function '%s': Failed to create shader module '%s' with result: %s.",
            __FUNCTION__, filename, vulkan_result_get_string(result, true)
        );
        return false;
    }

    resource_system_unload(&binary_resource);

    kzero_tc(&shader_stage[stage_index].pipelineinfo, VkPipelineShaderStageCreateInfo, 1);
    shader_stage[stage_index].pipelineinfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage[stage_index].pipelineinfo.stage  = stage_flag;
    shader_stage[stage_index].pipelineinfo.module = shader_stage[stage_index].handle;
    shader_stage[stage_index].pipelineinfo.pName  = "main"; // TODO: Сделать настраиваемым.

    ktrace("Function '%s': Shader module '%s' loaded successfully.", __FUNCTION__, filename);
    return true;
}
