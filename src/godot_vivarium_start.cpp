/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW START SCREEN
 *
 * @file        godot_vivarium_start.cpp
 * @brief       VivariumView - the start screen drawn while the host loads: title, spinner, progress bar, versions
 * @description THE SCREEN SHOWS THE LOAD, IT NEVER MAKES IT: the bar draws initialisation_progress()
 *              (godot_vivarium_clock.cpp), read from the same state follow_initialisation lifts the
 *              screen on, so it stands full in the frame the screen lifts. The spinner turns with the
 *              wall clock, so the screen moves while the host loads.
 *
 *              THE VERSIONS COME FROM THE BUILD, NEVER FROM A HAND: one line per part, the game above and
 *              the adapter under it, "<name> <version> (<Status>)". scripts/stage_native.py writes name,
 *              version and status of each part from its VERSION file into build.identity at build time,
 *              and the view reads them from there; a line whose values are missing stays empty.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/color.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/**
 * One version line of the start screen, "<name> <version> (<Status>)", read from build.identity.
 * Empty when one of the three is missing - a line is never completed by hand.
 */
::godot::String version_line(const ::godot::Dictionary& identity, const char* name_key, const char* version_key,
                             const char* status_key) {
    ::godot::String name;
    ::godot::String version;
    ::godot::String status;
    if (!read_text(identity, name_key, name) || !read_text(identity, version_key, version) ||
        !read_text(identity, status_key, status)) {
        return {};
    }
    return name + " " + version + " (" + status.capitalize() + ")";
}

}  // anonymous namespace

