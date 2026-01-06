// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (points) in;
layout (triangle_strip, max_vertices = 5) out;

layout(binding = 0) uniform ViewProjection {
    mat4 view;
    mat4 proj;
} uViewProj;

layout(location = 0) in float[] vRotation;
layout(location = 1) in vec2[] vScale;
layout(location = 2) in vec4[] vColor;
layout(location = 3) in uint[] vTexIndex;
layout(location = 4) in vec4[] vTexCoords;

layout(location = 0) out vec4 gColor;
layout(location = 1) out uint gTexIndex;
layout(location = 2) out vec2 gTexCoords;

vec2 positions[] = vec2[](
    vec2(-0.5, -0.5),
    vec2( 0.5, -0.5),
    vec2(-0.5,  0.5),
    vec2( 0.5,  0.5)
);

void main()
{
    float sinT = sin(vRotation[0]);
    float cosT = cos(vRotation[0]);

    mat4 translate = mat4(
                vec4(1.0, 0.0, 0.0, 0.0),
                vec4(0.0, 1.0, 0.0, 0.0),
                vec4(0.0, 0.0, 1.0, 0.0),
                gl_in[0].gl_Position
                );
    mat4 rotate = mat4(
                vec4(cosT, -sinT, 0.0, 0.0),
                vec4(sinT, cosT, 0.0, 0.0),
                vec4(0.0, 0.0, 1.0, 0.0),
                vec4(0.0, 0.0, 0.0, 1.0)
                );
    mat4 scale = mat4(
                vec4(vScale[0].x, 0.0, 0.0, 0.0),
                vec4(0.0, vScale[0].y, 0.0, 0.0),
                vec4(0.0, 0.0, 1.0, 0.0),
                vec4(0.0, 0.0, 0.0, 1.0)
                );

    mat4 model = translate * rotate * scale;

    mat4 mvp = uViewProj.proj * uViewProj.view * model;

    gl_Position = mvp * vec4(positions[0], 0.0, 1.0);
    gColor = vColor[0];
    gTexIndex = vTexIndex[0];
    gTexCoords = vTexCoords[0].xy;
    EmitVertex();

    gl_Position = mvp *vec4(positions[1], 0.0, 1.0);
    gColor = vColor[0];
    gTexIndex = vTexIndex[0];
    gTexCoords = vTexCoords[0].zy;
    EmitVertex();

    gl_Position = mvp * vec4(positions[2], 0.0, 1.0);
    gColor = vColor[0];
    gTexIndex = vTexIndex[0];
    gTexCoords = vTexCoords[0].xw;
    EmitVertex();

    gl_Position = mvp * vec4(positions[3], 0.0, 1.0);
    gColor = vColor[0];
    gTexIndex = vTexIndex[0];
    gTexCoords = vTexCoords[0].zw;
    EmitVertex();

    EndPrimitive();
}
