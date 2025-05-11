#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>

bool render_view_skybox_on_create(render_view* self);

void render_view_skybox_on_destroy(render_view* self);

void render_view_skybox_on_resize(render_view* self, u32 width, u32 height);

bool render_view_skybox_on_build_packet(render_view* self, void* data, render_view_packet* out_packet);

bool render_view_skybox_on_render(render_view* self, const render_view_packet* packet, u64 frame_number, u64 render_target_index);


