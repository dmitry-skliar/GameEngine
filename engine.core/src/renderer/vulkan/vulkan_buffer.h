#pragma once

#include <defines.h>
#include <renderer/vulkan/vulkan_types.h>

/*
*/
bool vulkan_buffer_create(
    vulkan_context* context, u64 size, VkBufferUsageFlagBits usage, u32 memory_property_flags,
    bool bind_on_create, vulkan_buffer* out_buffer
);

/*
*/
void vulkan_buffer_destroy(vulkan_context* context, vulkan_buffer* buffer);

/*
*/
bool vulkan_buffer_resize(
    vulkan_context* context, u64 new_size, vulkan_buffer* buffer, VkQueue queue, VkCommandPool pool
);

/*
*/
void vulkan_buffer_bind(vulkan_context* context, vulkan_buffer* buffer, u64 offset);

/*
*/
void* vulkan_buffer_lock_memory(vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags);

/*
*/
void vulkan_buffer_unlock_memory(vulkan_context* context, vulkan_buffer* buffer);

/*
*/
bool vulkan_buffer_allocate(vulkan_buffer* buffer, u64 size, u64* out_offset);

/*
*/
bool vulkan_buffer_free(vulkan_buffer* buffer, u64 size, u64 offset);

/*
*/
void vulkan_buffer_load_data(
    vulkan_context* context, vulkan_buffer* buffer, u64 offset, u64 size, u32 flags, const void* data
);

/*
*/
void vulkan_buffer_copy_to(
    vulkan_context* context, VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer source, u64 source_offset,
    VkBuffer destination, u64 destination_offset, u64 size
);
