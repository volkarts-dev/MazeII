// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D[] uSampler;

layout(location = 0) in vec4 gColor;
layout(location = 1) in flat uint gTexIndex;
layout(location = 2) in vec2 gTexCoords;

layout(location = 0) out vec4 fColor;

void main()
{
    vec4 texColor;
    if (gTexIndex == 0)
        texColor = textureLod(uSampler[0], gTexCoords, 0);
    else if (gTexIndex == 1)
        texColor = textureLod(uSampler[1], gTexCoords, 0);
    else if (gTexIndex == 2)
        texColor = textureLod(uSampler[2], gTexCoords, 0);
    else if (gTexIndex == 3)
        texColor = textureLod(uSampler[3], gTexCoords, 0);
    else if (gTexIndex == 4)
        texColor = textureLod(uSampler[4], gTexCoords, 0);
    else if (gTexIndex == 5)
        texColor = textureLod(uSampler[5], gTexCoords, 0);
    else if (gTexIndex == 6)
        texColor = textureLod(uSampler[6], gTexCoords, 0);
    else if (gTexIndex == 7)
        texColor = textureLod(uSampler[7], gTexCoords, 0);

    fColor = texColor * gColor;
}
