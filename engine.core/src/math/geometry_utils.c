// Внутренние подключения.
#include "math/geometry_utils.h"

// Внутренние подключения.
#include "math/kmath.h"

void geometry_generate_normals(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices)
{
    for(u32 i = 0; i < index_count; i += 3)
    {
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        vec3 edge1 = vec3_sub(vertices[i1].position, vertices[i0].position);
        vec3 edge2 = vec3_sub(vertices[i2].position, vertices[i0].position);

        vec3 normal = vec3_normalized(vec3_cross(edge1, edge2));

        // NOTE: Генерирует нормаль поверхности. Сглаживание выполнить отдельно, если это необходимо.
        vertices[i0].normal = normal;
        vertices[i1].normal = normal;
        vertices[i2].normal = normal;
    }
}

// Смотри: https://terathon.com/blog/tangent-space.html
//         https://triplepointfive.github.io/ogltutor/tutorials/tutorial26.html
void geometry_generate_tangent(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices)
{
    for(u32 i = 0; i < index_count; i += 3)
    {
        // Получение индексов вершин треуголиников.
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        // 
        vec3 edge1 = vec3_sub(vertices[i1].position, vertices[i0].position);
        vec3 edge2 = vec3_sub(vertices[i2].position, vertices[i0].position);

        //
        f32 deltaU1 = vertices[i1].texcoord.x - vertices[i0].texcoord.x;
        f32 deltaV1 = vertices[i1].texcoord.y - vertices[i0].texcoord.y;

        f32 deltaU2 = vertices[i2].texcoord.x - vertices[i0].texcoord.x;
        f32 deltaV2 = vertices[i2].texcoord.y - vertices[i0].texcoord.y;

        f32 dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
        f32 fc = 1.0f / dividend;

        vec3 tangent = (vec3){{
            (fc * (deltaV2 * edge1.x - deltaV1 * edge2.x)),
            (fc * (deltaV2 * edge1.y - deltaV1 * edge2.y)),
            (fc * (deltaV2 * edge1.z - deltaV1 * edge2.z))
        }};

        tangent = vec3_normalized(tangent);

        f32 sx = deltaU1, sy = deltaU2;
        f32 tx = deltaV1, ty = deltaV2;
        f32 handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;

        vec3 t3 = vec3_mul_scalar(tangent, handedness);
        vertices[i0].tangent = t3;
        vertices[i1].tangent = t3;
        vertices[i2].tangent = t3;
    }
}
