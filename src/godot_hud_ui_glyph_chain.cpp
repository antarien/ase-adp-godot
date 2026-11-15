/**
 * ASE GODOT ADAPTER - HUD UI GLYPH CHAIN
 *
 * @file        godot_hud_ui_glyph_chain.cpp
 * @brief       Draws the chain of the day's phases with the wandering sun or moon as the mockup's band() does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_glyph_chain.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_glyph_chain.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

namespace {

/** Reads the token `name` into `out`; false with `name` in `failed`. */
bool read_token(const UiTheme& theme, const char* name, double& out, std::string& failed) {
    if (!theme.measure(name, out)) {
        failed = name;
        return false;
    }
    return true;
}

/** Reads the colour `name` into `out`; false with `name` in `failed`. */
bool read_color(const UiTheme& theme, const char* name, ::godot::Color& out, std::string& failed) {
    if (!theme.color(name, out)) {
        failed = name;
        return false;
    }
    return true;
}

/** `color` with its alpha multiplied by `opacity`. */
::godot::Color faded(const ::godot::Color& color, double opacity) {
    ::godot::Color out = color;
    out.a = static_cast<float>(static_cast<double>(color.a) * opacity);
    return out;
}

}  // namespace

bool UiGlyphChain::bind(const UiTheme& theme, std::string& failed) {
    if (!line_.bind(theme, failed) || !ring_.bind(theme, failed) || !sun_.bind(theme, "ICON_SUN", failed) ||
        !moon_.bind(theme, "ICON_MOON", failed)) {
        return false;
    }
    if (!read_color(theme, "OVERLAY_INK_FAINT", faint_, failed) ||
        !read_color(theme, "OVERLAY_INK_SOFT", soft_, failed) || !read_color(theme, "DOMAIN_TIME", time_, failed) ||
        !read_color(theme, "DOMAIN_CUT", cut_, failed) || !read_color(theme, "DOMAIN_SKY", sky_, failed) ||
        !read_color(theme, "OVERLAY_DOT_DARK", dark_, failed)) {
        return false;
    }
    if (!read_token(theme, "hud_band_chain_x", x_, failed) ||
        !read_token(theme, "hud_band_chain_step", step_, failed) ||
        !read_token(theme, "hud_band_chain_y", y_, failed) ||
        !read_token(theme, "hud_band_light_line", light_line_, failed) ||
        !read_token(theme, "hud_band_light_alpha", light_alpha_, failed) ||
        !read_token(theme, "hud_band_phase_on_radius", on_radius_, failed) ||
        !read_token(theme, "hud_band_phase_off_radius", off_radius_, failed) ||
        !read_token(theme, "hud_band_phase_label_y", label_y_, failed) ||
        !read_token(theme, "hud_band_phase_label_size", label_size_, failed) ||
        !read_token(theme, "hud_band_sun_y", sun_y_, failed) ||
        !read_token(theme, "hud_band_sun_size", sun_size_, failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool UiGlyphChain::set(const std::vector<UiGlyphChainPhase>& phases, int32_t current, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.glyph_chain.unbound";
        return false;
    }
    if (phases.empty() || current < 0 || static_cast<size_t>(current) >= phases.size()) {
        failed = "hud.glyph_chain.current";
        return false;
    }
    // A word binds once per phase; a change of the phases' count binds the new ones.
    if (labels_.size() != phases.size()) {
        labels_.assign(phases.size(), UiTextBlock());
        for (UiTextBlock& label : labels_) {
            if (!label.bind_label(*theme_, failed)) {
                labels_.clear();
                return false;
            }
        }
    }
    for (size_t index = 0; index < phases.size(); ++index) {
        if (!labels_[index].set(phases[index].label, failed)) {
            return false;
        }
    }
    phases_ = phases;
    current_ = current;
    return true;
}

double UiGlyphChain::at(double hour) const {
    return x_ + hour * step_;
}

void UiGlyphChain::draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiGlyphChainDay& day) {
    if (theme_ == nullptr || phases_.empty()) {
        return;
    }
    canvas.draw_set_transform_matrix(frame);
    const float y = static_cast<float>(y_);
    // L([[x(0), y], [x(30), y]], {col: --hud3}): the whole day, from the first phase's start to the last's end.
    ::godot::PackedVector2Array day_line;
    day_line.push_back(::godot::Vector2(static_cast<float>(at(phases_.front().start)), y));
    day_line.push_back(::godot::Vector2(static_cast<float>(at(phases_.back().end)), y));
    line_.draw(canvas, day_line, faint_, line_.width(), line_.drawn(day.elapsed, 0.0, day.still));
    // The hours of daylight: stroke --cut, its width, its opacity.
    canvas.draw_line(::godot::Vector2(static_cast<float>(at(day.sunrise)), y),
                     ::godot::Vector2(static_cast<float>(at(day.sunset)), y), faded(cut_, light_alpha_),
                     static_cast<float>(light_line_), true);

    for (size_t index = 0; index < phases_.size(); ++index) {
        const UiGlyphChainPhase& phase = phases_[index];
        const bool on = static_cast<int32_t>(index) == current_;
        const double middle = at((phase.start + phase.end) * GODOT_HALF);
        const ::godot::Vector2 centre(static_cast<float>(middle), y);
        canvas.draw_set_transform_matrix(frame);
        if (on) {
            ring_.disc(canvas, centre, on_radius_, time_);
        } else {
            ring_.disc(canvas, centre, off_radius_, dark_);
            ring_.circle(canvas, centre, off_radius_, soft_, line_.width(), 1.0);
        }
        UiTextPlace word;
        word.at = ::godot::Vector2(static_cast<float>(middle), static_cast<float>(label_y_));
        word.size = label_size_;
        word.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        word.color = on ? time_ : soft_;
        word.opacity = 1.0;
        word.elapsed = day.elapsed;
        word.still = day.still;
        labels_[index].draw(canvas, frame, word);
    }
    // G(x(hour), sun_y, night ? MOON : SUN, size, night ? --sky : --cut).
    const ::godot::Vector2 body(static_cast<float>(at(day.hour)), static_cast<float>(sun_y_));
    if (day.night) {
        moon_.draw(canvas, frame, body, sun_size_, sky_, 1.0);
    } else {
        sun_.draw(canvas, frame, body, sun_size_, cut_, 1.0);
    }
}

}  // namespace ase::adp::godot
