// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Input.hpp"
#include "MazeDelegate.hpp"
#include "utils/StaticVector.hpp"
#include <entt/signal/delegate.hpp>

namespace ngn {
class UiRenderer;
} // namespace ngn

class GameStage;

enum class DialogButton
{
    None,
    One,
    Two,
};

class DialogData
{
public:
    glm::vec2 size{};
    std::string_view title{};
    std::string_view text{};
    std::string_view button1{};
    std::string_view button2{};
    entt::delegate<void()> button1Callback{};
    entt::delegate<void()> button2Callback{};
    DialogButton defaultButton{DialogButton::None};
};

class Dialog
{
public:
    static constexpr std::size_t MaxLines = 8;

    enum class State
    {
        Inactive,
        Active,
        Finished,
    };

public:
    Dialog(GameStage* gameStage);
    ~Dialog();

    void show(DialogData data);

    bool isActive() const { return state_ == State::Active; }
    bool isFinished() const { return state_ == State::Finished; }
    DialogButton button() const { return button_; }
    void reset();

    bool handleInputEvents(ngn::InputAction action, int key, ngn::InputMods mods);
    void update(float deltaTime);
    void draw();

private:
    void drawButton(const glm::vec2& pos, const std::string_view& text, DialogButton button);

    glm::vec2 dialogPos();
    glm::vec2 centerPos(ngn::FontId fontId, std::string_view text);
    void parseText(std::string_view text);

private:
    GameStage* gameStage_;
    ngn::UiRenderer* uiRenderer_;
    const Resources& resources_;

    DialogData data_{};
    ngn::StaticVector<std::string_view, MaxLines> lines_{};
    float textHeight_{};
    State state_{};
    DialogButton button_{};

};
