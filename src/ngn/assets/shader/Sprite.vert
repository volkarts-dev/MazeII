// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 position;
layout(location = 1) in float rotation;
layout(location = 2) in vec2 scale;
layout(location = 3) in vec4 color;
layout(location = 4) in uint texIndex;
layout(location = 5) in vec4 texCoords;

layout(location = 0) out float vRotation;
layout(location = 1) out vec2 vScale;
layout(location = 2) out vec4 vColor;
layout(location = 3) out uint vTexIndex;
layout(location = 4) out vec4 vTexCoords;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    vRotation = rotation;
    vScale = scale;
    vColor = color;
    vTexIndex = texIndex;
    vTexCoords = texCoords;
}
