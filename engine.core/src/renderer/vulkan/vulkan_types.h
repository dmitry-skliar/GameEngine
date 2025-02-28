#pragma once

#include <defines.h>
#include <debug/assert.h>
#include <vulkan/vulkan.h>
#include <containers/freelist.h>
#include <renderer/renderer_types.h>

/*
    @brief Проверяет возвразаемое значение указанного выражения на соответствие VK_SUCCESS.
    @param Выражение, результат которого следует проверить. 
*/
#define VK_CHECK(expr) kassert(expr == VK_SUCCESS, "")

struct vulkan_context;

/*
    @brief Контекст буфер Vulkan.
    NOTE: Используется для загрузки данных на видеокарту.
*/
typedef struct vulkan_buffer {
    // @brief Экземпляр буфера.
    VkBuffer handle;
    // @brief Флаги использования буфера.
    VkBufferUsageFlagBits usage;
    // @brief Указывает если буфер заблокирован.
    bool is_locked;
    // @brief Размер буфера.
    u64 total_size;
    // @brief Используемая память для буфера.
    VkDeviceMemory memory;
    // @brief Индекс используемый буфером.
    i32 memory_index;
    // @brief Флаги памяти.
    u32 memory_property_flags;
    // @brief Требования памяти списка.
    u64 freelist_memory_requirement;
    // @brief Блок памяти выделенный под список.
    void* freelist_memory;
    // @brief Экземпляр списка.
    freelist* buffer_freelist;
    //
    bool has_freelist;
} vulkan_buffer;

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

// @brief Контекст конвейера.
typedef struct vulkan_pipeline {
    // @brief Экземпляр конвейера.
    VkPipeline handle;
    // @brief ...
    VkPipelineLayout layout;
} vulkan_pipeline;

#define VULKAN_SHADER_MAX_NAME_LENGTH       256
#define VULKAN_SHADER_MAX_MATERIAL_COUNT    1024 
#define VULKAN_SHADER_MAX_GEOMETRY_COUNT    4096
#define VULKAN_SHADER_MAX_GLOBAL_TEXTURES   31
#define VULKAN_SHADER_MAX_INSTANCE_TEXTURES 31
#define VULKAN_SHADER_MAX_STAGES            8
#define VULKAN_SHADER_MAX_ATTRIBUTES        16
#define VULKAN_SHADER_MAX_UNIFORMS          128
#define VULKAN_SHADER_MAX_BINDINGS          2
#define VULKAN_SHADER_MAX_PUSH_CONST_RANGES 32
#define VULKAN_SHADER_MAX_DESCRIPTOR_COUNT  2
#define VULKAN_SHADER_SAMPLER_COUNT         1
#define VULKAN_SHADER_MAX_UI_COUNT          1024

// @brief Контекст данных геометрии в буфере.
typedef struct vulkan_geometry_data {
    // @brief Уникальный идентификатор геометрии.
    u32 id;
    // @brief Счетчик изменений. Увеличивается постоянно.
    u32 generation;
    // @brief Количество вершин в буфере вершин.
    u32 vertex_count;
    // @brief Размер вершины в байтах.
    u32 vertex_element_size;
    // @brief Смещение вершин в буфере вершин.
    u64 vertex_buffer_offset;
    // @brief Количество индексов в буфере индексов.
    u32 index_count;
    // @brief Размер индекса в байтах.
    u32 index_element_size;
    // @brief Смещение индексов в буфере индексов.
    u64 index_buffer_offset;
} vulkan_geometry_data;

// @brief Конкретная конфигурация стадии шейдера на конвейере.
typedef struct vulkan_shader_stage_config {
    // @brief Стадия конвейера.
    VkShaderStageFlagBits stage;
    // @brief Имя файла стадии.
    char file_name[255];
} vulkan_shader_stage_config;

// @brief Конфигурация набора дескрипторов.
typedef struct vulkan_descriptor_set_config {
    // @brief Количество прикреплений.
    u8 binding_count;
    // @brief Прикрепления.
    VkDescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDINGS];
} vulkan_descriptor_set_config;

// @brief Конфигурация шейдера (перед этапом инициализации).
typedef struct vulkan_shader_config {
    // @brief Количество стадий.
    u8 stage_count;
    // @brief Массив стадий конвейера.
    vulkan_shader_stage_config stages[VULKAN_SHADER_MAX_STAGES];
    // @brief Массив резмеров пулов дескрипторных наборов (ubo, image sampler).
    VkDescriptorPoolSize pool_sizes[2];
    // @brief Максимальное количество дескрипторных наборов, которое может предоставить шейдер.
    u16 max_descriptor_set_count;
    // @brief Количество дескрипторов.
    u8 descriptor_set_count;
    // @brief Массив наборов дескрипторов (0 - глобальные, 1- экземпляров).
    vulkan_descriptor_set_config descriptor_sets[2];
    // @brief Массив атрибутов шейдерного модуля.
    VkVertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];
} vulkan_shader_config;

