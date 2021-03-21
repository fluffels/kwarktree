#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"
#include "quaternions.glsl"

layout(push_constant) uniform PushConstants {
    vec3 color;
} pushConstants;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inTexCoord1;
layout(location=2) in vec2 inTexCoord2;
layout(location=3) in vec3 inNormal;
layout(location=4) in uint inColor;

layout(location=0) out vec4 outColor;

void main() {
    vec4 p = vec4(inPosition.x, -inPosition.z, inPosition.y, 1.f);
    p -= uniforms.eye;
    p = rotate_vertex_position(uniforms.rotation, p);
    gl_Position = uniforms.proj * p;
    outColor.a = ((inColor & 0xFF000000) >> 24) / 255.f;
    outColor.r = ((inColor & 0x00FF0000) >> 16) / 255.f;
    outColor.g = ((inColor & 0x0000FF00) >>  8) / 255.f;
    outColor.b = ((inColor & 0x000000FF)      ) / 255.f;
}
