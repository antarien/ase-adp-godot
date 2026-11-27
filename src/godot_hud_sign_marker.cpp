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
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_sign_marker.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_plant_water.hpp>
#include <ase/adp/godot/hud/sign/types.hpp>

#include <godot_cpp/variant/string.hpp>

#include <cmath>
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

void SignMarker::lay_brook(const std::vector<::godot::Vector2>& seen, float left, float right,
                           ::godot::PackedVector2Array& out) {
    if (seen.size() < 2) {
        return;
    }
    // The mockup lays its brook over the ground's width at the brook's row (m.html 361 zb, 638 sx(0.12)), so the
    // share of a sign falls at the screen's edge and its reading runs into the picture; the world's brook runs far
    // past both edges. Each step of the line is cut to the columns from left to right and the cut steps are joined
    // in their order; the whole line stands where all of it lies over the surface or none of it does.
    bool inside = true;
    for (const ::godot::Vector2& point : seen) {
        inside = inside && point.x >= left && point.x <= right;
    }
    std::vector<::godot::Vector2> kept;
    for (size_t at = 0; !inside && at + 1 < seen.size(); ++at) {
        const ::godot::Vector2 from = seen[at];
        const ::godot::Vector2 span = seen[at + 1] - from;
        float enter = 0.0f;
        float leave = 1.0f;
        if (span.x > 0.0f || span.x < 0.0f) {
            const float at_left = (left - from.x) / span.x;
            const float at_right = (right - from.x) / span.x;
            const float low = at_left < at_right ? at_left : at_right;
            const float high = at_left < at_right ? at_right : at_left;
            enter = low > enter ? low : enter;
            leave = high < leave ? high : leave;
        } else if (from.x < left || from.x > right) {
            continue;  // an upright step beside the surface
        }
        if (enter > leave) {
            continue;  // a step wholly beside the surface
        }
        const ::godot::Vector2 head = from + span * enter;
        if (kept.empty() || kept.back() != head) {
            kept.push_back(head);
        }
        kept.push_back(from + span * leave);
    }
    const std::vector<::godot::Vector2>& line = inside || kept.size() < 2 ? seen : kept;
    // The same count of points at equal steps of the line's length: SignMarker::place takes the share x at equal
    // steps between the points, so a step that the cut shortened would stretch the share there.
    float length = 0.0f;
    for (size_t at = 0; at + 1 < line.size(); ++at) {
        length += (line[at + 1] - line[at]).length();
    }
    size_t step = 0;
    float walked = 0.0f;  // the length of the line up to line[step]
    for (size_t point = 0; point < seen.size(); ++point) {
        const float target = length * static_cast<float>(point) / static_cast<float>(seen.size() - 1);
        while (step + 2 < line.size() && walked + (line[step + 1] - line[step]).length() < target) {
            walked += (line[step + 1] - line[step]).length();
            ++step;
        }
        const ::godot::Vector2 span = line[step + 1] - line[step];
        const float reach = span.length();
        const float rest = reach > 0.0f ? (target - walked) / reach : 0.0f;
        out.push_back(line[step] + span * (rest < 0.0f ? 0.0f : (rest > 1.0f ? 1.0f : rest)));
    }
}