// @brief Состояние дескриптора на кадре.
typedef struct vulkan_descriptor_state {
    u8 generations[5];                    // TODO: image_count == 5!
    u32 ids[5];
} vulkan_descriptor_state;

// @brief Состояние набора дескрипторов. Используется для отслеживания обновлений, пропуская те, которые не нуждаются!
typedef struct vulkan_shader_descriptor_set_state {
    VkDescriptorSet descriptor_sets[5];    // TODO: image_count == 5!
    vulkan_descriptor_state descriptor_states[VULKAN_SHADER_MAX_BINDINGS];
} vulkan_shader_descriptor_set_state;

// @brief Состояние шейдера на уровне экземпляра.
typedef struct vulkan_shader_instance_state {
    // @brief Идентификатор экземпляра.
    u32 id;
    // @brief Смещение в байтах в uniform-буфере.
    u64 offset;
    // @brief Состояние набора дескрипторов.
    vulkan_shader_descriptor_set_state descriptor_set_state;
    // @brief Указатель на текстуры экземпляра.
    texture** instance_textures;
} vulkan_shader_instance_state;

// @brief Стадия конкретного модуля шейдера (+конвейер).
typedef struct vulkan_shader_stage {
    // @brief Модуль шейдера (после связывания с pipline может быть уничтожен).
    VkShaderModule handle;
    // @brief Информация модуля шейдера.
    VkShaderModuleCreateInfo create_info;
    // @brief Информация модуля для pipeline.
    VkPipelineShaderStageCreateInfo shader_stage_create_info;
} vulkan_shader_stage;

// @brief Контекст шейдера.
typedef struct vulkan_shader {
    // @brief Идентификатор шейдера.
    u32 id;
    // @brief Конкретные cтадии шейдера (вершиная, фрагментная ...).
    vulkan_shader_stage stages[VULKAN_SHADER_MAX_STAGES];
    // @brief Конфигурация шейдера (перед этапом инициализации).
    vulkan_shader_config config;
    // @brief Пул дескрипторов.
    VkDescriptorPool descriptor_pool;
    // @brief Массив наборов дескрипторов: Индекс 0 - глобально, 1 - локально.
    VkDescriptorSetLayout descriptor_set_layouts[2];
    // @brief Массив набора дескрипторов на кадр.
    VkDescriptorSet global_descriptor_sets[5];       // TODO: image_count == 5!
    // @brief Буфер uniform переменных.
    vulkan_buffer uniform_buffer;
    // @brief Трансляция памяти буфера uniform (привязка).
    void* uniform_buffer_mapped_block;
    // @brief Количество экземпляров.
    u32 instance_count;
    // @brief Массив состояний экземпляров.
    vulkan_shader_instance_state instance_states[VULKAN_SHADER_MAX_MATERIAL_COUNT];
    // @brief Конвейер визуализации привязаный к шейдеру.
    vulkan_pipeline pipeline;
    // @brief Проходчик визуализации.
    vulkan_renderpass* renderpass;
} vulkan_shader;

// @brief Контекст визуализатора.
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
    bool recreating_swapchain;

    // @brief Графические коммандные буферы (используется darray).
    vulkan_command_buffer* graphics_command_buffers;
    // @brief Готовое для визуализации (используется darray).
    VkSemaphore* image_available_semaphores;
    // @brief Завершение визуализации (используется darray).
    VkSemaphore* queue_complete_semaphores;
    // @brief Количество fences.
    u32 in_flight_fence_count;
    // @brief Указатели на fence.
    VkFence in_flight_fences[4];
    // @brief Содержит указатели на существующие fence (принадлежащие кому-то другому).
    VkFence images_in_flight[5]; // TODO: image_count = 5 (временно)!
    // @brief Кадров цепочки обмена.
    u32 image_index;
    // @brief Текущий кадр показа.
    u32 current_frame;

    vulkan_renderpass main_renderpass;
    vulkan_renderpass ui_renderpass;

    vulkan_buffer object_vertex_buffer;
    vulkan_buffer object_index_buffer;

    // TODO: Сделать динамическим размер.
    vulkan_geometry_data geometries[VULKAN_SHADER_MAX_GEOMETRY_COUNT];

    // Кадровые буферы используются для визуализации мире на кадр.
    VkFramebuffer world_framebuffers[5]; // TODO: Потому что image_count == 5!

    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);
} vulkan_context;

// @brief Контекст данных текстуры.
typedef struct vulkan_texture_data {
    vulkan_image image;
    VkSampler sampler;
} vulkan_texture_data;
