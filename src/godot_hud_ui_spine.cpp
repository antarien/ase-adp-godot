/**
 * ASE GODOT ADAPTER - HUD UI SPINE
 *
 * @file        godot_hud_ui_spine.cpp
 * @brief       Lays down and draws the rows of the focus spine as the mockup's focus() does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_spine.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_spine.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

namespace {

/** `color` with its alpha multiplied by `opacity`. */
::godot::Color faded(const ::godot::Color& color, double opacity) {
    ::godot::Color out = color;
    out.a = static_cast<float>(static_cast<double>(color.a) * opacity);
    return out;
}

/** r[1].match(/\d/): the value carries a digit, so the mockup sets it in the data face. */
bool carries_digit(const std::string& value) {
    for (const char letter : value) {
        if (letter >= '0' && letter <= '9') {
            return true;
        }
    }
    return false;
}

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

}  // namespace

bool UiSpine::bind(const UiTheme& theme, std::string& failed) {
    if (!line_.bind(theme, failed) || !fade_.bind(theme, "hud_anim_fade", failed)) {
        return false;
    }
    if (!read_color(theme, "OVERLAY_INK", ink_, failed) || !read_color(theme, "OVERLAY_INK_SOFT", soft_, failed) ||
        !read_color(theme, "OVERLAY_INK_FAINT", faint_, failed) || !read_color(theme, "DEBUG_INK", debug_, failed)) {
        return false;
    }
    if (!read_token(theme, "hud_focus_row_y", row_y_, failed) ||
        !read_token(theme, "hud_focus_row_step", row_step_, failed) ||
        !read_token(theme, "hud_focus_row_stagger", stagger_, failed) ||
        !read_token(theme, "hud_focus_row_dot_radius", dot_radius_, failed) ||
        !read_token(theme, "hud_focus_row_dot_dy", dot_dy_, failed) ||
        !read_token(theme, "hud_focus_row_dot_delay", dot_delay_, failed) ||
        !read_token(theme, "hud_focus_row_tick_from", tick_from_, failed) ||
        !read_token(theme, "hud_focus_row_tick_to", tick_to_, failed) ||
        !read_token(theme, "hud_focus_title_dx", text_dx_, failed) ||
        !read_token(theme, "hud_focus_row_label_size", label_size_, failed) ||
        !read_token(theme, "hud_focus_row_label_delay", label_delay_, failed) ||
        !read_token(theme, "hud_focus_row_value_x", value_x_, failed) ||
        !read_token(theme, "hud_focus_row_value_size", value_size_, failed) ||
        !read_token(theme, "hud_focus_row_value_strong_size", value_strong_size_, failed) ||
        !read_token(theme, "hud_focus_row_value_delay", value_delay_, failed) ||
        !read_token(theme, "hud_focus_bar_dy", bar_dy_, failed) ||
        !read_token(theme, "hud_focus_bar_h", bar_h_, failed) ||
        !read_token(theme, "hud_focus_bar_strong_h", bar_strong_h_, failed) ||
        !read_token(theme, "hud_focus_pip_count", pip_count_, failed) ||
        !read_token(theme, "hud_focus_pip_step", pip_step_, failed) ||
        !read_token(theme, "hud_focus_pip_w", pip_w_, failed) ||
        !read_token(theme, "hud_focus_pip_h", pip_h_, failed) ||
        !read_token(theme, "hud_focus_pip_dy", pip_dy_, failed) ||
        !read_token(theme, "hud_focus_debug_dy", debug_dy_, failed) ||
        !read_token(theme, "hud_focus_debug_size", debug_size_, failed)) {
        return false;
    }
    theme_ = &theme;
    rows_.clear();
    lines_.clear();
    return true;
}

bool UiSpine::set(const std::vector<UiSpineRow>& rows, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.spine.unbound";
        return false;
    }
    // A row that is new binds its four blocks once; a row that stays keeps its shaped text.
    while (lines_.size() < rows.size()) {
        UiSpineLine line;
        if (!line.label.bind_label(*theme_, failed) || !line.number.bind_data(*theme_, failed) ||
            !line.word.bind_label(*theme_, failed) || !line.source.bind_data(*theme_, failed)) {
            return false;
        }
        lines_.push_back(line);
    }
    lines_.resize(rows.size());
    for (size_t index = 0; index < rows.size(); ++index) {
        UiSpineLine& line = lines_[index];
        line.digits = carries_digit(rows[index].value);
        UiTextBlock& value = line.digits ? line.number : line.word;
        if (!line.label.set(rows[index].label, failed) || !value.set(rows[index].value, failed) ||
            !line.source.set(rows[index].source, failed)) {
            return false;
        }
    }
    rows_ = rows;
    return true;
}

