/**
 * ASE GODOT ADAPTER - HUD BAG ICON
 *
 * @file        godot_hud_bag_icon.cpp
 * @brief       The leather pack closed at the hip: a disc with its drawing, tapped to open, a drop place
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_bag_icon.hpp.
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

#include <ase/adp/godot/godot_hud_bag_icon.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

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

/** Reads the word texts.hud.`key` into `runs`; false with the key in `failed`. */
bool read_word(const UiTheme& theme, const char* key, std::vector<HudRun>& runs, std::string& failed) {
    if (!theme.words(key, runs)) {
        failed = std::string("texts.hud.") + key;
        return false;
    }
    return true;
}

}  // namespace

bool BagIcon::bind(const UiTheme& theme, std::string& failed) {
    double x = 0.0;
    double y = 0.0;
    double fill_alpha = 0.0;
    if (!read_tokens(theme,
                     {{"hud_bag_icon_x", &x}, {"hud_bag_icon_y", &y}, {"hud_bag_icon_radius", &radius_},
                      {"hud_bag_icon_fill_alpha", &fill_alpha}, {"hud_bag_icon_art", &art_},
                      {"hud_bag_icon_art_dy", &art_dy_}, {"hud_bag_icon_hit_radius", &hit_radius_},
                      {"hud_drop_bag_icon_radius", &drop_radius_}, {"hud_bag_icon_load_dy", &load_dy_},
                      {"hud_bag_icon_load_size", &load_size_}, {"hud_bag_icon_line", &line_},
                      {"hud_bag_icon_open_line", &open_line_}, {"hud_load_decimals", &decimals_}},
                     failed) ||
        !read_colors(theme,
                     {{"OVERLAY_BAG_DISC", &disc_}, {"OVERLAY_INK_FAINT", &faint_}, {"DOMAIN_GROUND", &ground_},
                      {"DOMAIN_LIFE", &life_}, {"DOMAIN_ALERT", &alert_}},
                     failed) ||
        !read_word(theme, "load_hip", load_, failed)) {
        return false;
    }
    disc_.a = static_cast<float>(fill_alpha);
    // The threshold of perception is the spec's level, never a number of this unit.
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 2) {
        failed = "hud.perception.levels";
        return false;
    }
    level_load_ = levels[1];
    if (!ring_.bind(theme, failed) || !data_.bind_data(theme, failed) || !pack_.bind(theme, failed) ||
        !pack_.set(GODOT_HUD_ITEM_ART_PACK, failed)) {
        return false;
    }
    at_ = ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
    drawn_ = false;
    return true;
}

std::string BagIcon::number(double value) const {
    // f1: Math.round(v * 10) / 10, printed to hud_load_decimals places.
    const int decimals = static_cast<int>(decimals_);
    return hud_decimals(value, decimals);
}

bool BagIcon::draw(UiPen& canvas, const ::godot::Transform2D& frame, const BagIconFrame& now,
                   std::string& failed) {
    drawn_ = false;
    // The pack stands in full from the frame; who filled it, flora.carry.v1 or the carry bridge, the icon never
    // asks (VivariumView logs N0451 on its edge).
    const bool hot = now.hot != nullptr && std::string(now.hot) == GODOT_HUD_CONTAINER_PACK;
    const ::godot::Color& edge = hot ? (now.refused ? alert_ : life_) : (now.open ? ground_ : faint_);
    const double width = hot || now.open ? open_line_ : line_;

    canvas.draw_set_transform_matrix(frame);
    ring_.disc(canvas, at_, radius_, disc_);
    ring_.circle(canvas, at_, radius_, edge, width, 1.0);

    UiItemSpritePlace place;
    place.centre = at_ + ::godot::Vector2(0.0f, static_cast<float>(art_dy_));
    place.box = art_;
    pack_.draw(canvas, frame, place);
    canvas.draw_set_transform_matrix(frame);

    if (now.level >= level_load_) {
        double kg = 0.0;
        if (now.things != nullptr) {
            for (const BagThing& thing : *now.things) {
                kg += thing.kg;
            }
        }
        std::vector<HudRun> runs = load_;
        if (!fill_words(runs, {HudPortValue{"kg", number(kg)}}, failed) || !data_.set(runs, failed)) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
        UiTextPlace text;
        text.at = at_ + ::godot::Vector2(0.0f, static_cast<float>(load_dy_));
        text.size = load_size_;
        text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        text.color = faint_;
        text.opacity = 1.0;
        text.still = true;
        data_.draw(canvas, frame, text);
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    drawn_ = true;
    return true;
}

const char* BagIcon::hit(const ::godot::Vector2& point) const {
    return drawn_ && point.distance_to(at_) < hit_radius_ ? GODOT_HUD_ACT_PACK_CLOSE : nullptr;
}

bool BagIcon::place(::godot::Rect2& out) const {
    const float radius = static_cast<float>(hit_radius_);
    out = ::godot::Rect2(at_ - ::godot::Vector2(radius, radius), ::godot::Vector2(radius + radius, radius + radius));
    return drawn_;
}

const char* BagIcon::over(const ::godot::Vector2& point) const {
    return drawn_ && point.distance_to(at_) < drop_radius_ ? GODOT_HUD_CONTAINER_PACK : nullptr;
}

}  // namespace ase::adp::godot
