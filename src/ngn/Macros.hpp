// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#define NGN_DISABLE_COPY(clazz) \
    clazz(const clazz&) = delete; \
    clazz& operator=(const clazz&) = delete;

#define NGN_DISABLE_MOVE(clazz) \
    clazz(clazz&&) = delete; \
    clazz& operator=(clazz&&) = delete;

#define NGN_DISABLE_COPY_MOVE(clazz) \
    NGN_DISABLE_COPY(clazz) \
    NGN_DISABLE_MOVE(clazz)

#define NGN_DEFAULT_COPY(clazz) \
    clazz(const clazz&) = default; \
    clazz& operator=(const clazz&) = default;

#define NGN_DEFAULT_MOVE(clazz) \
    clazz(clazz&&) = default; \
    clazz& operator=(clazz&&) = default;

#define NGN_UNUSED(x) ((void)x)
