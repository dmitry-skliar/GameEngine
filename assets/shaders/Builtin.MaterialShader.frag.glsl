#version 450

// Должно соответствовать subpass.pColorAttachments = ..., 0 индексу массива.
layout(location = 0) out vec4 out_color;

// Порядок должен соответствовать uniform-переменным уровня экземпляра в shadercfg.
layout(set = 1, binding = 0) uniform local_uniform_object {
    vec4 diffuse_color; // Цветовой фильтр материала? (заданое в материале RGBA).
    float shininess;
} object_ubo;

const int SAMP_DIFFUSE  = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL   = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

// flat указывает что значение не интерполируется, оно всегда одно и тоже.
layout(location = 0) flat in int in_mode;

// Принимаемые данные переданные по конвейеру от другого шейдера (data transfer object).
layout(location = 1) in struct dto {
    vec4 ambient;       // Цвет неосвещенной поверхности.
    vec2 tex_coord;     // Текстурные координаты.
    vec3 normal;        // Вектор нормали (трансформированые).
    vec3 view_position;
    vec3 frag_position;
    vec4 color;
    vec4 tangent;
} in_dto;

// Общее освещение сцены.
struct directional_light {
    vec3 direction;    // Вектор направления.
    vec4 color;        // RGBA.
};

// Отдельный источник света.
struct point_light {
    vec3 position;
    vec4 color;
    float constant;    // Обычно 1, сделать так, чтобы знаменатель никогда не был меньше 1.
    float linear;      // Линейно уменьшает интенсивность света.
    float quadratic;   // Уменьшает падение света на больших расстояниях.
};

// TODO: Задавать из приложения.
directional_light source_light = {
    vec3(-0.57735, -0.57735, -0.57735),
    vec4(0.8, 0.8, 0.8, 1.0)
};

point_light point_light0 = {
    vec3(-10.5, 0.0, -10.5),
    vec4(0.0, 1.0, 0.0, 1.0),
    1.0,
    0.0001,
    0.05
};

point_light point_light1 = {
    vec3(10.5, 0.0, -10.5),
    vec4(1.0, 0.0, 0.0, 1.0),
    1.0,
    0.0001, // 0.35
    0.05    // 0.44
};


// Tangent, bitangent and normal.
mat3 TBN;

// Функиця расчета освещения для пикселя по заданой нормали (глобальное освещение).
vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction);

// Функиця расчета освещения для пикселя по заданой нормали (отдельного источника освещения).
vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction);

// В фрагментном шейдере main применяется к каждому пикселю.
void main()
{
    vec3 normal = in_dto.normal;
    vec3 tangent = in_dto.tangent.xyz;
    tangent = (tangent - dot(tangent, normal) * normal);
    vec3 bitangent = cross(in_dto.normal, in_dto.tangent.xyz) * in_dto.tangent.w;
    TBN = mat3(tangent, bitangent, normal);

    // Обновление нормали для использования сэмплера для normal map.
    vec3 local_normal = 2.0 * texture(samplers[SAMP_NORMAL], in_dto.tex_coord).rgb - 1.0;
    normal = normalize(TBN * local_normal);

    if(in_mode == 0 || in_mode == 1)
    {
        vec3 view_direction = normalize(in_dto.view_position - in_dto.frag_position);

        out_color = calculate_directional_light(source_light, normal, view_direction);

        out_color += calculate_point_light(point_light0, normal, in_dto.frag_position, view_direction);
        out_color += calculate_point_light(point_light1, normal, in_dto.frag_position, view_direction);
    }
    else if(in_mode == 2)
    {
        out_color = vec4(abs(normal), 1.0);
    }
}

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction)
{
    // Получаем степень освещености, но только в положительном направлении векторов.
    float diffuse_factor = max(dot(normal, -light.direction), 0.0);

    vec3 half_direction = normalize(view_direction - light.direction);
    float specular_factor = pow(max(dot(half_direction, normal), 0.0), object_ubo.shininess);

    // Получаем цвет пикселя текстуры.
    vec4 diff_samp = texture(samplers[SAMP_DIFFUSE], in_dto.tex_coord);
    vec4 ambient = vec4(vec3(in_dto.ambient * object_ubo.diffuse_color), diff_samp.a);
    vec4 diffuse = vec4(vec3(light.color * diffuse_factor), diff_samp.a);
    vec4 specular = vec4(vec3(light.color * specular_factor), diff_samp.a);

    if(in_mode == 0)
    {
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4(texture(samplers[SAMP_SPECULAR], in_dto.tex_coord).rgb, diffuse.a);
    }

    return (ambient + diffuse + specular);
}

vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction)
{
    vec3 light_direction = normalize(light.position - frag_position);
    float diff = max(dot(normal, light_direction), 0.0);

    vec3 reflect_direction = reflect(-light_direction, normal);
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), object_ubo.shininess);

    // Вычисление затухания света с расстоянием.
    float distance = length(light.position - frag_position);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec4 ambient = in_dto.ambient;
    vec4 diffuse = light.color * diff;
    vec4 specular = light.color * spec;

    if(in_mode == 0)
    {
        vec4 diff_samp = texture(samplers[SAMP_DIFFUSE], in_dto.tex_coord);
        diffuse *= diff_samp;
        ambient *= diff_samp;
        specular *= vec4(texture(samplers[SAMP_SPECULAR], in_dto.tex_coord).rgb, diffuse.a);
    }

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}
