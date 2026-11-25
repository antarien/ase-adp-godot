#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI FRAME RATE
 *
 * @file        godot_hud_ui_frame_rate.hpp
 * @brief       The small number of the frames per second in the upper right corner, its place, ink and tone
 * @description Operator order 2026-11-24: the frames per second stand in a corner of the screen while the game runs, as
 *              a very small number that can still be read. UiFrameRate holds the count (HudFrameRate) and draws it.
 *
 *              THE PLACE IS THE GAP OF THE STATUS LINE: right of the battery up to the edge of the mockup's frame,
 *              on the battery's height, centred, in the size of the status line's clock. Every measure is a token of
 *              SHA the status line already has (hud_band_battery_*, hud_band_clock_size, hud_frame_width). Nothing
 *              of the HUD is drawn there and no touch lands there: the debug gear stands under it, the sky's field
 *              ends left of it. The ink is the HUD's ink; a number under the floor takes the tone of the alert. The
 *              outline in the HUD's shadow (hud_text_shadow_width) keeps it readable over a bright sky.
 *
 *              A CANVAS ITEM OF ITS OWN. The view's canvas is drawn anew in every frame. The number is a Label under
 *              the view's node, laid again only when the number, its tone or its place changes, so the reading of the
 *              frame rate costs the frame no drawing. It takes no touch.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_frame_rate.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>

namespace ase::adp::godot {

/** The frames per second of the running view, counted and shown. */
class UiFrameRate {
public:
    /**
     * Binds the measures, colours and the face of the theme, the window of hud_refresh_ms and the floor of
     * `frame_budget_ms`. False with `failed` naming what is missing or cannot judge.
     */
    bool bind(const UiTheme& theme, double frame_budget_ms, std::string& failed);

    /**
     * One frame, `elapsed_s` real seconds after the one before. `origin` and `scale` lay the mockup's units of the
     * band zone onto the canvas; a scale that is not positive hides the number. True when the verdict changed: the
     * caller writes its one line.
     */
    bool frame(::godot::Node& parent, double elapsed_s, const ::godot::Vector2& origin, float scale);

    /** The count starts anew with the next frame: the app was away, its span is no frame. */
    void restart() { rate_.restart(); }

    /** The number is hidden and the count starts anew: the host stands, there is no running picture to count. */
    void rest();

    const HudFrameRate& rate() const { return rate_; }

private:
    void lay(::godot::Node& parent);                              // the label under the view's node, once
    void place(const ::godot::Vector2& origin, float scale);      // its rectangle and its size, on a change only

    HudFrameRate     rate_;
    ::godot::Label*  label_ = nullptr;      // child of the view's node; Godot frees it with the node
    ::godot::Ref<::godot::FontFile> face_;  // the face of every number of the HUD
    ::godot::Color   ink_;
    ::godot::Color   alert_;
    ::godot::Color   shadow_;
    ::godot::Rect2   gap_;                  // the free gap of the status line, mockup units
    double           size_ = 0.0;           // type size, mockup units
    double           outline_ = 0.0;        // width of the outline, mockup units
    ::godot::Vector2 origin_;               // where the band zone's units were laid last
    float            scale_ = 0.0f;         // with which scale, 0 = never
};

}  // namespace ase::adp::godot
