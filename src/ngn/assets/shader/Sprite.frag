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
    if (gTexIndex != 99)
    {
        fColor = texture(uSampler[1], gTexCoords);
    }
    else
    {
        fColor = gColor;
    }
}
