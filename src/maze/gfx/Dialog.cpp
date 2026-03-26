// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "Dialog.hpp"

#include "GameStage.hpp"
#include "gfx/FontCollection.hpp"
#include "gfx/UiRenderer.hpp"

namespace {

constexpr auto LineHeightFactor = 2.0f;

} // namespace

Dialog::Dialog(GameStage* gameStage) :
    gameStage_{gameStage},
    uiRenderer_{gameStage_->app()->uiRenderer()},
    resources_{gameStage_->delegate()->resources()}
{
    reset();
}

Dialog::~Dialog()
{
}

void Dialog::show(DialogData data)
{
    data_ = std::move(data);
    state_ = State::Active;
    button_ = data_.defaultButton;
    parseText(data_.text);
}

void Dialog::reset()
{
    state_ = State::Inactive;
    button_ = DialogButton::None;
}

bool Dialog::handleInputEvents(ngn::InputAction action, int key, ngn::InputMods mods)
{
    NGN_UNUSED(mods);

    if (state_ != State::Active || action != ngn::InputAction::Press)
        return false;

    if (!data_.button2.empty())
    {
        if ((key == GLFW_KEY_LEFT) ||
            (key == GLFW_KEY_RIGHT) ||
            (key == GLFW_KEY_TAB))
        {
            button_ = button_ == DialogButton::One ? DialogButton::Two : DialogButton::One;
        }
    }

    if (key == GLFW_KEY_ENTER)
    {
        switch (button_)
        {
            case DialogButton::One:
                if (data_.button1Callback)
                    data_.button1Callback();
                state_ = State::Finished;
                break;
            case DialogButton::Two:
                if (data_.button2Callback)
                    data_.button2Callback();
                state_ = State::Finished;
                break;
            case DialogButton::None:
                break;
        }
    }

    return true;
}

void Dialog::update(float deltaTime)
{
    NGN_UNUSED(deltaTime);
}

void Dialog::draw()
{
    if (state_ != State::Active)
        return;

    const auto pos = dialogPos();

    const glm::vec2 base{256, 0};

    uiRenderer_->renderSprite({
        .position = pos,
        .rotation = 0.0f,
        .scale = data_.size,
        .color = {1.0, 1.0, 1.0, 1.0},
        .texCoords = glm::vec4{base, base + data_.size},
        .texIndex = std::to_underlying(resources_.uiTexture),
    });

    const auto titlePos = pos - centerPos(ngn::FontId{0}, data_.title) + glm::vec2{0, -96};
    uiRenderer_->renderText(ngn::FontId{0}, data_.title, titlePos);

    for (uint32_t i = 0; i < lines_.size(); i++)
    {
        const auto yPos =
                -textHeight_ * 0.5f +
                textHeight_ * (static_cast<float>(i) / static_cast<float>(lines_.size()));
        const auto linePos = pos + glm::vec2{0, yPos};

        const auto textRelPos = -centerPos(ngn::FontId{1}, lines_[i]);
        uiRenderer_->renderText(ngn::FontId{1}, lines_[i], linePos + textRelPos);
    }

    const bool secondBtn = !data_.button2.empty();

    drawButton(pos + glm::vec2{secondBtn ? -64 : 0, 100}, data_.button1, DialogButton::One);

    if (secondBtn)
    {
        drawButton(pos + glm::vec2{64, 100}, data_.button2, DialogButton::Two);
    }
}

void Dialog::drawButton(const glm::vec2& pos, const std::string_view& text, DialogButton button)
{
    const bool selected = button == button_;

    uiRenderer_->renderSprite({
        .position = pos,
        .rotation = 0.0f,
        .scale = {96, 25},
        .color = {1.0, 1.0, 1.0, 1.0},
        .texCoords = selected ? glm::vec4{352, 256, 448, 281} : glm::vec4{256, 256, 351, 281},
        .texIndex = std::to_underlying(resources_.uiTexture),
    });

    const auto btnTxtPos = pos - centerPos(ngn::FontId{1}, text);
    uiRenderer_->renderText(ngn::FontId{1}, text, btnTxtPos);
}

glm::vec2 Dialog::dialogPos()
{
    return gameStage_->app()->windowSize() * 0.5f;
}

inline glm::vec2 Dialog::centerPos(ngn::FontId fontId, std::string_view text)
{
    const auto dim = uiRenderer_->fontCollection()->textDimension(fontId, text) * 0.5f;
    return {dim.x, -dim.y};
}

void Dialog::parseText(std::string_view text)
{
    lines_.clear();

    while (!text.empty() && lines_.size() < lines_.capaciy())
    {
        const auto nl = text.find('\n');
        lines_.push_back(text.substr(0, nl));
        if (nl == std::string_view::npos)
            break;
        text = text.substr(nl + 1);
    }

    const auto dim = uiRenderer_->fontCollection()->textDimension(ngn::FontId{1}, "Ag");
    const float lineHeight = dim.y * LineHeightFactor;
    textHeight_ = lineHeight * static_cast<float>(lines_.size());
}
