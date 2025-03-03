#pragma once

#include <defines.h>
#include <math/math_types.h>

KAPI transform transform_create();

KAPI transform transform_from_position(vec3 position);

KAPI transform transform_from_rotation(quat rotation);

KAPI transform transform_from_position_rotation(vec3 position, quat rotation);

KAPI transform transform_from_position_rotation_scale(vec3 position, quat rotation, vec3 scale);

KAPI transform* transform_get_parent(transform* t);

KAPI void transform_set_parent(transform* t, transform* parent);

KAPI vec3 transform_get_position(const transform* t);

KAPI void transform_set_position(transform* t, vec3 position);

KAPI void transform_translate(transform* t, vec3 translation);

KAPI quat transform_get_rotation(const transform* t);

KAPI void transform_set_rotation(transform* t, quat rotation);

KAPI void transform_rotate(transform* t, quat rotation);

KAPI vec3 transform_get_scale(const transform* t);

KAPI void transform_set_scale(transform* t, vec3 scale);

KAPI void transform_scale(transform* t, vec3 scale);

KAPI void transform_set_position_rotation(transform* t, vec3 position, quat rotation);

KAPI void transform_set_position_rotation_scale(transform* t, vec3 position, quat rotation, vec3 scale);

KAPI void transform_translate_rotate(transform* t, vec3 translation, quat rotation);

KAPI mat4 transform_get_local(transform* t);

KAPI mat4 transform_get_world(transform* t);
