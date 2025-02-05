#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <vulkan/vulkan.h>

typedef struct vulkan_image {
    VkImage handle;
    VkImageView view;
    VkDeviceMemory memory;
    u32 width;
    u32 height;
} vulkan_image;

typedef enum vulkan_renderpass_state {
    VULKAN_RENDERPASS_STATE_READY,
    VULKAN_RENDERPASS_STATE_RECORDING,
    VULKAN_RENDERPASS_STATE_IN_RENDERPASS,
    VULKAN_RENDERPASS_STATE_RECORDING_ENDED,
    VULKAN_RENDERPASS_STATE_SUBMITTED,
    VULKAN_RENDERPASS_STATE_NOT_ALLOCATED
} vulkan_renderpass_state;

typedef struct vulkan_renderpass {
    VkRenderPass handle;
    f32 x, y, w, h;
    f32 r, g, b, a;
    f32 depth;
    f32 stencil;
    vulkan_renderpass_state state;
} vulkan_renderpass;

typedef struct vulkan_framebuffer {
    VkFramebuffer handle;
    u32 attachment_count;
    // @brief Буферы представлений изображений (используется darray).
    VkImageView* attachments;
    vulkan_renderpass* renderpass;
} vulkan_framebuffer;

typedef struct vulkan_buffer {
    u64 total_size;
    VkBuffer handle;
    VkBufferUsageFlagBits usage;
    bool is_locked;
    VkDeviceMemory memory;
    i32 memory_index;
    u32 memory_property_flags;
} vulkan_buffer;

typedef struct vulkan_swapchain {
    // @brief Количество кадров для визуализации.
    u32 max_frames_in_flight;
    // @brief Количество кадров цепочки.
    u32 image_count;
    // @brief Формат пикселей.
    VkSurfaceFormatKHR image_format;
    // @brief Изображений (используется darray).
    VkImage* images;
    // @brief Представления изображений (используется darray).
    VkImageView* views;
    // @brief Цепочка обмена.
    VkSwapchainKHR handle;
    // @brief Буфер глубины.
    vulkan_image depth_attachment;
    // @brief Кадровые буферы для визуализации на экране (используется darray).
    vulkan_framebuffer* framebuffers;
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state {
    VULKAN_COMMAND_BUFFER_STATE_READY,
    VULKAN_COMMAND_BUFFER_STATE_RECORDING,
    VULKAN_COMMAND_BUFFER_STATE_IN_RENDERPASS,
    VULKAN_COMMAND_BUFFER_STATE_RECORDING_ENDED,
    VULKAN_COMMAND_BUFFER_STATE_SUBMITTED,
    VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED,
} vulkan_command_buffer_state;

typedef struct vulkan_command_buffer {
    VkCommandBuffer handle;
    vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_device_queue {
    // @brief Указатель на очередь.
    VkQueue handle;
    // @brief Указатель на пул команд.
    VkCommandPool command_pool;
    // @brief Индекс семейства очередей.
    u32 index;
    // @brief Количество очередей в семействе.
    u32 count;
} vulkan_device_queue;

// TODO: Вместо массивов darray использовать array.
typedef struct vulkan_device_swapchain_support {
    // @brief Флаги возможностей.
    VkSurfaceCapabilitiesKHR capabilities;
    // @brief Доступное количество форматов пикселей. 
    u32 format_count;
    // @brief Форматы пикселей (используется массив darray, для получения размера использовать darray_get_capacity).
    VkSurfaceFormatKHR* formats;
    // @brief Доступное количество режимов представлений.
    u32 present_mode_count;
    // @brief Режимы изображений представлений (используется массив darray, для получения размера использовать darray_get_capacity).
    VkPresentModeKHR* present_modes;
} vulkan_device_swapchain_support;

// TODO: Так же сделать выбор по индексу видеокарты!
typedef struct vulkan_device_requirements {
    // @brief Поддержка определенного типа физического устройства.
    VkPhysicalDeviceType device_type;
    // @brief Поддержка расширений физического устройства (используется массив darray).
    const char** extensions;
    // @brief Поддержка анизотропии.
    bool sampler_anisotropy;
} vulkan_device_requirements;

typedef struct vulkan_device {
    // @brief Физическое устройство.
    VkPhysicalDevice physical;
    // @brief Логическое устройство.
    VkDevice logical;
    // @brief Поддерживаемые опции цепочки обмена.
    vulkan_device_swapchain_support swapchain_support;
    // @brief Очередь графики.
    vulkan_device_queue graphics_queue;
    // @brief Очередь представлений.
    vulkan_device_queue present_queue;
    // @brief Очередь операций с памятью.
    vulkan_device_queue transfer_queue;
    // @brief Очередь вычислений (не используется).
    vulkan_device_queue compute_queue;
    // @brief Парамерты устройства.
    VkPhysicalDeviceProperties properties;
    // @brief Функции устройства.
    VkPhysicalDeviceFeatures features;
    // @brief Память устройства.
    VkPhysicalDeviceMemoryProperties memory;
    // @brief Поддержка прямой видимости памяти устройства.
    bool memory_local_host_visible_support;
    // @brief Формат буфера глубины.
    VkFormat depth_format;
} vulkan_device;

typedef struct vulkan_fence {
    VkFence handle;
    bool is_signaled;
} vulkan_fence;

typedef struct vulkan_shader_stage {
    VkShaderModule handle;
    VkShaderModuleCreateInfo handleinfo;
    VkPipelineShaderStageCreateInfo pipelineinfo;
} vulkan_shader_stage;

typedef struct vulkan_pipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
} vulkan_pipeline;

#define MATERIAL_SHADER_STAGE_COUNT 2

typedef struct vulkan_material_shader {
    // @brief Шаги шейдерных модулей: Vertex, Fragment.
    vulkan_shader_stage stages[MATERIAL_SHADER_STAGE_COUNT];
    // @brieg Pipeline.
    vulkan_pipeline pipeline;
    // @brief
    VkDescriptorPool global_descriptor_pool;
    // @brief
    VkDescriptorSetLayout global_descriptor_set_layout;
    // @brief
    VkDescriptorSet global_descriptor_sets[5]; // FIX: Потому что image_count = 5 (временно)!
    // @brief Общий uniform object.
    global_uniform_object global_ubo;
    // @brief Общий unform buffer.
    vulkan_buffer global_uniform_buffer;
} vulkan_material_shader;

typedef struct vulkan_context {
    u32 framebuffer_width;
    u32 framebuffer_height;

    u64 framebuffer_size_generation;
    u64 framebuffer_size_last_generation;

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT debug_messenger;
    vulkan_device device;
    vulkan_swapchain swapchain;
    vulkan_renderpass main_renderpass;

    u64 geometry_vertex_offset;
    vulkan_buffer object_vertex_buffer;
    u64 geometry_index_offset;
    vulkan_buffer object_index_buffer;

    // @brief Графические коммандные буферы (используется darray).
    vulkan_command_buffer* graphics_command_buffers;
    // @brief Готовое для визуализации (используется darray).
    VkSemaphore* image_available_semaphores;
    // @brief Завершение визуализации (используется darray).
    VkSemaphore* queue_complete_semaphores;
    u32 in_flight_fence_count;
    vulkan_fence* in_flight_fences;
    // Содержит указатели на существующие ограждения, принадлежащие кому-то другому.
    vulkan_fence** images_in_flight;
    u32 image_index;
    u32 current_frame;
    bool recreating_swapchain;
    vulkan_material_shader material_shader;

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);
} vulkan_context;
