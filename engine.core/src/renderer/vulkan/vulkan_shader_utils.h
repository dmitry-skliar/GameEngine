#pragma once

#include <defines.h>
#include <renderer/vulkan/vulkan_types.h>

/*
*/
bool vulkan_shader_module_create(
    vulkan_context* context, const char* name, const char* type_str, VkShaderStageFlagBits stage_flag, u32 stage_index,
    vulkan_shader_stage* shader_stage
);
