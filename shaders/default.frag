#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants {
    int texIndex;
} pushConstants;

layout(binding=1) uniform sampler2D surface[200];

layout(location=0) in vec4 color;
layout(location=1) in vec2 texCoord;

layout(location=0) out vec4 outColor;

void main() {
    // TODO: this colour actually seems relevant
    // some lighting component?
    // outColor = vec4(color.rgb, 1);
    outColor = texture(surface[pushConstants.texIndex], texCoord);
}
