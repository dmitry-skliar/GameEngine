#pragma once

#include <resources/resource_types.h>

bool mesh_load_from_resource(const char* resource_name, mesh* out_mesh);

void mesh_unload(mesh* mesh);
