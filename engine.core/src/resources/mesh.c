// Собствнные подключения.
#include "resources/mesh.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "kstring.h"
#include "systems/job_system.h"
#include "systems/resource_system.h"
#include "systems/geometry_system.h"

typedef struct mesh_load_params {
    const char* resource_name;
    mesh* out_mesh;
    resource mesh_resource;
} mesh_load_params;

void mesh_load_job_success(void* params)
{
    mesh_load_params* mesh_params = params;

    geometry_config* configs = mesh_params->mesh_resource.data;
    mesh_params->out_mesh->geometry_count = mesh_params->mesh_resource.data_size;
    mesh_params->out_mesh->geometries = kallocate_tc(struct geometry*, mesh_params->out_mesh->geometry_count, MEMORY_TAG_ARRAY);

    for(u32 i = 0; i < mesh_params->out_mesh->geometry_count; ++i)
    {
        mesh_params->out_mesh->geometries[i] = geometry_system_acquire_from_config(&configs[i], true);
    }

    mesh_params->out_mesh->generation++;

    ktrace("Successfully loaded mesh '%s'.", mesh_params->resource_name);

    resource_system_unload(&mesh_params->mesh_resource);
    if(mesh_params->resource_name)
    {
        string_free(mesh_params->resource_name);
    }
}

void mesh_load_job_fail(void* params)
{
    mesh_load_params* mesh_params = params;

    kerror("Failed to load mesh '%s'.", mesh_params->resource_name);

    resource_system_unload(&mesh_params->mesh_resource);
    if(mesh_params->resource_name)
    {
        string_free(mesh_params->resource_name);
    }
}

bool mesh_load_job(void* params, void* result_data)
{
    mesh_load_params* load_params = params;

    bool result = resource_system_load(load_params->resource_name, RESOURCE_TYPE_MESH, null, &load_params->mesh_resource);

    // NOTE: Теже параметры используются и для результата.
    kcopy_tc(result_data, load_params, struct mesh_load_params, 1);

    return result;
}

bool mesh_load_from_resource(const char* resource_name, mesh* out_mesh)
{
    out_mesh->generation = INVALID_ID_U8;

    mesh_load_params params;
    params.resource_name = string_duplicate(resource_name);
    params.out_mesh = out_mesh;
    params.mesh_resource = (resource){};

    job job = job_create_default(mesh_load_job, mesh_load_job_success, mesh_load_job_fail, &params, sizeof(mesh_load_params), sizeof(mesh_load_params));
    job_system_submit(&job);

    return true;
}

void mesh_unload(mesh* mesh)
{
    if(!mesh)
    {
        kerror("Function '%s' requires a valid pointer to mesh.", __FUNCTION__);
        return;
    }

    for(u32 i = 0; i < mesh->geometry_count; ++i)
    {
        geometry_system_release(mesh->geometries[i]);
    }

    kfree(mesh->geometries, MEMORY_TAG_ARRAY);
    mesh->geometries = null;
    mesh->geometry_count = 0;
    mesh->generation = INVALID_ID_U8;
}
