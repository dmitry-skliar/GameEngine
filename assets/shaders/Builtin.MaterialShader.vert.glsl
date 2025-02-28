#version 450

// Должно соответствовать vertex_3d.
layout(location = 0) in vec3 in_position; // Локальные координаты вершин.
layout(location = 1) in vec3 in_normal;   // Локальные нормали вершин.
layout(location = 2) in vec2 in_texcoord; // Текстурный координаты.
layout(location = 3) in vec4 in_color;
layout(location = 4) in vec4 in_tangent;

// Порядок должен соответствовать глобальным uniform-переменным в shadercfg.
layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;    // Проекционая матрица.
    mat4 view;          // Матрица вида (по сути матрица камера).
    vec4 ambient_color; // Цвет поверхности не поподающий под приямой источник света.
    vec3 view_position; // Положение камеры.
} global_ubo;

layout(push_constant) uniform push_constants {
    // Гарантируется всего 128 байт.
    mat4 model;         // Мировая матрица (масштаб, вращение и положение объекта в мире).
} u_push_constants;

// Передаваемые данные в далее по конвейеру (data transfer object).
layout(location = 1) out struct dto {
    vec4 ambient;       // Цвет неосвещенной поверхности.
    vec2 tex_coord;     // Текстурные координаты.
    vec3 normal;        // Вектор нормали (трансформированые).
    vec3 view_position;
    vec3 frag_position;
    vec4 color;
    vec4 tangent;
} out_dto;

// В вершинном щейдере main применяется к каждой вершине.
// NOTE: vec4, где дополнительное значение w == 1 - для точки, w == 0 - для вектора.
void main()
{
    out_dto.tex_coord = in_texcoord;
    out_dto.color = in_color;
    out_dto.frag_position = vec3(u_push_constants.model * vec4(in_position, 1.0)); // Позиция в мировом пространстве.

    mat3 m3_model = mat3(u_push_constants.model);
    out_dto.normal = m3_model * in_normal;
    out_dto.tangent = vec4(normalize(m3_model * in_tangent.xyz), in_tangent.w);
    out_dto.ambient = global_ubo.ambient_color;
    out_dto.view_position = global_ubo.view_position;
    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 1.0);
}
