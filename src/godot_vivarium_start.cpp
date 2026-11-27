/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW START SCREEN
 *
 * @file        godot_vivarium_start.cpp
 * @brief       VivariumView - the start screen: the choice how the world starts, then title, spinner, bar, versions
 * @description THE SCREEN HOLDS THE WORLD UNTIL THE KEEPER CHOSE (operator order of 2026-11-19): while
 *              start_mode_ is GODOT_START_MODE_NONE no host runs, and the screen shows the choice of
 *              VivariumStartChoice where the loading line stands later, in the HUD's own form (hex chips of
 *              UiHexChip under the HUD's frame, words, glyphs and measures from SHA). The start chip calls
 *              begin_host with the chosen mode, and boot_host hands OFFLINE_INITIAL to the host as
 *              VivariumStartConfig::start_fresh. A launch that takes no taps (self test, measurement, asset
 *              showcase) boots at once on the switches at rest: offline, the last kept state.
 *
 *              THE SCREEN SHOWS THE LOAD, IT NEVER MAKES IT: the bar draws initialisation_progress()
 *              (godot_vivarium_clock.cpp), read from the same state follow_initialisation lifts the
 *              screen on, so it stands full in the frame the screen lifts. The spinner turns with the
 *              wall clock, so the screen moves while the host loads.
 *
 *              THE VERSIONS COME FROM THE BUILD, NEVER FROM A HAND: one line per part, the game above and
 *              the adapter under it, "<name> <version> [<status>]". scripts/stage_native.py writes name,
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
 * @modified    2026-11-21
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_vivarium_start_choice.hpp>
#include <ase/adp/godot/godot_vivarium_start_mode.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/color.hpp>

#include <cstdint>
#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/**
 * One version line of the start screen, "<name> <version> [<status>]", read from build.identity - the house
 * form of a version tag, the status lower case in square brackets.
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
    return name + " " + version + " [" + status + "]";
}

/** One chip of the choice: its glyph start_glyph.<key> over its word start.<key>, both words of SHA. */
bool bind_start_chip(const UiTheme& theme, const char* key, UiHexChip& chip, std::string& failed) {
    std::string label;
    std::string icon;
    const std::string stem = key;
    return chip.bind(theme, failed) && plain_word(theme, "start." + stem, label, failed) &&
           word_icon(theme, "start_glyph." + stem, icon, failed) && chip.set_glyph(icon, label, failed);
}

/** The domain tone a token of SHA names: hud_start_* holds the code, DOMAIN_<code> the colour. */
bool bind_start_tone(const UiTheme& theme, const char* token, ::godot::Color& out, std::string& failed) {
    ::godot::String code;
    if (!theme.token_text(token, code)) {
        failed = token;
        return false;
    }
    const std::string name = code.utf8().get_data();
    if (!theme.tone(name, out)) {
        failed = "DOMAIN_" + name;
        return false;
    }
    return true;
}

}  // anonymous namespace

// THE CHOICE

