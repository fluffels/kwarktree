#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "uniforms.glsl"
#include "rotors.glsl"

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inTexCoord;
layout(location=2) in vec2 inLightMapCoord;
layout(location=3) in vec3 inNormal;
layout(location=4) in uint inColor;

layout(location=0) out vec4 outColor;
layout(location=1) out vec2 outTexCoord;
layout(location=2) out vec2 outLightMapCoord;

void main() {
    vec3 p = vec3(inPosition.x, -inPosition.z, inPosition.y);
    p = p - uniforms.eye.xyz;
    p = rotorRotate(uniforms.rotation, p);
    gl_Position = uniforms.proj * vec4(p, 1);
    outColor.a = ((inColor & 0xFF000000) >> 24) / 255.f;
    outColor.r = ((inColor & 0x00FF0000) >> 16) / 255.f;
    outColor.g = ((inColor & 0x0000FF00) >>  8) / 255.f;
    outColor.b = ((inColor & 0x000000FF)      ) / 255.f;
    outTexCoord = inTexCoord;
    outLightMapCoord = inLightMapCoord;
}