void VivariumView::draw_start_screen(const ::godot::Rect2& visible) {
    // Opaque over the whole surface and over the 3D scene behind the canvas: the title, a turning spinner
    // above the loading line, and under it the progress bar with its percent line. The bar shows
    // initialisation_progress(), read from the same state follow_initialisation lifts the screen on, so
    // it stands full in the frame the screen lifts. The view redraws every frame (process), and the
    // spinner turns with the wall clock, so the screen moves while the host loads.
    use_canvas();
    node_->draw_rect(visible, ::godot::Color::hex(GODOT_COLOR_BACKGROUND));
    const ::godot::Ref<::godot::Font> font = view_font();
    use_band(layout_.title_at.y);
    node_->draw_string(font, layout_.title_at, texts_.title, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f,
                       GODOT_FONT_TITLE, ::godot::Color::hex(GODOT_COLOR_TEXT));
    ::godot::Rect2 places = layout_.place_rects.front();
    for (const ::godot::Rect2& rect : layout_.place_rects) {
        places = places.merge(rect);
    }
    use_band(places.position.y);
    const float centre_x = places.get_center().x;
    const float line_y = places.get_center().y;
    // The loading line is the anchor, and every gap is measured from its glyph box, never from its
    // baseline: the line in title size rises a full ascent above the baseline and hangs a descent below
    // it, so a gap taken from the baseline lets the spinner and the bar run into the letters.
    const int32_t loading_size = fitting_size(texts_.loading, places.size.x, GODOT_FONT_TITLE);
    const float loading_top = line_y - font->get_ascent(loading_size);
    const float loading_bottom = line_y + font->get_descent(loading_size);

    // the spinner: GODOT_SPINNER_DOTS dots on a circle, the leading one full, each one behind it dimmer.
    // GODOT_SPINNER_GAP is the clear space between the lowest edge of the spinner (its radius and one dot
    // below the centre) and the top of the loading line: taken from the centre, the dots of the lower
    // half came down to 30 px over the letters and the spinner sat on the text
    const ::godot::Time* clock = ::godot::Time::get_singleton();
    const uint64_t now_ms = clock == nullptr ? 0u : clock->get_ticks_msec();
    const double turn = static_cast<double>(now_ms % GODOT_SPINNER_PERIOD_MS) /
                        static_cast<double>(GODOT_SPINNER_PERIOD_MS);
    const uint32_t leading = static_cast<uint32_t>(turn * static_cast<double>(GODOT_SPINNER_DOTS)) % GODOT_SPINNER_DOTS;
    const ::godot::Vector2 hub(centre_x, loading_top - GODOT_SPINNER_GAP - GODOT_SPINNER_RADIUS - GODOT_SPINNER_DOT);
    for (uint32_t dot = 0u; dot < GODOT_SPINNER_DOTS; ++dot) {
        const float angle = ase::math::TWO_PI * static_cast<float>(dot) / static_cast<float>(GODOT_SPINNER_DOTS);
        const ::godot::Vector2 at(hub.x + GODOT_SPINNER_RADIUS * ase::math::sin(angle),
                                  hub.y - GODOT_SPINNER_RADIUS * ase::math::cos(angle));
        const uint32_t behind = (leading + GODOT_SPINNER_DOTS - dot) % GODOT_SPINNER_DOTS;
        const float fade = 1.0f - (1.0f - GODOT_SPINNER_DIM) * static_cast<float>(behind) /
                                      static_cast<float>(GODOT_SPINNER_DOTS - 1u);
        ::godot::Color colour = ::godot::Color::hex(GODOT_COLOR_PROGRESS);
        colour.a = fade;
        node_->draw_circle(at, GODOT_SPINNER_DOT, colour);
    }

    node_->draw_string(font, ::godot::Vector2(places.position.x, line_y), texts_.loading,
                       ::godot::HORIZONTAL_ALIGNMENT_CENTER, places.size.x, loading_size,
                       ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));

    // the progress bar: GODOT_PROGRESS_GAP under the bottom of the loading line, the track over its
    // whole width, the filled share over it
    const double progress = ase::math::clamp(initialisation_progress(), 0.0, 1.0);
    const float bar_width = places.size.x * GODOT_PROGRESS_WIDTH_SHARE;
    const ::godot::Rect2 track(centre_x - bar_width * 0.5f, loading_bottom + GODOT_PROGRESS_GAP, bar_width,
                               GODOT_PROGRESS_HEIGHT);
    node_->draw_rect(track, ::godot::Color::hex(GODOT_COLOR_PROGRESS_TRACK));
    node_->draw_rect(::godot::Rect2(track.position, ::godot::Vector2(bar_width * static_cast<float>(progress),
                                                                     GODOT_PROGRESS_HEIGHT)),
                     ::godot::Color::hex(GODOT_COLOR_PROGRESS));
    const ::godot::String percent = ::godot::String::num(progress * GODOT_PERCENT, 0) + " %";
    // the percent line: its top GODOT_PROGRESS_GAP under the bar, so its baseline one ascent lower
    const float percent_y = track.get_end().y + GODOT_PROGRESS_GAP + font->get_ascent(GODOT_FONT_BODY);
    node_->draw_string(font, ::godot::Vector2(places.position.x, percent_y), percent,
                       ::godot::HORIZONTAL_ALIGNMENT_CENTER, places.size.x, GODOT_FONT_BODY,
                       ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));

    // the versions, bottom right in the margin of the build line: the game above, the adapter under it
    use_canvas();
    const ::godot::String versions[] = {
        version_line(identity_, "client_name", "client_version", "client_status"),
        version_line(identity_, "adapter_name", "adapter_version", "adapter_status")};
    const float margin = layout_.build_at.x;
    const float line_height = font->get_height(GODOT_FONT_SMALL);
    float baseline = visible.get_end().y - margin - font->get_descent(GODOT_FONT_SMALL) - line_height;
    for (const ::godot::String& line : versions) {
        if (!line.is_empty()) {
            node_->draw_string(font, ::godot::Vector2(visible.position.x + margin, baseline), line,
                               ::godot::HORIZONTAL_ALIGNMENT_RIGHT, visible.size.x - 2.0f * margin, GODOT_FONT_SMALL,
                               ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));
        }
        baseline += line_height;
    }
}

}  // namespace ase::adp::godot
