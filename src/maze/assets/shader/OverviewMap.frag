// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vColor;

layout(location = 0) out vec4 fColor;

void main()
{
    fColor = vColor;
}
