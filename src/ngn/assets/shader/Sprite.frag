// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D[] uSampler;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 texCoords;
layout(location = 2) in flat uint texIndex;

layout(location = 0) out vec4 fColor;

void main()
{
    vec4 texColor;
    if (texIndex == 0)
        texColor = textureLod(uSampler[0], texCoords, 0);
    else if (texIndex == 1)
        texColor = textureLod(uSampler[1], texCoords, 0);
    else if (texIndex == 2)
        texColor = textureLod(uSampler[2], texCoords, 0);
    else if (texIndex == 3)
        texColor = textureLod(uSampler[3], texCoords, 0);
    else if (texIndex == 4)
        texColor = textureLod(uSampler[4], texCoords, 0);
    else if (texIndex == 5)
        texColor = textureLod(uSampler[5], texCoords, 0);
    else if (texIndex == 6)
        texColor = textureLod(uSampler[6], texCoords, 0);
    else if (texIndex == 7)
        texColor = textureLod(uSampler[7], texCoords, 0);

    fColor = texColor * color;
}