void SignMarker::follow_brook(const VivariumStreamView& stream, const PlantMeshArrays& ground, uint64_t revision,
                              const UiAnchor& anchor, const UiGround& land, float left, float right,
                              BrookSight& kept, ::godot::PackedVector2Array& out, std::string& flank) {
    // The sign lies on the water the table draws (B0285): build_water_line gives the brook's water station by
    // station, at the level ase-hydro's depth lays over the deepest ground of each section and across the bed
    // where that water stands. The straight line between the filed ends ran over the bank of the winding bed,
    // about 35 px under the water in run 20261113 (water at y 676 to 728, the sign at 763). Only the water the
    // camera sees over the ground counts (UiGround::sees): the eye looks flat over the near bank, and the centre
    // of all the water lay under the lip, 35 to 70 px below the visible strip (run 20261113-1951-m05-rain).
    struct WaterEye {
        const UiGround* land;              // the drawn ground the line from the eye is held against
        ::godot::Vector3 eye;              // the camera's place this frame
        mutable uint32_t hidden = 0;       // the water points the ground hides
        mutable ::godot::Vector3 point;    // the last of them
        mutable ::godot::Vector3 over;     // the ground that hides it
        mutable double least = 0.0;        // the least height a hiding ground stands over the line, metres
    };
    WaterEye sight{&land, ::godot::Vector3(), 0u, ::godot::Vector3(), ::godot::Vector3(), 0.0};
    const bool eyed = anchor.eye(sight.eye);
    bool (*const sees)(const void*, float, float, float) = [](const void* context, float x, float y, float z) {
        const auto* held = static_cast<const WaterEye*>(context);
        const ::godot::Vector3 point(x, y, z);
        if (held->land->sees(held->eye, point, held->over)) {
            return true;
        }
        // the line at the hiding place: the share of the run from the eye that place stands at
        const double run = ::godot::Vector2(x - held->eye.x, z - held->eye.z).length();
        const double reach = ::godot::Vector2(held->over.x - held->eye.x, held->over.z - held->eye.z).length();
        const double line = held->eye.y + (y - held->eye.y) * (run > 0.0 ? reach / run : 0.0);
        const double above = held->over.y - line;
        held->least = held->hidden == 0u || above < held->least ? above : held->least;
        held->hidden += 1u;
        held->point = point;
        return false;
    };
    // THE STATIONS HOLD WHILE NOTHING THEY STAND ON MOVED (P0692): the vertex loop of build_water_line and the march
    // of UiGround::sees over each water point ran every frame, 16 heights a metre of sight for each of the 1716
    // hidden points of run 20261113-d2309-tri, with the ground, the brook and the eye unchanged. The ground's
    // revision, the eye and the fields of the brook's port the stations are read of decide: its line (x, z, end_x,
    // end_z), its half width (station_of), its depth (stream_levels) and its groundwater table (table_of). The flow
    // of the port, velocity_m_s, discharge_m3_s and groundwater_reach_m, lays no station: ase-hydro moves them with
    // every sample, and each change re-marched every water point (P0692). They colour the table (flow_at), never
    // the line.
    const auto level = [](double a, double b) { return a == b || (!std::isfinite(a) && !std::isfinite(b)); };
    const VivariumStreamView& held = kept.stream;
    const bool same_brook = held.object_id == stream.object_id && level(held.x, stream.x) &&
                            level(held.z, stream.z) && level(held.end_x, stream.end_x) &&
                            level(held.end_z, stream.end_z) && level(held.half_m, stream.half_m) &&
                            level(held.depth_m, stream.depth_m) &&
                            level(held.groundwater_level_m, stream.groundwater_level_m);
    if (!kept.laid || kept.ground != revision || !same_brook || kept.eyed != eyed || (eyed && kept.eye != sight.eye)) {
        kept.water.clear();
        build_water_line(stream, ground, eyed ? sees : nullptr, &sight, kept.water);
        // The bank hid all the water (run 20261113-d2309-tri: 1716 of 1716, the water 0.43 m under the lip, no
        // water colour in the picture): what the eye sees of the brook is its carved bed, and the sign lies on that.
        kept.water_stations = static_cast<uint32_t>(kept.water.size() / 3u);
        kept.bed_stations = 0u;
        if (kept.water.empty()) {
            kept.bed_stations = build_bed_line(stream, ground, eyed ? sees : nullptr, &sight, kept.water);
        }
        kept.laid = true;
        kept.ground = revision;
        kept.stream = stream;
        kept.eyed = eyed;
        kept.eye = sight.eye;
        kept.hidden = sight.hidden;
        kept.point = sight.point;
        kept.over = sight.over;
        kept.least = sight.least;
    }
    const std::vector<float>& water = kept.water;
    const uint32_t water_stations = kept.water_stations;
    const uint32_t bed_stations = kept.bed_stations;
    std::vector<::godot::Vector2> seen;
    for (size_t at = 0; at + 2 < water.size(); at += 3) {
        ::godot::Vector2 point;  // UiAnchor places in mockup units already
        if (anchor.place(::godot::Vector3(water[at], water[at + 1], water[at + 2]), point)) {
            seen.push_back(point);
        }
    }
    lay_brook(seen, left, right, out);
    // The flank of the line: the stations with water seen, the points the camera places in mockup units and the
    // columns the line is cut to, so a sign beside the water names which of the three put it there.
    flank = "hud brook line stations=" + std::to_string(water_stations) + " bed=" + std::to_string(bed_stations) +
            " seen=" + std::to_string(seen.size()) +
            " eye=" + std::to_string(static_cast<int32_t>(eyed)) + " at=" + std::to_string(sight.eye.x) + "," +
            std::to_string(sight.eye.y) + "," + std::to_string(sight.eye.z) + " hidden=" +
            std::to_string(kept.hidden) + " least=" + std::to_string(kept.least) + " last=" + std::to_string(kept.point.x) + "," +
            std::to_string(kept.point.y) + "," + std::to_string(kept.point.z) + " over=" +
            std::to_string(kept.over.x) + "," + std::to_string(kept.over.y) + "," + std::to_string(kept.over.z) +
            " left=" +
            std::to_string(static_cast<int32_t>(left)) + " right=" + std::to_string(static_cast<int32_t>(right)) +
            " points=";
    for (const ::godot::Vector2& point : seen) {
        flank += std::to_string(static_cast<int32_t>(point.x)) + "," + std::to_string(static_cast<int32_t>(point.y)) +
                 ";";
    }
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
