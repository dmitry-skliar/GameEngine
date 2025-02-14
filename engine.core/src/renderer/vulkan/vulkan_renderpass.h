#pragma once

#include <defines.h>
#include <renderer/vulkan/vulkan_types.h>

/*
*/
void vulkan_renderpass_create(
    vulkan_context* context, vulkan_renderpass* out_renderpass, vec4 render_area, vec4 clear_color, f32 depth,
    u32 stencil, renderpass_clear_flag clear_flags, bool has_prev_pass, bool has_next_pass
);

/*
*/
void vulkan_renderpass_destroy(vulkan_context* context, vulkan_renderpass* renderpass);

/*
*/
void vulkan_renderpass_begin(
    vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass, VkFramebuffer frame_buffer
);

/*
*/
void vulkan_renderpass_end(vulkan_command_buffer* command_buffer, vulkan_renderpass* renderpass);
