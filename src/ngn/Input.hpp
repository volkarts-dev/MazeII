// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include <GLFW/glfw3.h>

namespace ngn {

enum class InputAction : uint32_t
{
    Release = GLFW_RELEASE,
    Press = GLFW_PRESS,
};

inline InputAction toInputAction(int action)
{
    return static_cast<InputAction>(action);
}

enum class InputMods : uint32_t
{
    Shift = GLFW_MOD_SHIFT,
    Ctrl = GLFW_MOD_CONTROL,
    Alt = GLFW_MOD_ALT,
    Super = GLFW_MOD_SUPER,
    CapsLock = GLFW_MOD_CAPS_LOCK,
    NumLock = GLFW_MOD_NUM_LOCK,
};

inline InputMods toInputMods(int mods)
{
    return static_cast<InputMods>(mods);
}

inline InputMods operator|(InputMods mods, InputMods mod)
{
    return static_cast<InputMods>(static_cast<uint32_t>(mods) | static_cast<uint32_t>(mod));
}

inline InputMods operator&(InputMods mods, InputMods mod)
{
    return static_cast<InputMods>(static_cast<uint32_t>(mods) & static_cast<uint32_t>(mod));
}

inline bool inputModsSet(InputMods mods, InputMods test)
{
    return (static_cast<uint32_t>(mods) & static_cast<uint32_t>(test)) > 0;
}

} // namespace ngn
