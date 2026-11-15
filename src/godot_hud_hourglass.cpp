/**
 * ASE GODOT ADAPTER - HUD HOURGLASS
 *
 * @file        godot_hud_hourglass.cpp
 * @brief       The hourglass of the calendar sheet: the game clock's pace shown, never asked
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_hourglass.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_046
 * @category    ecs/module
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Growth Clock Controller (GROWCLOCON)
 * @register    N0482 (the hourglass a display), N0459 (flora.pace.v1: the factor the clock runs at), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_hourglass.hpp>
#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool Hourglass::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_sheet_x", &sheet_x_},
             {"hud_glass_label_y", &label_y_},
             {"hud_glass_label_size", &label_size_},
             {"hud_glass_x", &x_},
             {"hud_glass_y", &y_},
             {"hud_glass_size", &size_},
             {"hud_glass_chip_x", &chip_x_},
             {"hud_glass_chip_radius", &chip_radius_},
             {"hud_line_chip_on", &chip_line_on_},
             {"hud_glass_chip_text_dy", &chip_text_dy_},
             {"hud_glass_chip_text_size", &chip_text_size_},
             {"hud_glass_note_y", &note_y_},
             {"hud_glass_note_size", &note_size_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FAINT", &faint_},
                                                                       {"DOMAIN_TIME", &time_},
                                                                       {"DOMAIN_TIME_CHIP_ON", &chip_on_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    std::vector<HudRun> head;
    std::vector<HudRun> note;
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{
             {"time", &head}, {"time_note", &note}, {"time_factor", &factor_words_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    if (!bind_word_icon(theme, "time_glyph", glyph_, failed)) {
        return false;
    }
    shown_ = -1.0;
    return head_.bind_label(theme, failed) && note_.bind_label(theme, failed) && factor_.bind_data(theme, failed) &&
           head_.set(head, failed) && note_.set(note, failed);
}

bool Hourglass::draw(UiPen& canvas, const ::godot::Transform2D& frame, const HourglassFrame& now,
                     std::string& failed) {
    // T(sheet_x, label_y, time).
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(sheet_x_), static_cast<float>(label_y_));
    text.size = label_size_;
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = soft_;
    text.opacity = 1.0;
    text.elapsed = now.elapsed;
    text.still = now.still;
    head_.draw(canvas, frame, text);

    // G(x, y, time_glyph, size, time), turned by the game hour; upright while every motion is held.
    if (now.still) {
        glyph_.draw(canvas, frame, ::godot::Vector2(static_cast<float>(x_), static_cast<float>(y_)), size_, time_,
                    1.0);
    } else {
        draw_turned(canvas, frame, now.hour);
    }

    // The factor the clock runs at, on one hexagon centred at (chip_x, y): a readout, no chip to choose.
    if (!show_factor(now.factor, failed)) {
        return false;
    }
    const ::godot::Vector2 centre(static_cast<float>(chip_x_), static_cast<float>(y_));
    const ::godot::PackedVector2Array corners = hud_hex_points(centre, chip_radius_);
    canvas.draw_set_transform_matrix(frame);
    canvas.draw_colored_polygon(corners, chip_on_);
    ::godot::PackedVector2Array outline = corners;
    outline.push_back(corners[0]);
    canvas.draw_polyline(outline, time_, static_cast<float>(chip_line_on_), true);
    UiTextPlace word = text;
    word.at = centre + ::godot::Vector2(0.0f, static_cast<float>(chip_text_dy_));
    word.size = chip_text_size_;
    word.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    word.color = time_;
    factor_.draw(canvas, frame, word);

    // T(sheet_x, note_y, time_note): the game's pace is constant, the time lapse stays in the debug layer.
    UiTextPlace line = text;
    line.at = ::godot::Vector2(static_cast<float>(sheet_x_), static_cast<float>(note_y_));
    line.size = note_size_;
    line.color = faint_;
    note_.draw(canvas, frame, line);
    return true;
}

void Hourglass::draw_turned(UiPen& canvas, const ::godot::Transform2D& frame, double hour) const {
    // One turn per game hour: the share of the hour the clock has reached, never the pace, so the glass turns as
    // fast as the clock runs and stands where a jump lands. Rastered at the whole screen pixel size like
    // UiIcon::draw under a frame, the glyph centred on the origin of a transform that turns round its centre.
    const ::godot::Transform2D screen = canvas.screen();
    const ::godot::Transform2D device = screen * frame;
    const int64_t raster = hud_raster(device, size_);
    const ::godot::Vector2 at = device.xform(::godot::Vector2(static_cast<float>(x_), static_cast<float>(y_)));
    const double share = ::godot::Math::fposmod(hour, 1.0);
    const auto angle = static_cast<float>(::godot::Math::deg_to_rad(share * GODOT_RING_TURN_DEG));
    canvas.draw_set_transform_matrix(screen.affine_inverse() * ::godot::Transform2D(angle, at));
    glyph_.draw(canvas, ::godot::Vector2(), static_cast<double>(raster), time_, 1.0);
    canvas.draw_set_transform_matrix(frame);
}

bool Hourglass::show_factor(double factor, std::string& failed) {
    if (::godot::Math::is_equal_approx(factor, shown_)) {
        return true;
    }
    std::vector<HudRun> runs = factor_words_;
    if (!fill_words(runs, {HudPortValue{"f", hud_shortest(factor)}}, failed) || !factor_.set(runs, failed)) {
        return false;
    }
    shown_ = factor;
    return true;
}

}  // namespace ase::adp::godot
