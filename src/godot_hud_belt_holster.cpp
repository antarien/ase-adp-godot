/**
 * ASE GODOT ADAPTER - HUD BELT HOLSTER
 *
 * @file        godot_hud_belt_holster.cpp
 * @brief       The leather belt across the foot of the screen and its two holsters, one tool each
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_belt_holster.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_belt_holster.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

#include <initializer_list>

namespace ase::adp::godot {

namespace {

/** Reads every token of `slots`; false with the first missing name in `failed`. */
bool read_tokens(const UiTheme& theme, std::initializer_list<std::pair<const char*, double*>> slots,
                 std::string& failed) {
    for (const std::pair<const char*, double*>& slot : slots) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    return true;
}

/** Reads every colour of `slots`; false with the first unknown name in `failed`. */
bool read_colors(const UiTheme& theme, std::initializer_list<std::pair<const char*, ::godot::Color*>> slots,
                 std::string& failed) {
    for (const std::pair<const char*, ::godot::Color*>& slot : slots) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    return true;
}

::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

/** The square of side twice `half` about `middle`. */
::godot::Rect2 square(const ::godot::Vector2& middle, double half) {
    return ::godot::Rect2(middle - at(half, half), at(half + half, half + half));
}

}  // namespace

bool BeltHolster::bind(const UiTheme& theme, std::string& failed) {
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    double fill_alpha = 0.0;
    if (!read_tokens(theme,
                     {{"hud_belt_line", &strap_line_}, {"hud_belt_stitch_line", &stitch_line_},
                      {"hud_belt_stitch_dash", &stitch_dash_}, {"hud_belt_stitch_gap", &stitch_gap_},
                      {"hud_belt_1_x", &x1}, {"hud_belt_1_y", &y1}, {"hud_belt_2_x", &x2}, {"hud_belt_2_y", &y2},
                      {"hud_belt_holster_half", &half_}, {"hud_belt_holster_radius", &radius_},
                      {"hud_belt_holster_fill_alpha", &fill_alpha}, {"hud_belt_holster_line", &line_},
                      {"hud_belt_holster_hot_line", &hot_line_}, {"hud_belt_empty_dash", &empty_dash_},
                      {"hud_belt_empty_gap", &empty_gap_}, {"hud_belt_art", &art_}, {"hud_belt_box", &box_},
                      {"hud_drop_belt_radius", &drop_radius_}},
                     failed) ||
        !read_colors(theme,
                     {{"BELT_STRAP", &strap_ink_}, {"BELT_STRAP_SHADE", &shade_ink_}, {"HOLSTER_GROUND", &ground_},
                      {"HOLSTER_EDGE", &edge_}, {"OVERLAY_INK_FAINT", &empty_}, {"DOMAIN_LIFE", &life_},
                      {"DOMAIN_ALERT", &alert_}},
                     failed)) {
        return false;
    }
    ground_.a = static_cast<float>(fill_alpha);
    ::godot::String path;
    if (!theme.token_text("hud_belt_path", path) || path.is_empty()) {
        failed = "hud_belt_path";
        return false;
    }
    if (!read_svg_path(std::string(path.utf8().get_data()), strap_, failed)) {
        return false;
    }
    at_[0] = at(x1, y1);
    at_[1] = at(x2, y2);
    theme_ = &theme;
    sprites_.clear();
    return true;
}

const UiItemSprite* BeltHolster::sprite(const std::string& art, std::string& failed) {
    for (const std::pair<std::string, UiItemSprite>& entry : sprites_) {
        if (entry.first == art) {
            return &entry.second;
        }
    }
    UiItemSprite drawing;
    if (!drawing.bind(*theme_, failed) || !drawing.set(art, failed)) {
        return nullptr;
    }
    sprites_.emplace_back(art, drawing);
    return &sprites_.back().second;
}

