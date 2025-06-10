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
#include "systems/camera_system.h"
#include "systems/render_view_system.h"
#include "systems/job_system.h"

// TODO: Временный тестовый код: начало.
#include "kstring.h"
#include "math/kmath.h"
#include "math/transform.h"
#include "resources/mesh.h"
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

    u64 job_system_memory_requirement;
    void* job_system_state;

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

    u64 render_view_system_memory_requirement;
    void* render_view_system_state;

    u64 texture_system_memory_requirement;
    void* textute_system_state;

    u64 material_system_memory_requirement;
    void* material_system_state;

    u64 geometry_system_memory_requirement;
    void* geometry_system_state;

    u64 camera_system_memory_requirement;
    void* camera_system_state;

    // TODO: Временный тестовый код: начало.
    skybox sb;
    texture_map* sb_maps[1];

    mesh world_meshes[10];
    mesh* car_mesh;
    mesh* sponza_mesh;
    bool models_loaded;

    mesh ui_meshes[10];
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
    if(code == EVENT_CODE_DEBUG_0)
    {
        const char* names[3] = { "cobblestone", "paving", "paving2" };

        static i8 choice = 1;
        const char* old_name = names[choice];

        choice++;
        choice %= 3;

        geometry* g = app_state->world_meshes[0].geometries[0];

        if(g)
        {
            // TODO: Текстура еще не загружена, а возникают артефакты (загрузка текстуры по умолчанию).
            g->material = material_system_acquire(names[choice]);

            if(!g->material)
            {
                kwarng("Function '%s': Failed to load material %s. Using default! ", __FUNCTION__, names[choice]);
                g->material = material_system_get_default();
            }

            material_system_release(old_name);
        }
    }
    else if(code == EVENT_CODE_DEBUG_1)
    {
        if(!app_state->models_loaded)
        {
            kdebug("Loading models...");
            app_state->models_loaded = true;

            if(!mesh_load_from_resource("falcon", app_state->car_mesh))
            {
                kerror("Failed to load falcon mesh!");
            }

            if(!mesh_load_from_resource("sponza", app_state->sponza_mesh))
            {
                kerror("Failed to load sponza mesh!");
            }
        }
    }
    else
    {
        return false;
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
    memory_cfg.total_allocation_size = 1 GiB;
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

    // TODO: Временно для отладки!
    app_state->models_loaded = false;

    // Создание системного линейного распределителя памяти.
    u64 systems_allocator_total_size = 64 MiB;
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

    bool renderer_multithreaded = renderer_is_multithreaded();

    // NOTE: Минус один, т.к. главный поток уже запущен и используется.
    i32 thread_count = platform_thread_get_processor_count() - 1;
    if(thread_count < 1)
    {
        kfatal("Error: Platform reported processor count (minus one for main thread) as %i. Need at least one additional thread for the job system.", thread_count);
        return false;
    }

    const i32 max_thread_count = 15; // TODO: Сделать настраиваемым.
    if(thread_count > max_thread_count)
    {
        ktrace("Available threads on the system is %i, but will be capped at %i.", thread_count, max_thread_count);
        thread_count = max_thread_count;
    }
    kinfor("Available threads for job system: %i", thread_count);

    // Назначение всем очередям, выполнять обычные задания.
    u32 job_thread_types[15];
    for(u32 i = 0; i < 15; ++i)
    {
        job_thread_types[i] = JOB_TYPE_GENERAL;
    }

    if(max_thread_count == 1 || !renderer_multithreaded)
    {
        job_thread_types[0] |= (JOB_TYPE_GPU_RESOURCE | JOB_TYPE_RESOURCE_LOAD);
    }
    else if(max_thread_count == 2)
    {
        job_thread_types[0] |= JOB_TYPE_GPU_RESOURCE;
        job_thread_types[1] |= JOB_TYPE_RESOURCE_LOAD;
    }
    else
    {
        job_thread_types[0] |= JOB_TYPE_GPU_RESOURCE;
        job_thread_types[1] |= JOB_TYPE_RESOURCE_LOAD;
    }

    job_system_config job_sys_config;
    job_sys_config.max_job_thread_count = thread_count;
    job_sys_config.type_masks = job_thread_types;

    job_system_initialize(&app_state->job_system_memory_requirement, null, &job_sys_config);
    app_state->job_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->job_system_memory_requirement);
    if(!job_system_initialize(&app_state->job_system_memory_requirement, app_state->job_system_state, &job_sys_config))
    {
        kerror("Failed to initialize job system. Aborted!");
        return false;
    }
    kinfor("Job system started.");

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

    camera_system_config camera_sys_config;
    camera_sys_config.max_camera_count = 61;
    camera_system_initialize(&app_state->camera_system_memory_requirement, null, &camera_sys_config);
    app_state->camera_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->camera_system_memory_requirement);
    if(!camera_system_initialize(&app_state->camera_system_memory_requirement, app_state->camera_system_state, &camera_sys_config))
    {
        kerror("Failed to initialize camera system. Aborted!");
        return false;
    }
    kinfor("Camera system started.");

    render_view_system_config render_view_sys_config;
    render_view_sys_config.max_view_count = 251;
    render_view_system_initialize(&app_state->render_view_system_memory_requirement, null, &render_view_sys_config);
    app_state->render_view_system_state = linear_allocator_allocate(app_state->systems_allocator, app_state->render_view_system_memory_requirement);
    if(!render_view_system_initialize(&app_state->renderer_system_memory_requirement, app_state->render_view_system_state, &render_view_sys_config))
    {
        kerror("Failed to initialize render view system. Aborted!");
        return false;
    }
    kinfor("Render view system started.");

    // Загрузка проходчиков визуализаторов.
    render_view_config skybox_config = {};
    skybox_config.type = RENDERER_VIEW_KNOWN_TYPE_SKYBOX;
    skybox_config.width = 0;
    skybox_config.height = 0;
    skybox_config.name = "skybox";
    skybox_config.pass_count = 1;
    render_view_pass_config skybox_passes[1];
    skybox_passes[0].name = "Builtin.RenderpassSkybox";
    skybox_config.passes = skybox_passes;
    skybox_config.view_matrix_source = RENDER_VIEW_MATRIX_SOURCE_SCENE_CAMERA;
    if(!render_view_system_create(&skybox_config))
    {
        kerror("Failed to create skybox view. Aborted!");
        return false;
    }

    render_view_config opaque_world_config = {};
    opaque_world_config.type = RENDERER_VIEW_KNOWN_TYPE_WORLD;
    opaque_world_config.width = 0;
    opaque_world_config.height = 0;
    opaque_world_config.name = "world_opaque";
    opaque_world_config.pass_count = 1;
    render_view_pass_config passes[1];
    passes[0].name = "Builtin.RenderpassWorld";
    opaque_world_config.passes = passes;
    opaque_world_config.view_matrix_source = RENDER_VIEW_MATRIX_SOURCE_SCENE_CAMERA;
    if(!render_view_system_create(&opaque_world_config))
    {
        kerror("Failed to create view. Aborted!");
        return false;
    }

    render_view_config ui_view_config = {};
    ui_view_config.type = RENDERER_VIEW_KNOWN_TYPE_UI;
    ui_view_config.width = 0;
    ui_view_config.height = 0;
    ui_view_config.name = "ui";
    ui_view_config.pass_count = 1;
    render_view_pass_config ui_passes[1];
    ui_passes[0].name = "Builtin.RenderpassUI";
    ui_view_config.passes = ui_passes;
    ui_view_config.view_matrix_source = RENDER_VIEW_MATRIX_SOURCE_SCENE_CAMERA;
    if(!render_view_system_create(&ui_view_config))
    {
        kerror("Failed to create view. Aborted!");
        return false;
    }

    // TODO: Временный тестовый код: начало.

    // Skybox.
    texture_map* cube_map = &app_state->sb.cubemap;
    cube_map->filter_magnify = cube_map->filter_minify = TEXTURE_FILTER_LINEAR;
    cube_map->repeat_u = cube_map->repeat_v = cube_map->repeat_w = TEXTURE_REPEAT_CLAMP_TO_EDGE;
    cube_map->use = TEXTURE_USE_MAP_CUBEMAP;
    if(!renderer_texture_map_acquire_resources(cube_map))
    {
        kerror("Unable to acquire resources for cube map texture.");
        return false;
    }
    cube_map->texture = texture_system_acquire_cube("skybox", true);

    geometry_config skybox_cube_config = geometry_system_generate_cube_config(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "skybox_cube", null);
    skybox_cube_config.material_name[0] = '\0';
    app_state->sb.g = geometry_system_acquire_from_config(&skybox_cube_config, true);
    app_state->sb.render_frame_number = INVALID_ID_U64;
    shader* skybox_shader = shader_system_get(BUILTIN_SHADER_NAME_SKYBOX);
    app_state->sb_maps[0] = cube_map; 
    if(!renderer_shader_acquire_instance_resources(skybox_shader, app_state->sb_maps, &app_state->sb.instance_id))
    {
        kerror("Unable to acquire shader resources for skybox texture.");
        return false;
    }
    geometry_system_config_dispose(&skybox_cube_config);

    // Обнуление сеток.
    for(u32 i = 0; i < 10; ++i)
    {
        app_state->world_meshes[i].generation = INVALID_ID_U8;
        app_state->ui_meshes[i].generation = INVALID_ID_U8;
    }

    // World.
    u8 mesh_count = 0;

    // Тестовый куб.
    mesh* cube_mesh = &app_state->world_meshes[mesh_count];
    cube_mesh->generation = 0;
    cube_mesh->geometry_count = 1;
    cube_mesh->geometries = kallocate_tc(geometry*, cube_mesh->geometry_count, MEMORY_TAG_ARRAY);
    geometry_config g_config = geometry_system_generate_cube_config(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "test_cube", "test_material");
    cube_mesh->geometries[0] = geometry_system_acquire_from_config(&g_config, true);
    cube_mesh->transform = transform_create();
    geometry_system_config_dispose(&g_config);
    mesh_count++;

    // Машина.
    app_state->car_mesh = &app_state->world_meshes[mesh_count];
    app_state->car_mesh->transform = transform_from_position((vec3){{20.0f, 0.0f, 0.0f}});
    mesh_count++;

    // Спонза.
    app_state->sponza_mesh = &app_state->world_meshes[mesh_count];
    app_state->sponza_mesh->transform = transform_from_position((vec3){{0.0f, 0.0f, 0.0f}});
    mesh_count++;

    // UI.
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

    mesh* ui_mesh = &app_state->ui_meshes[0];
    ui_mesh->generation = 0;
    ui_mesh->geometry_count = 1;
    ui_mesh->geometries = kallocate_tc(geometry*, ui_mesh->geometry_count, MEMORY_TAG_ARRAY);
    ui_mesh->geometries[0] = geometry_system_acquire_from_config(&ui_config, true);
    ui_mesh->transform = transform_create();

    event_register(EVENT_CODE_DEBUG_0, null, event_on_debug_event);
    event_register(EVENT_CODE_DEBUG_1, null, event_on_debug_event);
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
    renderer_on_resize(game_inst->window_width, game_inst->window_height);

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

    // f64 running_time     = 0;
    // u16 frame_count      = 0;
    f64 frame_limit_time = 1.0f / 120; // TODO: сделать настраиваемым!

    // TODO: Временный тестовый код: начало.
    render_view_packet views[3];
    // TODO: Временный тестовый код: конец.

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

            // Update the job system.
            job_system_update();

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

            // TODO: Временный тестовый код: начало.
            quat rotation = quat_from_axis_angle(vec3_up(), 0.5f * delta, false);
            transform_rotate(&app_state->world_meshes[0].transform, rotation);

            // TODO: Реарганизовать.
            render_packet packet = {};
            packet.delta_time = (f32)delta;
            packet.view_count = 3;
            kzero_tc(views, render_view_packet, packet.view_count);
            packet.views = views;

            // Skybox.
            skybox_packet_data skybox_data = {};
            skybox_data.sb = &app_state->sb;
            if(!render_view_system_build_packet(render_view_system_get("skybox"), &skybox_data, &packet.views[0]))
            {
                kerror("Failed to build packet for view 'skybox'.");
                return false;
            }

            // World.
            mesh_packet_data world_mesh_data = {};

            u32 mesh_count = 0;
            mesh* meshes[10];
            // TODO: Изменяемый массив.
            for(u32 i = 0; i < 10; ++i)
            {
                if(app_state->world_meshes[i].generation != INVALID_ID_U8)
                {
                    meshes[mesh_count] = &app_state->world_meshes[i];
                    mesh_count++;
                }
            }

            world_mesh_data.mesh_count = mesh_count;
            world_mesh_data.meshes = meshes;

            if(!render_view_system_build_packet(render_view_system_get("world_opaque"), &world_mesh_data, &packet.views[1]))
            {
                kerror("Failed to build packet for view 'world_opaque'.");
                return false;
            }

            // UI.
            mesh_packet_data ui_mesh_data = {};

            u32 ui_mesh_count = 0;
            mesh* ui_meshes[10];

            // TODO: Изменяемый массив.
            for(u32 i = 0; i < 10; ++i)
            {
                if(app_state->ui_meshes[i].generation != INVALID_ID_U8)
                {
                    ui_meshes[ui_mesh_count] = &app_state->ui_meshes[i];
                    ui_mesh_count++;
                }
            }

            ui_mesh_data.mesh_count = ui_mesh_count;
            ui_mesh_data.meshes = ui_meshes;

            if(!render_view_system_build_packet(render_view_system_get("ui"), &ui_mesh_data, &packet.views[2]))
            {
                kerror("Failed to build packet for view 'ui'.");
                return false;
            }
            // TODO: Временный тестовый код: конец.

            if(!renderer_draw_frame(&packet))
            {
                kerror("Renderer failed draw frame, shutting down!");
                app_state->is_running = false;
                break;
            }

            // TODO: Временный тестовый код: начало.
            // Очистка данных пакетов.
            for(u32 i = 0; i < packet.view_count; ++i)
            {
                packet.views[i].view->on_destroy_packet(packet.views[i].view, &packet.views[i]);
            }
            // TODO: Временный тестовый код: конец.

            // Расчет времени кадра.
            f64 frame_end_time = platform_time_absolute();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            // running_time += frame_elapsed_time;
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

                // frame_count++;
            }

            // TODO: Перенести перед отрисовкой кадра
            // NOTE: Устройства ввода последнее что должно обновляться в кадре!
            input_system_update(delta);

            app_state->last_time = current_time;
        }
    }

    kfree(app_state->game_inst->state, MEMORY_TAG_GAME);
    kinfor("Game stopped.");

    // TODO: Временный тестовый код: начало.
    for(u32 i = 0; i < 10; ++i)
    {
        if(app_state->world_meshes[i].generation != INVALID_ID_U8)
        {
            kdebug("World mesh[%u]: get geometries is %u", i, app_state->world_meshes[i].geometry_count);
            kfree(app_state->world_meshes[i].geometries, MEMORY_TAG_ARRAY);
        }
    }

    for(u32 i = 0; i < 10; ++i)
    {
        if(app_state->ui_meshes[i].generation != INVALID_ID_U8)
        {
            kdebug("UI mesh[%u]: get geometries is %u", i, app_state->ui_meshes[i].geometry_count);
            kfree(app_state->ui_meshes[i].geometries, MEMORY_TAG_ARRAY);
        }
    }

    // Удаление данных скайбокса.
    shader* skybox_shader = shader_system_get(BUILTIN_SHADER_NAME_SKYBOX);
    renderer_shader_release_instance_resources(skybox_shader, app_state->sb.instance_id); // TODO: Размер ubo_stride не проверяется, а потому error!
    renderer_texture_map_release_resources(&app_state->sb.cubemap);
    // TODO: Временный тестовый код: конецы.

    // NOTE: Что бы исключить нежелательные эффекты управление остановить первым!
    input_system_shutdown();
    kinfor("Input system stopped.");

    render_view_system_shutdown();
    kinfor("Render view system stopped.");

    camera_system_shutdown();
    kinfor("Camera system stopped.");

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

    job_system_shutdown();
    kinfor("Job system stopped.");

    platform_window_destroy(app_state->platform_window_state);
    kinfor("Platform window destroyed.");

    event_system_shutdown();
    kinfor("Event system stopped.");

    linear_allocator_free_all(app_state->systems_allocator);
    linear_allocator_destroy(app_state->systems_allocator);
    app_state->systems_allocator = null;
    app_state->game_inst->application_state = null;

    kfree(app_state, MEMORY_TAG_APPLICATION);
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
