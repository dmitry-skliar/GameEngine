// Собственные подключения.
#include "systems/camera_system.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "containers/hashtable.h"
#include "renderer/camera.h"

typedef struct camera_lookup {
    u16 id;
    u16 reference_count;
    camera c;
} camera_lookup;

typedef struct camera_system_state {
    // Конфигурация системы камер.
    camera_system_config config;
    // Камера по умолчанию.
    camera default_camera;
    // Массив камер.
    camera_lookup* cameras;
    // Хэш таблица инентификаторов камер.
    hashtable* lookup;
} camera_system_state;

static camera_system_state* state_ptr = null;

bool system_status_valid(const char* func_name)
{
    if(!state_ptr)
    {
        if(func_name)
        {
            kerror(
                "Function '%s' requires the camera system to be initialized. Call 'camera_system_initialize' first.",
                func_name
            );
        }
        return false;
    }
    return true;
}

bool camera_system_initialize(u64* memory_requirement, void* memory, camera_system_config* config)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once!", __FUNCTION__);
        return false;
    }

    if(!memory_requirement || !config)
    {
        kerror("Function '%s' requires a valid pointers to memory_requirement and config.", __FUNCTION__);
        return false;
    }

    if(!config->max_camera_count)
    {
        kerror("Function '%s': config.max_camera_count must be greater then zero.", __FUNCTION__);
        return false;
    }

    u64 state_requirement = sizeof(camera_system_state);
    u64 array_requirement = sizeof(camera_lookup) * config->max_camera_count;
    u64 hashtable_requirement = 0;
    hashtable_config hcfg = { sizeof(u16), config->max_camera_count };
    hashtable_create(&hashtable_requirement, null, &hcfg, null);
    *memory_requirement = state_requirement + array_requirement + hashtable_requirement;

    if(!memory)
    {
        return true;
    }

    // Обнуление заголовка системы камер.
    kzero_tc(memory, camera_system_state, 1);
    state_ptr = memory;

    // Запись данных конфигурации системы.
    state_ptr->config = *config;

    // Получение и запись указателя на блок камер.
    void* array_block = POINTER_GET_OFFSET(state_ptr, state_requirement);
    state_ptr->cameras = array_block;

    // Получение и запись указателя на хэш-таблицу.
    void* hashtable_block = POINTER_GET_OFFSET(array_block, array_requirement);
    if(!hashtable_create(&hashtable_requirement, hashtable_block, &hcfg, &state_ptr->lookup))
    {
        kerror("Function '%s': Failed to create hashtable of ids to camers.", __FUNCTION__);
        return false;
    }

    // Отмечает все камеры как недействительные.
    for(u32 i = 0; i < state_ptr->config.max_camera_count; ++i)
    {
        state_ptr->cameras[i].id = INVALID_ID_U16;
        state_ptr->cameras[i].reference_count = 0;
    }

    // Создание материала по умолчанию.
    state_ptr->default_camera = camera_create();

    return true;
}

void camera_system_shutdown()
{
    if(!system_status_valid(__FUNCTION__)) return;
    hashtable_destroy(state_ptr->lookup);
    state_ptr = null;
}

camera* camera_system_acquire(const char* name)
{
    if(!system_status_valid(__FUNCTION__)) return null;

    if(string_equali(name, DEFAULT_CAMERA_NAME))
    {
        return &state_ptr->default_camera;
    }

    u16 id = INVALID_ID_U16;
    if(!hashtable_get(state_ptr->lookup, name, &id) || id == INVALID_ID_U16)
    {
        // Поиск свободного слота памяти.
        for(u16 i = 0; i < state_ptr->config.max_camera_count; ++i)
        {
            if(i == INVALID_ID_U16)
            {
                id = i;
                break;
            }
        }

        if(id == INVALID_ID_U16)
        {
            kerror("Function '%s' failed to acquire new slot. Adjust camera system config to allow more.", __FUNCTION__);
            return null;
        }

        ktrace("Function '%s': Creating new camera named '%s'.", __FUNCTION__, name);
        state_ptr->cameras[id].id = id;
        state_ptr->cameras[id].c = camera_create();

        // Обновление записи в таблице.
        if(!hashtable_set(state_ptr->lookup, name, &id, true))
        {
            kerror("Function '%s' Failed to update camera id.", __FUNCTION__);
            state_ptr->cameras[id].id = INVALID_ID_U16;
            state_ptr->cameras[id].reference_count = 0;
            return null;
        }
    }

    state_ptr->cameras[id].reference_count++;

    return &state_ptr->cameras[id].c;
}

void camera_system_release(const char* name)
{
    if(!system_status_valid(__FUNCTION__)) return;

    if(string_equali(name, DEFAULT_CAMERA_NAME))
    {
        ktrace("Function '%s': Cannot release defautl camera. Nothing was done.", __FUNCTION__);
        return;
    }

    u16 id = INVALID_ID_U16;
    if(!hashtable_get(state_ptr->lookup, name, &id) || id == INVALID_ID_U16)
    {
        kwarng("Function '%s': Tried to release non-existent camera '%s'.", __FUNCTION__, name);
        return;
    }

    state_ptr->cameras[id].reference_count--;

    if(state_ptr->cameras[id].reference_count < 1)
    {
        camera_reset(&state_ptr->cameras[id].c);
        state_ptr->cameras[id].id = INVALID_ID_U16;

        // Обновление записи в таблице.
        id = INVALID_ID_U16;
        if(!hashtable_set(state_ptr->lookup, name, &id, true))
        {
            kerror("Function '%s' Failed to update camera id.", __FUNCTION__);
        }
    }
}

camera* camera_system_get_default()
{
    if(!system_status_valid(__FUNCTION__)) return null;
    return &state_ptr->default_camera;
}
