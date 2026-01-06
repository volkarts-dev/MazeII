// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform ViewProjection {
    mat4 view;
    mat4 proj;
} uViewProj;

layout(location = 0) in vec2 point;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 vColor;

void main()
{
    mat4 mvp = uViewProj.proj * uViewProj.view;

    gl_Position = mvp * vec4(point, 0.0, 1.0);
    vColor = color;
}
