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
layout(location=4) in float inColor;

layout(location=0) out vec3 outColor;

void main() {
    vec4 p = vec4(inPosition, 1.f);
    p -= uniforms.eye;
    p = rotate_vertex_position(uniforms.rotation, p);
    gl_Position = uniforms.proj * p;
    outColor = pushConstants.color;
}
