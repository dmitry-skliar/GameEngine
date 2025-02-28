// Cобственные подключения.
#include "application.h"

// Внутренние подключения.
#include "logger.h"
#include "event.h"
#include "input.h"
#include "clock.h"
#include "platform/window.h"
#include "platform/time.h"
#include "platform/thread.h"
#include "memory/memory.h"
#include "memory/allocators/linear_allocator.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

// TODO: Временный тестовый код: начало.
#include "kstring.h"
#include "math/kmath.h"
#include "math/geometry_utils.h"
// TODO: Временный тестовый код: конец.

typedef struct application_state {
    game* game_inst;
    bool  is_running;
    bool  is_suspended;
    clock clock;
    f64   last_time;

    linear_allocator* systems_allocator;

    u64 event_system_memory_requirement;
    void* event_system_state;

    u64 input_system_memory_requirement;
    void* input_system_state;

    u64 platform_window_memory_requirement;
    window* platform_window_state;

    u64 resource_system_memory_requirement;
    void* resource_system_state;

    u64 shader_system_memory_requirement;
    void* shader_system_state;

    u64 renderer_system_memory_requirement;
    void* renderer_system_state;

    u64 texture_system_memory_requirement;
    void* textute_system_state;

    u64 material_system_memory_requirement;
    void* material_system_state;

    u64 geometry_system_memory_requirement;
    void* geometry_system_state;

    // TODO: Временный тестовый код: начало.
    geometry* test_geometry;
    geometry* test_ui_geometry;
    // TODO: Временный тестовый код: конец.

} application_state;

static application_state* app_state = null;

void application_on_focus(bool focused);
void application_on_mouse_wheel(i32 zdelta);
void application_on_mouse_button(u32 button, bool pressed);
void application_on_mouse_move(i32 x, i32 y);
void application_on_keyboard_key(u32 keycode, bool pressed);
void application_on_resize(i32 width, i32 height);
void application_on_close();

// TODO: Временный тестовый код: начало.
bool event_on_debug_event(event_code code, void* sender, void* listener_inst, event_context* context)
{
    const char* diff_names[3] = { "cobblestone",      "paving",      "paving2"      };
    const char* spec_names[3] = { "cobblestone_SPEC", "paving_SPEC", "paving2_SPEC" };
    const char* norm_names[3] = { "cobblestone_NRM",  "paving_NRM",  "paving2_NRM"  };

    static i8 choice = 1;
    const char* old_diff_name = diff_names[choice];
    const char* old_spec_name = spec_names[choice];
    const char* old_norm_name = norm_names[choice];

    choice++;
    choice %= 3;

    if(app_state->test_geometry)
    {
        // Diffuse.
        app_state->test_geometry->material->diffuse_map.texture = texture_system_acquire(diff_names[choice], true);
        if(!app_state->test_geometry->material->diffuse_map.texture)
        {
            kwarng("Function '%s': Failed to load diffuse texture. Using default! ", __FUNCTION__);
            app_state->test_geometry->material->diffuse_map.texture = texture_system_get_default_diffuse_texture();
        }
        texture_system_release(old_diff_name);

        // Specular.
        app_state->test_geometry->material->specular_map.texture = texture_system_acquire(spec_names[choice], true);
        if(!app_state->test_geometry->material->specular_map.texture)
        {
            kwarng("Function '%s': Failed to load specular texture. Using default! ", __FUNCTION__);
            app_state->test_geometry->material->specular_map.texture = texture_system_get_default_specular_texture();
        }
        texture_system_release(old_spec_name);

        // Normal.
        app_state->test_geometry->material->normal_map.texture = texture_system_acquire(norm_names[choice], true);
        if(!app_state->test_geometry->material->normal_map.texture)
        {
            kwarng("Function '%s': Failed to load normal texture. Using default! ", __FUNCTION__);
            app_state->test_geometry->material->normal_map.texture = texture_system_get_default_normal_texture();
        }
        texture_system_release(old_norm_name);
    }

    return true;
}
// TODO: Временный тестовый код: конец.