bool VivariumStartChoice::bind(const UiTheme& theme, std::string& failed) {
    placed_ = false;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_start_chip_radius", &radius_},
             {"hud_start_chip_spread", &spread_},
             {"hud_start_chip_row_gap", &row_gap_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    return bind_start_tone(theme, "hud_start_chip_tone", tone_, failed) &&
           bind_start_tone(theme, "hud_start_begin_tone", begin_tone_, failed) &&
           bind_start_chip(theme, "online", online_, failed) && bind_start_chip(theme, "offline", offline_, failed) &&
           bind_start_chip(theme, "initial", initial_, failed) && bind_start_chip(theme, "resume", resume_, failed) &&
           bind_start_chip(theme, "begin", begin_, failed);
}

void VivariumStartChoice::draw(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& anchor,
                               double elapsed, bool still) {
    // Every chip stands at once (no fade): the screen waits for a tap, nothing enters it. The chosen side of a
    // row is lit in the switches' tone, the other side rests in the same tone unlit, as the fan's chips do.
    const double half = spread_ * 0.5;
    const auto place = [&](UiHexChipPlace& at, double x, double y, const ::godot::Color& tone, bool on) {
        at.centre = ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
        at.radius = radius_;
        at.tone = tone;
        at.on = on;
        at.fade = false;
        at.delay = 0.0;
        at.elapsed = elapsed;
        at.still = still;
    };
    const double x = static_cast<double>(anchor.x);
    double y = static_cast<double>(anchor.y);
    place(online_at_, x - half, y, tone_, !offline_chosen_);
    place(offline_at_, x + half, y, tone_, offline_chosen_);
    online_.draw(canvas, frame, online_at_);
    offline_.draw(canvas, frame, offline_at_);
    if (offline_chosen_) {
        y += row_gap_;
        place(initial_at_, x - half, y, tone_, !resume_chosen_);
        place(resume_at_, x + half, y, tone_, resume_chosen_);
        initial_.draw(canvas, frame, initial_at_);
        resume_.draw(canvas, frame, resume_at_);
    }
    y += row_gap_;
    place(begin_at_, x, y, begin_tone_, true);
    begin_.draw(canvas, frame, begin_at_);
    placed_ = true;
}

uint8_t VivariumStartChoice::tap(const ::godot::Vector2& point) {
    if (!placed_) {
        return GODOT_START_MODE_NONE;
    }
    if (online_.hit(online_at_, point)) {
        offline_chosen_ = false;
    } else if (offline_.hit(offline_at_, point)) {
        offline_chosen_ = true;
    } else if (offline_chosen_ && initial_.hit(initial_at_, point)) {
        resume_chosen_ = false;
    } else if (offline_chosen_ && resume_.hit(resume_at_, point)) {
        resume_chosen_ = true;
    } else if (begin_.hit(begin_at_, point)) {
        return chosen();
    }
    return GODOT_START_MODE_NONE;
}

bool VivariumStartChoice::take_begin_place(const ::godot::Transform2D& frame, ::godot::Vector2& centre,
                                           double& radius) {
    if (!placed_) {
        return false;
    }
    // the frame scales uniformly (draw_start_screen: k on both axes), so one axis carries the radius
    const ::godot::Vector2 at = frame.xform(begin_at_.centre);
    const double across = begin_at_.radius * static_cast<double>(frame.get_scale().x);
    if (begin_announced_once_ && begin_announced_ == at && begin_announced_radius_ == across) {
        return false;
    }
    begin_announced_once_ = true;
    begin_announced_ = at;
    begin_announced_radius_ = across;
    centre = at;
    radius = across;
    return true;
}

bool VivariumStartChoice::initial_place(::godot::Vector2& centre, double& radius) const {
    // the row NOVO / RETOMAR is drawn only under OFFLINE (draw), so only then has initial_at_ a place
    if (!placed_ || !offline_chosen_) {
        return false;
    }
    centre = initial_at_.centre;
    radius = initial_at_.radius;
    return true;
}

uint8_t VivariumStartChoice::chosen() const {
    if (!offline_chosen_) {
        return GODOT_START_MODE_ONLINE;
    }
    return resume_chosen_ ? GODOT_START_MODE_OFFLINE_RESUME : GODOT_START_MODE_OFFLINE_INITIAL;
}

// THE VIEW

void VivariumView::begin_host(uint8_t mode) {
    start_mode_ = mode;
    note(::godot::String("start mode ") + ::godot::String::num_int64(static_cast<int64_t>(mode)));
    boot_host(true);
}

bool VivariumView::start_input(const ::godot::Ref<::godot::InputEvent>& local) {
    // ONE PATH PER PHYSICAL TAP, as on the care view: a touch as ScreenTouch, a mouse as MouseButton.
    const ::godot::Ref<::godot::InputEventScreenTouch> touch = local;
    const ::godot::Ref<::godot::InputEventMouseButton> mouse = local;
    ::godot::Vector2 at;
    if (touch.is_valid() && touch->is_pressed()) {
        at = touch->get_position();
    } else if (mouse.is_valid() && mouse->is_pressed() && mouse->get_button_index() == ::godot::MOUSE_BUTTON_LEFT) {
        at = mouse->get_position();
    } else {
        return false;
    }
    node_->get_viewport()->set_input_as_handled();
    // The chips stand in mockup units under start_frame_, the frame the last start screen drew them with.
    const uint8_t mode = start_choice_.tap(start_frame_.affine_inverse().xform(at));
    // EVERY TAP OF THE START SCREEN IS NAMED, a miss as well (mode 0): a run that taps the start chip tells a tap
    // that never arrived from one that landed beside the chip by this line, at the logical place it arrived.
    note(::godot::String("start tap x=") + ::godot::String::num(at.x, 1) + " y=" + ::godot::String::num(at.y, 1) +
         " mode=" + ::godot::String::num_int64(static_cast<int64_t>(mode)));
    if (mode != GODOT_START_MODE_NONE) {
        begin_host(mode);
    }
    node_->queue_redraw();
    return true;
}

void VivariumView::draw_start_screen(const ::godot::Rect2& visible) {
    // Opaque over the whole surface and over the 3D scene behind the canvas: the title, then either the choice
    // how the world starts (no host runs yet) or a turning spinner above the loading line and under it the
    // progress bar with its percent line. The bar shows initialisation_progress(), read from the same state
    // follow_initialisation lifts the screen on, so it stands full in the frame the screen lifts. The view
    // redraws every frame (process), and the spinner turns with the wall clock, so the screen moves while the
    // host loads.
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

    if (start_mode_ == GODOT_START_MODE_NONE) {
        // THE CHOICE stands where the loading line stands later, under the HUD's frame (place_hud): one mockup
        // unit is placement_.scale * hud_state_.hud_unit_ logical pixels on the middle band. Its anchor is the loading
        // line's centre on the canvas, carried into mockup units, the first row one row gap above it, so the
        // rows stand around the line.
        use_canvas();
        const float k = static_cast<float>(static_cast<double>(placement_.scale) * hud_state_.hud_unit_);
        start_frame_ = ::godot::Transform2D(0.0f, ::godot::Vector2(k, k), 0.0f,
                                            ::godot::Vector2(placement_.offset_x, placement_.middle_y));
        if (k > 0.0f) {
            const ::godot::Vector2 line(placement_.offset_x + centre_x * placement_.scale,
                                        band_offset(layout_.bands, placement_, places.position.y) +
                                            line_y * placement_.scale);
            ::godot::Vector2 anchor = start_frame_.affine_inverse().xform(line);
            UiPen pen(*node_);
            start_choice_.draw(pen, start_frame_, anchor, hud_state_.hud_clock_, hud_state_.hud_still_);
            // The start chip on the canvas, in the logical pixels a tap arrives in (start_input carries it back
            // under start_frame_): named once per new place, so a run that starts the world by tapping it, as the
            // keeper does, reads where it stands instead of guessing it (usecase:100387).
            ::godot::Vector2 at;
            double begin_radius = 0.0;
            if (start_choice_.take_begin_place(start_frame_, at, begin_radius)) {
                note(::godot::String("start choice begin x=") + ::godot::String::num(at.x, 1) + " y=" +
                     ::godot::String::num(at.y, 1) + " r=" + ::godot::String::num(begin_radius, 1));
                // the chip NOVO moves only with the start chip (one row above it, VivariumStartChoice::draw), so
                // its place is named beside each new place of the start chip: a run that starts OFFLINE NOVO
                // taps it first, as the keeper does (ST04 of the task inventory)
                ::godot::Vector2 initial_centre;
                double initial_radius = 0.0;
                if (start_choice_.initial_place(initial_centre, initial_radius)) {
                    const ::godot::Vector2 initial_at = start_frame_.xform(initial_centre);
                    note(::godot::String("start choice initial x=") + ::godot::String::num(initial_at.x, 1) +
                         " y=" + ::godot::String::num(initial_at.y, 1) + " r=" +
                         ::godot::String::num(initial_radius * k, 1));
                }
            }
        }
    } else {
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
        const uint32_t leading =
            static_cast<uint32_t>(turn * static_cast<double>(GODOT_SPINNER_DOTS)) % GODOT_SPINNER_DOTS;
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
        // the percent line names the step that holds the screen behind the separator (B0357), the word
        // follow_initialisation chose in the same frame; before its first frame the line stands alone
        ::godot::String percent = ::godot::String::num(progress * GODOT_PERCENT, 0) + " %";
        if (!start_hold_.hold_word.is_empty()) {
            percent += texts_.separator + start_hold_.hold_word;
        }
        // the percent line: its top GODOT_PROGRESS_GAP under the bar, so its baseline one ascent lower
        const float percent_y = track.get_end().y + GODOT_PROGRESS_GAP + font->get_ascent(GODOT_FONT_BODY);
        node_->draw_string(font, ::godot::Vector2(places.position.x, percent_y), percent,
                           ::godot::HORIZONTAL_ALIGNMENT_CENTER, places.size.x, GODOT_FONT_BODY,
                           ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));
    }

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
