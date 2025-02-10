// Собственные подключения.
#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "platform/file.h"

bool vulkan_shader_module_create(
    vulkan_context* context, const char* name, const char* type_str, VkShaderStageFlagBits stage_flag, u32 stage_index,
    vulkan_shader_stage* shader_stage
)
{
    char filename[512];
    string_format(filename, "../assets/shaders/%s.%s.spv", name, type_str);

    kzero_tc(&shader_stage[stage_index].handleinfo, VkShaderModuleCreateInfo, 1);
    shader_stage[stage_index].handleinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    file* handle = null;
    if(!platform_file_open(filename, FILE_MODE_READ | FILE_MODE_BINARY, &handle))
    {
        kerror("Function '%': Unable to read shader module '%s'.", __FUNCTION__, filename);
        return false;
    }

    u64 file_size = platform_file_size(handle);
    void* file_buffer = kallocate(file_size, MEMORY_TAG_ARRAY);

    if(!platform_file_reads(handle, file_buffer, &file_size))
    {
        kerror("Function '%s': Unable to read shader module '%s'.", __FUNCTION__, filename);
        return false;
    }

    shader_stage[stage_index].handleinfo.codeSize = file_size;
    shader_stage[stage_index].handleinfo.pCode = (u32*)file_buffer;

    platform_file_close(handle);
    handle = null;

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

    kzero_tc(&shader_stage[stage_index].pipelineinfo, VkPipelineShaderStageCreateInfo, 1);
    shader_stage[stage_index].pipelineinfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage[stage_index].pipelineinfo.stage  = stage_flag;
    shader_stage[stage_index].pipelineinfo.module = shader_stage[stage_index].handle;
    shader_stage[stage_index].pipelineinfo.pName  = "main"; // TODO: Сделать настраиваемым.

    if(file_buffer)
    {
        kfree(file_buffer, file_size, MEMORY_TAG_ARRAY);
        file_buffer = null;
    }

    ktrace("Function '%s': Shader module '%s' loaded successfully.", __FUNCTION__, filename);
    return true;
}