bool application_create(game* game_inst)
{
    if(app_state)
    {
        kerror("Function '%s' was called more than once.", __FUNCTION__);
        return false;
    }

    if(!game_inst)
    {
        kerror("Function '%s' require a game structure.", __FUNCTION__);
        return false;
    }

    // TODO: Сделать менеджер систем и подсистем. Решит проблему правильной инициализации и завершения.
    // Система контроля памяти.
    memory_system_config memory_cfg;
    memory_cfg.total_allocation_size = GIBIBYTES(1);
    if(!memory_system_initialize(&memory_cfg))
    {
        kerror("Failed to initialize memory system. Aborted!");
        return false;
    }
    kinfor("Memory system started.");

    // Создание контекста приложения.
    app_state = kallocate_tc(application_state, 1, MEMORY_TAG_APPLICATION);
    kzero_tc(app_state, application_state, 1);
    app_state->game_inst = game_inst;
    game_inst->application_state = app_state;

    // Создание системного линейного распределителя памяти.
    u64 systems_allocator_total_size = MEBIBYTES(64);
    app_state->systems_allocator = linear_allocator_create(systems_allocator_total_size); // TODO: Реарганизовать!

    // Система событий (должно быть инициализировано до создания окна приложения).
    event_system_initialize(&app_state->event_system_memory_requirement, null);
    app_state->event_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->event_system_memory_requirement);
    event_system_initialize(&app_state->event_system_memory_requirement, app_state->event_system_state);
    kinfor("Event system started.");

    // TODO: Отвязать от системы событий! и перенести!
    // Система ввода (должно быть инициализировано до создания окна приложения, но после системы событий - связаны).
    input_system_initialize(&app_state->input_system_memory_requirement, null);
    app_state->input_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->input_system_memory_requirement);
    input_system_initialize(&app_state->input_system_memory_requirement, app_state->input_system_state);
    kinfor("Input system started.");

    // Создание окна приложения.
    window_config window_sys_config;
    window_sys_config.title = game_inst->window_title;
    window_sys_config.width = game_inst->window_width;
    window_sys_config.height = game_inst->window_height;
    platform_window_create(&app_state->platform_window_memory_requirement, null, null);
    app_state->platform_window_state = linear_allocator_allocate(app_state->systems_allocator, app_state->platform_window_memory_requirement);
    if(!platform_window_create(&app_state->platform_window_memory_requirement, app_state->platform_window_state, &window_sys_config))
    {
        kerror("Failed to create window. Aborted!");
        return false;
    }
    kinfor("Platform window created.");

    // Установка обработчиков событий окна.
    platform_window_set_on_close_handler(app_state->platform_window_state, application_on_close);
    platform_window_set_on_resize_handler(app_state->platform_window_state, application_on_resize);
    platform_window_set_on_keyboard_key_handler(app_state->platform_window_state, application_on_keyboard_key);
    platform_window_set_on_mouse_move_handler(app_state->platform_window_state, application_on_mouse_move);
    platform_window_set_on_mouse_button_handler(app_state->platform_window_state, application_on_mouse_button);
    platform_window_set_on_mouse_wheel_handler(app_state->platform_window_state, application_on_mouse_wheel);
    platform_window_set_on_focus_handler(app_state->platform_window_state, application_on_focus);

    // Система загрузки ресурсов (должна загружаться до визуализатора, и других ресурсных систем).
    resource_system_config resource_sys_config;
    resource_sys_config.asset_base_path = "../assets";
    resource_sys_config.max_loader_count = 32;
    resource_system_initialize(&app_state->resource_system_memory_requirement, null, &resource_sys_config);
    app_state->resource_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->resource_system_memory_requirement);
    if(!resource_system_initialize(&app_state->resource_system_memory_requirement, app_state->resource_system_state, &resource_sys_config))
    {
        kerror("Failed to initialize resource system. Aborted!");
        return false;
    }
    kinfor("Resource system started.");

    shader_system_config shader_sys_config;
    shader_sys_config.max_shader_count = 1024;
    shader_sys_config.max_uniform_count = 128;
    shader_sys_config.max_global_textures = 31;
    shader_sys_config.max_instance_textures = 31;
    shader_system_initialize(&app_state->shader_system_memory_requirement, null, &shader_sys_config);
    app_state->shader_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->shader_system_memory_requirement);
    if(!shader_system_initialize(&app_state->shader_system_memory_requirement, app_state->shader_system_state, &shader_sys_config))
    {
        kerror("Failed to initialize shader system. Aborted!");
        return false;
    }
    kinfor("Shader system started.");

    // Система визуализатора графики.
    renderer_system_initialize(&app_state->renderer_system_memory_requirement, null, null);
    app_state->renderer_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->renderer_system_memory_requirement);
    if(!renderer_system_initialize(&app_state->renderer_system_memory_requirement, app_state->renderer_system_state, app_state->platform_window_state))
    {
        kerror("Failed to initialize renderer system. Aborted!");
        return false;
    }
    kinfor("Renderer system started.");

    // Система упавления текстурами.
    texture_system_config texture_sys_config;
    texture_sys_config.max_texture_count = 65536;
    texture_system_initialize(&app_state->texture_system_memory_requirement, null, &texture_sys_config);
    app_state->textute_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->texture_system_memory_requirement);
    if(!texture_system_initialize(&app_state->texture_system_memory_requirement, app_state->textute_system_state, &texture_sys_config))
    {
        kerror("Failed to initialize texture system. Aborted!");
        return false;
    }
    kinfor("Texture system started.");

    // Система управления материалами.
    material_system_config material_sys_config;
    material_sys_config.max_material_count = 4096;
    material_system_initialize(&app_state->material_system_memory_requirement, null, &material_sys_config);
    app_state->material_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->material_system_memory_requirement);
    if(!material_system_initialize(&app_state->material_system_memory_requirement, app_state->material_system_state, &material_sys_config))
    {
        kerror("Failed to initialize material system. Aborted!");
        return false;
    }
    kinfor("Material system started.");

    geometry_system_config geometry_sys_config;
    geometry_sys_config.max_geometry_count = 4096;
    geometry_system_initialize(&app_state->geometry_system_memory_requirement, null, &geometry_sys_config);
    app_state->geometry_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->geometry_system_memory_requirement);
    if(!geometry_system_initialize(&app_state->geometry_system_memory_requirement, app_state->geometry_system_state, &geometry_sys_config))
    {
        kerror("Failed to initialize geometry system. Aborted!");
        return false;
    }
    kinfor("Geometry system started.");

    // TODO: Временный тестовый код: начало.
    geometry_config g_config = geometry_system_generate_cube_config(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube", "test_material");
    geometry_generate_tangent(g_config.vertex_count, g_config.vertices, g_config.index_count, g_config.indices);
    app_state->test_geometry = geometry_system_acquire_from_config(&g_config, true);

    // TODO: Вынести в отдельную функцию очистки конфига.
    kfree_tc(g_config.vertices, vertex_3d, g_config.vertex_count, MEMORY_TAG_ARRAY);
    kfree_tc(g_config.indices, u32, g_config.index_count, MEMORY_TAG_ARRAY);

    // UI геометрия.

    geometry_config ui_config;
    ui_config.vertex_size = sizeof(vertex_2d);
    ui_config.vertex_count = 4;
    ui_config.index_size = sizeof(u32);
    ui_config.index_count = 6;
    string_ncopy(ui_config.material_name, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);
    string_ncopy(ui_config.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);

    const f32 w = 256.0f;
    const f32 h = 64.0f;
    vertex_2d uiverts[4];

    uiverts[0].position.x =  0.0f;  // 0    3
    uiverts[0].position.y =  0.0f;  //
    uiverts[0].texcoord.x =  0.0f;  //
    uiverts[0].texcoord.y =  0.0f;  // 2    1

    uiverts[1].position.x =  w;
    uiverts[1].position.y =  h;
    uiverts[1].texcoord.x =  1.0f;
    uiverts[1].texcoord.y =  1.0f;

    uiverts[2].position.x =  0.0f;
    uiverts[2].position.y =  h;
    uiverts[2].texcoord.x =  0.0f;
    uiverts[2].texcoord.y =  1.0f;

    uiverts[3].position.x =  w;
    uiverts[3].position.y =  0.0f;
    uiverts[3].texcoord.x =  1.0f;
    uiverts[3].texcoord.y =  0.0f;
    ui_config.vertices = uiverts;

    u32 uiindices[6] = {2, 1, 0, 3, 0, 1};
    ui_config.indices = uiindices;

    app_state->test_ui_geometry = geometry_system_acquire_from_config(&ui_config, true);

    // app_state->test_geometry = geometry_system_get_default();
    event_register(EVENT_CODE_DEBUG_0, null, event_on_debug_event);
    // TODO: Временный тестовый код: конец.

    // Выделение памяти под состояние игры.
    game_inst->state = kallocate(game_inst->state_memory_requirement, MEMORY_TAG_GAME);

    // Инициализация приложения пользователя (игры, 3d приложения).
    if(!game_inst->initialize(game_inst))
    {
        kerror("Failed to initialize game. Aborted!");
        return false;
    }
    kinfor("Game initialized.");

    // Принудительное обновление размера окна.
    game_inst->on_resize(game_inst, game_inst->window_width, game_inst->window_height);

    return true;
}

