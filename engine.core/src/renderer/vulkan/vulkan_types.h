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

typedef enum renderpass_clear_flag {
    RENDERPASS_CLEAR_NONE_FLAG           = 0x0,
    RENDERPASS_CLEAR_COLOR_BUFFER_FLAG   = 0x1,
    RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG   = 0x2,
    RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG = 0x4,
} renderpass_clear_flag;

typedef struct vulkan_renderpass {
    VkRenderPass handle;
    vec4 render_area;
    vec4 clear_color;
    bool do_clear_color;
    bool do_clear_depth;
    bool do_clear_stencil;
    bool has_prev_pass;
    bool has_next_pass;
    f32 depth;
    f32 stencil;
    vulkan_renderpass_state state;
} vulkan_renderpass;

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
    VkFramebuffer framebuffers[5]; // TODO: image_count == 5!
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
    // @brief Поддержка анизотропной фильтрации.
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

typedef struct vulkan_descriptor_state {
    // На кадр.
    u32 generations[5];                    // TODO: image_count = 5!
    u32 ids[5];
} vulkan_descriptor_state;

#define VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT 2
#define VULKAN_MATERIAL_SHADER_SAMPLER_COUNT 1

typedef struct vulkan_material_shader_instance_state {
    // На кадр.
    VkDescriptorSet descriptor_sets[5];    // TODO: image_count = 5!
    // На дескриптор.
    vulkan_descriptor_state descriptor_states[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
} vulkan_material_shader_instance_state;

// @brief Максимальное количество материальных экземпляров.
// TODO: Сделать настраиваемым.
#define VULKAN_MAX_MATERIAL_COUNT 1024

// @brief Максимальное количество одновременно загружаемых геометрий.
// TODO: Сделать настраиваемым.
#define VULKAN_MAX_GEOMETRY_COUNT 4096

typedef struct vulkan_geometry_data {
    u32 id;
    u32 generation;
    u32 vertex_count;
    u32 vertex_element_size;
    u32 vertex_buffer_offset;
    u32 index_count;
    u32 index_element_size;
    u32 index_buffer_offset;
} vulkan_geometry_data;

typedef struct vulkan_material_shader_global_ubo {
    mat4 projection;       // 64 bytes.
    mat4 view;             // 64 bytes.
    mat4 m_reserved[2];    // 128 bytes зарезервировано.
} vulkan_material_shader_global_ubo;

typedef struct vulkan_material_shader_instance_ubo {
    vec4 diffuse_color;    // 16 bytes.
    vec4 v_reserved[3];    // 48 bytes.
    // mat4 m_reserved[3];
} vulkan_material_shader_instance_ubo;

// TODO: Временно.
typedef enum vulkan_shader_type {
    VULKAN_SHADER_TYPE_MATERIAL,
    VULKAN_SHADER_TYPE_UI
} vulkan_shader_type;

typedef struct vulkan_material_shader {
    // @brief Шаги шейдерных модулей: Vertex, Fragment.
    vulkan_shader_stage stages[MATERIAL_SHADER_STAGE_COUNT];

    VkDescriptorPool global_descriptor_pool;
    VkDescriptorSetLayout global_descriptor_set_layout;
    VkDescriptorSet global_descriptor_sets[5]; // TODO: Потому что image_count = 5 (временно)!
    vulkan_material_shader_global_ubo global_ubo;
    vulkan_buffer global_uniform_buffer;

    VkDescriptorPool object_descriptor_pool;
    VkDescriptorSetLayout object_descriptor_set_layout;
    vulkan_buffer object_uniform_buffer;
    // TODO: Заменить на freelist.
    u32 object_uniform_buffer_index;

    texture_use sampler_uses[VULKAN_MATERIAL_SHADER_SAMPLER_COUNT];

    // TODO: Сделать динамическим.
    vulkan_material_shader_instance_state instance_states[VULKAN_MAX_MATERIAL_COUNT];

    vulkan_pipeline pipeline;

    // TODO: Временно.
    vulkan_shader_type shader_type;    
} vulkan_material_shader;

typedef struct vulkan_context {
    f32 frame_delta_time;
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
    vulkan_renderpass ui_renderpass;

    u64 geometry_vertex_offset;
    vulkan_buffer object_vertex_buffer;
    u64 geometry_index_offset;
    vulkan_buffer object_index_buffer;

    // TODO: Сделать динамическим размер.
    vulkan_geometry_data geometries[VULKAN_MAX_GEOMETRY_COUNT];

    // @brief Графические коммандные буферы (используется darray).
    vulkan_command_buffer* graphics_command_buffers;
    // @brief Готовое для визуализации (используется darray).
    VkSemaphore* image_available_semaphores;
    // @brief Завершение визуализации (используется darray).
    VkSemaphore* queue_complete_semaphores;
    u32 in_flight_fence_count;
    VkFence in_flight_fences[4];
    // Содержит указатели на существующие ограждения, принадлежащие кому-то другому.
    VkFence images_in_flight[5]; // TODO: Потому что image_count = 5 (временно)!
    u32 image_index;
    u32 current_frame;
    bool recreating_swapchain;
    vulkan_material_shader material_shader;
    vulkan_material_shader ui_shader;

    // Кадровые буферы используются для визуализации мире на кадр.
    VkFramebuffer world_framebuffers[5]; // TODO: Потому что image_count == 5!

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);
} vulkan_context;

typedef struct vulkan_texture_data {
    vulkan_image image;
    VkSampler sampler;
} vulkan_texture_data;
