#version 450

// Должно соответствовать vertex_2d. 
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
    mat4 view;
} global_ubo;

layout(push_constant) uniform push_constants {
    mat4 model;
} u_push_constants;

layout(location = 1) out struct dto {
    vec2 tex_coord;
} out_dto;

void main()
{
    // NOTE: Намеренно переворачивает текстурную координату Y. Это, вместе с перевернутой ортогональной матрицей,
    // помещает [0, 0] в верхний левый угол вместо нижнего левого угла и корректирует текстурные координаты для
    // отображения в правильном направлении.
    out_dto.tex_coord = vec2(in_texcoord.x, 1.0 - in_texcoord.y);
    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 0.0, 1.0);
}