bool BeltHolster::draw(UiPen& canvas, const ::godot::Transform2D& frame, const BeltHolsterFrame& now,
                       std::string& failed) {
    places_.clear();
    held_[0].clear();
    held_[1].clear();
    drawn_ = false;
    if (theme_ == nullptr) {
        failed = "hud.belt_holster.unbound";
        return false;
    }
    // The belt stands in full from the frame; who filled it, flora.carry.v1 or the carry bridge, the belt never
    // asks (VivariumView logs N0451 on its edge).
    canvas.draw_set_transform_matrix(frame);
    for (const UiSvgOutline& outline : strap_) {
        svg_stroke(canvas, outline, strap_ink_, strap_line_);
        svg_stroke_dashed(canvas, outline, shade_ink_, stitch_line_, stitch_dash_, stitch_gap_);
    }
    const bool drawn =
        draw_holster(canvas, frame, at_[0], GODOT_HUD_CONTAINER_BELT_FIRST, now.first, now, failed) &&
        draw_holster(canvas, frame, at_[1], GODOT_HUD_CONTAINER_BELT_SECOND, now.second, now, failed);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    drawn_ = drawn;
    return drawn;
}

bool BeltHolster::draw_holster(UiPen& canvas, const ::godot::Transform2D& frame,
                               const ::godot::Vector2& middle, const char* name, const BagThing* thing,
                               const BeltHolsterFrame& now, std::string& failed) {
    const bool hot = now.hot != nullptr && std::string(now.hot) == name;
    const ::godot::Color& ink = hot ? (now.refused ? alert_ : life_) : (thing != nullptr ? edge_ : empty_);
    const double width = hot ? hot_line_ : line_;
    UiSvgOutline holster;
    svg_append_rounded_rect(holster, square(middle, half_), static_cast<float>(radius_));
    canvas.draw_set_transform_matrix(frame);
    svg_fill(canvas, holster, ground_);
    if (thing != nullptr) {
        svg_stroke(canvas, holster, ink, width);
    } else {
        svg_stroke_dashed(canvas, holster, ink, width, empty_dash_, empty_gap_);
    }

    if (thing == nullptr) {
        return true;
    }
    const size_t slot = std::string(name) == GODOT_HUD_CONTAINER_BELT_FIRST ? 0 : 1;
    held_[slot] = thing->key;
    const UiItemSprite* drawing = sprite(thing->art, failed);
    if (drawing == nullptr) {
        return false;
    }
    const double k = box_ / ase::math::max(drawing->view_width(), drawing->view_height());
    places_.push_back(BagThingPlace{thing->key, square(middle, box_ * GODOT_HALF), k});
    if (!now.dragged.empty() && now.dragged == thing->key) {
        return true;
    }
    UiItemSpritePlace place;
    place.centre = middle;
    place.box = art_;
    place.level = thing->level;
    drawing->draw(canvas, frame, place);
    canvas.draw_set_transform_matrix(frame);
    return true;
}

BeltHolsterHit BeltHolster::hit(const ::godot::Vector2& point) const {
    if (!drawn_) {
        return BeltHolsterHit{};
    }
    for (size_t slot = 0; slot < 2; ++slot) {
        if (square(at_[slot], half_).has_point(point)) {
            return held_[slot].empty() ? BeltHolsterHit{GODOT_HUD_ACT_NOOP, std::string()}
                                       : BeltHolsterHit{GODOT_HUD_ACT_ITEM, held_[slot]};
        }
    }
    return BeltHolsterHit{};
}

const char* BeltHolster::over(const ::godot::Vector2& point) const {
    if (point.distance_to(at_[0]) < drop_radius_) {
        return GODOT_HUD_CONTAINER_BELT_FIRST;
    }
    if (point.distance_to(at_[1]) < drop_radius_) {
        return GODOT_HUD_CONTAINER_BELT_SECOND;
    }
    return nullptr;
}

void BeltHolster::holster_places(std::vector<std::pair<std::string, ::godot::Rect2>>& out) const {
    if (!drawn_) {
        return;
    }
    const ::godot::Vector2 half(static_cast<float>(half_), static_cast<float>(half_));
    const char* names[2] = {GODOT_HUD_CONTAINER_BELT_FIRST, GODOT_HUD_CONTAINER_BELT_SECOND};
    for (size_t index = 0; index < 2; ++index) {
        out.emplace_back(names[index], ::godot::Rect2(at_[index] - half, half * 2.0f));
    }
}

}  // namespace ase::adp::godot
