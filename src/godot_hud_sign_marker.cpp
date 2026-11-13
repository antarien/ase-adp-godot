/**
 * ASE GODOT ADAPTER - HUD SIGN MARKER
 *
 * @file        godot_hud_sign_marker.cpp
 * @brief       The glyph of each sign at its place in the world; the open one hands its reading to SignReading
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block SignMarker, see godot_hud_sign_marker.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    ecs/module
 * @usecase     usecase:100370 - wilt from drought
 * @actor       Hänge-Birke (Betula pendula)
 * @register    N0461, N0449, carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.01.00003
 */

#include <ase/adp/godot/godot_hud_sign_marker.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool SignMarker::bind_sign(const UiTheme& theme, const HudSignSpec& spec, Sign& out, std::string& failed) const {
    const std::string stem = "hud_sign_" + spec.name;
    ::godot::String text;
    if (!theme.token_text(stem + "_tone", text)) {
        failed = stem + "_tone";
        return false;
    }
    const std::string code = text.utf8().get_data();
    if (!theme.tone(code, out.tone)) {
        failed = "DOMAIN_" + code;
        return false;
    }
    if (!theme.token_text(stem + "_at", text)) {
        failed = stem + "_at";
        return false;
    }
    out.at = text.utf8().get_data();
    if (out.at != GODOT_HUD_SIGN_AT_CROWN && out.at != GODOT_HUD_SIGN_AT_TRUNK && out.at != GODOT_HUD_SIGN_AT_BROOK &&
        out.at != GODOT_HUD_SIGN_AT_SCREEN) {
        failed = stem + "_at: " + out.at + " is none of crown, trunk, brook, screen";
        return false;
    }
    if (!theme.measure(stem + "_x", out.x)) {
        failed = stem + "_x";
        return false;
    }
    if (!theme.measure(stem + "_y", out.y)) {
        failed = stem + "_y";
        return false;
    }
    return bind_word_icon(theme, "signs." + spec.name + ".glyph", out.glyph, failed);
}

bool SignMarker::bind(const UiTheme& theme, std::string& failed) {
    drawn_.clear();
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_sign_radius", &radius_},
             {"hud_sign_glyph_size", &glyph_size_},
             {"hud_sign_line", &line_w_},
             {"hud_sign_on_line", &on_line_},
             {"hud_sign_pulse_radius", &pulse_radius_},
             {"hud_sign_hit_radius", &hit_radius_},
             {"hud_sign_stagger", &stagger_},
             {"hud_sign_crown_min_h", &crown_min_h_},
             {"hud_sign_crown_spread", &crown_spread_},
             {"hud_line", &line_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!theme.color("OVERLAY_DISC", disc_)) {
        failed = "OVERLAY_DISC";
        return false;
    }

    const HudContextSpec& spec = theme.spec();
    signs_.clear();
    for (const HudSignSpec& sign : spec.signs) {
        Sign made;
        if (!bind_sign(theme, sign, made, failed)) {
            return false;
        }
        signs_.push_back(made);
    }
    if (spec.levels.empty()) {
        failed = "hud.perception.levels";
        return false;
    }
    level_words_ = spec.levels[0];

    return fade_.bind(theme, "hud_anim_fade", failed) && ring_.bind(theme, failed) && reading_.bind(theme, failed);
}

bool SignMarker::place(const Sign& sign, const SignMarkerFrame& now, ::godot::Vector2& out) const {
    if (sign.at == GODOT_HUD_SIGN_AT_SCREEN) {
        out = ::godot::Vector2(static_cast<float>(sign.x), static_cast<float>(sign.y));
        return true;
    }
    if (sign.at == GODOT_HUD_SIGN_AT_BROOK) {
        // The share x along the brook's line, whose points stand at equal steps of the share.
        const int64_t points = now.brook.size();
        if (points < 2) {
            return false;
        }
        const double along = sign.x * static_cast<double>(points - 1);
        int64_t index = static_cast<int64_t>(along);
        index = index < 0 ? 0 : (index > points - 2 ? points - 2 : index);
        const double rest = along - static_cast<double>(index);
        const ::godot::Vector2 from = now.brook[index];
        const ::godot::Vector2 to = now.brook[index + 1];
        out = from + (to - from) * static_cast<float>(rest) + ::godot::Vector2(0.0f, static_cast<float>(sign.y));
        return true;
    }
    if (!now.trunk_placed) {
        return false;
    }
    if (sign.at == GODOT_HUD_SIGN_AT_TRUNK) {
        out = now.trunk + ::godot::Vector2(static_cast<float>(sign.x), static_cast<float>(sign.y));
        return true;
    }
    // crownPt(x, y): H = max(treeH(), hud_sign_crown_min_h), (TX + x * H * spread, TY() - H * y).
    const double high = now.height > crown_min_h_ ? now.height : crown_min_h_;
    out = now.trunk + ::godot::Vector2(static_cast<float>(sign.x * high * crown_spread_),
                                       static_cast<float>(-high * sign.y));
    return true;
}