void UiSpine::draw(UiPen& canvas, const ::godot::Transform2D& frame, double column_x, double elapsed,
                   bool still, bool debug) {
    // Dots, ticks, bars and pips draw in the mockup's units: under the frame, as every text block leaves it.
    canvas.draw_set_transform_matrix(frame);
    const double text_x = column_x + text_dx_;
    const double bar_w = value_x_ - text_x;
    for (size_t index = 0; index < rows_.size(); ++index) {
        const UiSpineRow& row = rows_[index];
        UiSpineLine& line = lines_[index];
        const double step = static_cast<double>(index);
        const double y = row_y_ + step * row_step_;
        const double stagger = step * stagger_;
        const float dot_y = static_cast<float>(y + dot_dy_);

        // The dot fades in; the tick grows from its left end, as L without a delay does.
        const double dot_shown = fade_.progress(elapsed, dot_delay_ + stagger, still);
        canvas.draw_circle(::godot::Vector2(static_cast<float>(column_x), dot_y), static_cast<float>(dot_radius_),
                           faded(row.tone, dot_shown));
        ::godot::PackedVector2Array tick;
        tick.push_back(::godot::Vector2(static_cast<float>(column_x + tick_from_), dot_y));
        tick.push_back(::godot::Vector2(static_cast<float>(column_x + tick_to_), dot_y));
        line_.draw(canvas, tick, faint_, line_.width(), line_.drawn(elapsed, 0.0, still));

        UiTextPlace label;
        label.at = ::godot::Vector2(static_cast<float>(text_x), static_cast<float>(y));
        label.size = label_size_;
        label.anchor = GODOT_TEXT_ANCHOR_START;
        label.color = soft_;
        label.opacity = 1.0;
        label.fade = true;
        label.delay = label_delay_ + stagger;
        label.elapsed = elapsed;
        label.still = still;
        line.label.draw(canvas, frame, label);

        UiTextPlace value = label;
        value.at = ::godot::Vector2(static_cast<float>(value_x_), static_cast<float>(y));
        value.size = line.digits ? value_size_ : value_strong_size_;
        value.anchor = GODOT_TEXT_ANCHOR_END;
        value.color = row.alert ? row.tone : ink_;
        value.delay = value_delay_ + stagger;
        (line.digits ? line.number : line.word).draw(canvas, frame, value);

        if (row.bar) {
            const float bar_y = static_cast<float>(y + bar_dy_);
            canvas.draw_rect(::godot::Rect2(static_cast<float>(text_x), bar_y, static_cast<float>(bar_w),
                                            static_cast<float>(bar_h_)),
                             faint_);
            const double filled = bar_w * ase::math::clamp(row.share, 0.0, 1.0);
            canvas.draw_rect(::godot::Rect2(static_cast<float>(text_x), bar_y, static_cast<float>(filled),
                                            static_cast<float>(bar_strong_h_)),
                             row.tone);
        }
        if (row.pips) {
            const int32_t count = static_cast<int32_t>(pip_count_);
            for (int32_t pip = 0; pip < count; ++pip) {
                const double pip_x = text_x + static_cast<double>(pip) * pip_step_;
                canvas.draw_rect(::godot::Rect2(static_cast<float>(pip_x), static_cast<float>(y + pip_dy_),
                                                static_cast<float>(pip_w_), static_cast<float>(pip_h_)),
                                 pip < row.pips_lit ? row.tone : faint_);
            }
        }
        if (debug && !row.source.empty()) {
            UiTextPlace source;
            source.at = ::godot::Vector2(static_cast<float>(value_x_), static_cast<float>(y + debug_dy_));
            source.size = debug_size_;
            source.anchor = GODOT_TEXT_ANCHOR_END;
            source.color = debug_;
            source.opacity = 1.0;
            source.elapsed = elapsed;
            source.still = still;
            line.source.draw(canvas, frame, source);
        }
    }
}

}  // namespace ase::adp::godot