bool application_run()
{
    if(!app_state)
    {
        kerror("Application context was not created. Call 'application_create' first.");
        return false;
    }

    app_state->is_running = true;

    clock_start(&app_state->clock);
    clock_update(&app_state->clock);
    app_state->last_time = app_state->clock.elapsed;

    f64 running_time     = 0;
    u16 frame_count      = 0;
    f64 frame_limit_time = 1.0f / 240; // TODO: сделать настраиваемым!

    while(app_state->is_running)
    {
        if(!platform_window_dispatch(app_state->platform_window_state))
        {
            app_state->is_running = false;
        }

        if(!app_state->is_suspended)
        {
            // Обновляем таймер и получаем дельту!
            clock_update(&app_state->clock);
            f64 current_time = app_state->clock.elapsed;
            f64 delta = current_time - app_state->last_time;
            f64 frame_start_time = platform_time_absolute();
            
            if(!app_state->game_inst->update(app_state->game_inst, (f32)delta))
            {
                kerror("Game update failed, shutting down!");
                app_state->is_running = false;
                break;
            }

            // Пользовательский рендер.
            if(!app_state->game_inst->render(app_state->game_inst, (f32)delta))
            {
                kerror("Game render failed, shutting down!");
                app_state->is_running = false;
                break;
            }

            // TODO: Провести рефактор render_packet.
            render_packet packet;
            packet.delta_time = (f32)delta;

            // TODO: Временный тестовый код: начало.
            geometry_render_data test_render;
            test_render.geometry = app_state->test_geometry;
            // test_render.model = mat4_identity();
            static f32 angle = 0;
            angle += (0.5f * delta);
            quat rotation = quat_from_axis_angle(vec3_up(), angle, true);
            test_render.model = quat_to_mat4(rotation);

            packet.geometry_count = 1;
            packet.geometries = &test_render;

            geometry_render_data test_ui_render;
            test_ui_render.geometry = app_state->test_ui_geometry;
            test_ui_render.model = mat4_translation(vec3_zero());

            packet.ui_geometry_count = 1;
            packet.ui_geometries = &test_ui_render;
            // TODO: Временный тестовый код: конец.

            if(!renderer_draw_frame(&packet))
            {
                kerror("Renderer failed draw frame, shutting down!");
                app_state->is_running = false;
                break;
            }

            // Расчет времени кадра.
            f64 frame_end_time = platform_time_absolute();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_secounds = frame_limit_time - frame_elapsed_time;

            if(remaining_secounds > 0)
            {
                u64 remaining_ms = remaining_secounds * 1000;

                // Если время еще не вышло, то возвращаем управление операционной системе!
                // TODO: Вынести лимитер кадров в настройки приложения.
                bool frame_limit_on = true;
                if(remaining_ms > 0 && frame_limit_on)
                {
                    platform_thread_sleep(remaining_ms - 1);
                }

                frame_count++;

            }

            // NOTE: Устройства ввода последнее что должно обновляться в кадре!
            input_system_update(delta);

            app_state->last_time = current_time;
        }
    }

    kfree(app_state->game_inst->state, app_state->game_inst->state_memory_requirement, MEMORY_TAG_GAME);
    kinfor("Game stopped.");

    // NOTE: Что бы исключить нежелательные эффекты!
    input_system_shutdown();
    kinfor("Input system stopped.");

    geometry_system_shutdown();
    kinfor("Geometry system stopped.");

    material_system_shutdown();
    kinfor("Material system stopped.");

    texture_system_shutdown();
    kinfor("Texture system stopped.");

    shader_system_shutdown(); // Остановка раньше, потому как использует функции рендерера!
    kinfor("Shader system stopped.");

    renderer_system_shutdown();
    kinfor("Renderer system stopped.");

    resource_system_shutdown();
    kinfor("Resource system stopped.");

    platform_window_destroy(app_state->platform_window_state);
    kinfor("Platform window destroyed.");

    event_system_shutdown();
    kinfor("Event system stopped.");

    linear_allocator_free_all(app_state->systems_allocator);
    linear_allocator_destroy(app_state->systems_allocator);
    app_state->systems_allocator = null;
    app_state->game_inst->application_state = null;

    kfree_tc(app_state, application_state, 1, MEMORY_TAG_APPLICATION);
    app_state = null;

    memory_system_shutdown();
    kinfor("Memory system stopped.");

    return true;
}