void SignMarker::draw_marker(UiPen& canvas, const ::godot::Transform2D& frame,
                             const SignMarkerFrame& now, const HudSignShown& shown, size_t order,
                             const ::godot::Vector2& centre) {
    const Sign& sign = signs_[shown.sign];
    const double shown_share = fade_.progress(now.elapsed, static_cast<double>(order) * stagger_, now.still);
    ::godot::Color tone = sign.tone;
    tone.a *= static_cast<float>(shown_share);
    ::godot::Color disc = disc_;
    disc.a *= static_cast<float>(shown_share);

    canvas.draw_set_transform_matrix(frame);
    if ((shown.alert || now.shown.level == level_words_) && !shown.open) {
        ring_.pulse(canvas, centre, pulse_radius_, tone, line_, now.elapsed, now.still);
    }
    ring_.disc(canvas, centre, radius_, disc);
    ring_.circle(canvas, centre, radius_, sign.tone, shown.open ? on_line_ : line_w_, shown_share);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    sign.glyph.draw(canvas, frame, centre, glyph_size_, sign.tone, shown_share);
    drawn_.push_back(Drawn{shown.sign, centre});
}

bool SignMarker::draw(UiPen& canvas, const ::godot::Transform2D& frame, const SignMarkerFrame& now,
                      std::string& failed) {
    drawn_.clear();

    // The markers, the open one's reading over all of them.
    const HudSignShown* open = nullptr;
    ::godot::Vector2 open_at;
    size_t order = 0;
    for (const HudSignShown& shown : now.shown.signs) {
        if (shown.sign >= signs_.size()) {
            failed = "SignMarker: sign " + std::to_string(shown.sign) + " outside hud.signs";
            return false;
        }
        ::godot::Vector2 centre;
        if (!place(signs_[shown.sign], now, centre)) {
            continue;
        }
        draw_marker(canvas, frame, now, shown, order++, centre);
        if (shown.open) {
            open = &shown;
            open_at = centre;
        }
    }
    if (open == nullptr) {
        return true;
    }
    SignReadingFrame reading;
    reading.shown = *open;
    reading.centre = open_at;
    reading.level = now.shown.level;
    reading.values = &now.values;
    reading.places = &now.places;
    reading.opened = now.opened;
    reading.still = now.still;
    return reading_.draw(canvas, frame, reading, failed);
}

bool SignMarker::tone(uint32_t sign, ::godot::Color& out) const {
    if (sign >= signs_.size()) {
        return false;
    }
    out = signs_[sign].tone;
    return true;
}

bool SignMarker::hit(const ::godot::Vector2& point, SignMarkerHit& out) const {
    out = SignMarkerHit{};
    uint32_t tool = UINT32_MAX;
    if (reading_.hit(point, tool)) {
        out.offer = tool;
        return true;
    }
    // The topmost marker answers first, as in the artifact's SVG (m.html 662): a later group lies over an earlier
    // one, so where two hit circles overlap (flush and shed share 0.55/0.8) the tap opens the one drawn last.
    for (auto marker = drawn_.rbegin(); marker != drawn_.rend(); ++marker) {
        if (marker->centre.distance_to(point) <= static_cast<float>(hit_radius_)) {
            out.sign = marker->sign;
            return true;
        }
    }
    return false;
}

void SignMarker::places(std::vector<SignMarkerPlace>& out) const {
    out.clear();
    const float reach = static_cast<float>(hit_radius_);
    for (const Drawn& marker : drawn_) {
        out.push_back(SignMarkerPlace{marker.sign, ::godot::Rect2(marker.centre - ::godot::Vector2(reach, reach),
                                                                  ::godot::Vector2(reach, reach) * 2.0f)});
    }
}

}  // namespace ase::adp::godot
