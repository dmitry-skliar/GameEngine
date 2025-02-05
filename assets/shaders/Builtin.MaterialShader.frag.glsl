#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec4 out_color;

void main()
{
    // out_color = vec4(1.0, 0.5, 0.0, 1.0);
    out_color = vec4(in_position.r + 0.4, in_position.g + 0.3, in_position.b + 0.5, 1.0);
}
