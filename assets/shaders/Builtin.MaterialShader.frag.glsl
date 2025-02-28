#version 450

// Должно соответствовать subpass.pColorAttachments = ..., 0 индексу массива.
layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform local_uniform_object {
    vec4 diffuse_color; // Цветовой фильтр материала? (заданое в материале RGBA).
} object_ubo;

// Текстуры.
const int  SAMP_DIFFUSE = 0; // Обычная текстура.
layout(set = 1, binding = 1) uniform sampler2D samplers[1];

// Принимаемые данные переданные по конвейеру от другого шейдера (data transfer object).
layout(location = 1) in struct dto {
    vec4 ambient;       // Цвет неосвещенной поверхности.
    vec2 tex_coord;     // Текстурные координаты.
    vec3 normal;        // Вектор нормали (трансформированые).
} in_dto;

struct directional_light {
    vec3 direction;    // Вектор направления.
    vec4 color;        // RGBA.
};

// TODO: Задавать из приложения.
directional_light source_light = {
    vec3(-0.57735, -0.57735, -0.57735),
    vec4(0.8, 0.8, 0.8, 1.0)
};

// Функиця расчета освещения для пикселя по заданой нормали.
vec4 calculate_directional_light(directional_light light, vec3 normal);

// В фрагментном шейдере main применяется к каждому пикселю.
void main()
{
    out_color = calculate_directional_light(source_light, in_dto.normal);
}

vec4 calculate_directional_light(directional_light light, vec3 normal)
{
    // Получаем степень освещености, но только в положительном направлении векторов.
    float diffuse_factor = max(dot(normal, -light.direction), 0.0);

    // Получаем цвет пикселя текстуры.
    vec4 diff_samp = texture(samplers[SAMP_DIFFUSE], in_dto.tex_coord);
    vec4 ambient = vec4(vec3(in_dto.ambient * object_ubo.diffuse_color), diff_samp.a);
    vec4 diffuse = vec4(vec3(light.color * diffuse_factor), diff_samp.a);

    diffuse *= diff_samp;
    ambient *= diff_samp;

    return (ambient + diffuse);
}