//////////////////////////////// EVENTS /////////////////////////////////

void application_on_focus(bool focused)
{
    // Stub!
}

void application_on_mouse_wheel(i32 zdelta)
{
    input_update_mouse_wheel(zdelta);    
}

void application_on_mouse_button(u32 button, bool pressed)
{
    input_update_mouse_button(button, pressed);
}

void application_on_mouse_move(i32 x, i32 y)
{
    input_update_mouse_move(x, y);
}

void application_on_keyboard_key(u32 keycode, bool pressed)
{
    if(keycode == KEY_Q && pressed) app_state->is_running = false;
    if(keycode == KEY_I && pressed)
    {
        const char* meminfo = memory_system_usage_str();
        kinfor(meminfo);
        string_free(meminfo);
    }

    input_update_keyboard_key(keycode, pressed);
}

void application_on_resize(i32 width, i32 height)
{
    // Обновление размеров в приложения.
    app_state->game_inst->on_resize(app_state->game_inst, width, height);

    // Обновление размеров в визуализаторе.
    renderer_on_resize(width, height);

    // Создание события на обновление размеров.
    event_context context = { .i32[0] = width, .i32[1] = height };
    event_send(EVENT_CODE_APPLICATION_RESIZE, null, &context);
}

void application_on_close()
{
    app_state->is_running = false;
    event_send(EVENT_CODE_APPLICATION_QUIT, null, null);
}
