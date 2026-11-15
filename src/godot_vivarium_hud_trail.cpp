/**
 * ASE GODOT ADAPTER - VIVARIUM HUD TRAIL BRIDGE
 *
 * @file        godot_vivarium_hud_trail.cpp
 * @brief       The trail of past events the bridge writes into the HUD's frame while flora.trail.v1 files none
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.4, see godot_vivarium_hud_trail.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0450 (flora.trail.v1: the past events with date, domain, words and case), carrier N0448
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_hud_trail.hpp>
#include <ase/adp/godot/godot_hud_context_resolver.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

namespace ase::adp::godot {

namespace {

/** The text of `token` into `out`; false with the token in `failed` where SHA states none or an empty one. */
bool read_text(const UiTheme& theme, const std::string& token, std::string& out, std::string& failed) {
    ::godot::String text;
    if (!theme.token_text(token, text) || text.is_empty()) {
        failed = token + ": a text";
        return false;
    }
    out = std::string(text.utf8().get_data());
    return true;
}

/** The whole number of `token` from 0 into `out`; false with the token in `failed`. */
bool read_whole(const UiTheme& theme, const std::string& token, uint32_t& out, std::string& failed) {
    double value = 0.0;
    if (!theme.measure(token, value) || value < 0.0 || value != static_cast<double>(static_cast<uint32_t>(value))) {
        failed = token + ": a whole number from 0";
        return false;
    }
    out = static_cast<uint32_t>(value);
    return true;
}

}  // namespace

bool VivariumTrailBridge::load_value(const UiTheme& theme, const std::string& prefix, uint32_t value,
                                     HudPortValue& out, std::string& failed) const {
    const std::string key = prefix + "_value_" + std::to_string(value + 1);
    if (!read_text(theme, key + "_name", out.name, failed)) {
        return false;
    }
    // A word of texts.hud, written in the language of the view, or a number with its places.
    std::string word;
    ::godot::String probe;
    if (theme.token_text(key + "_word", probe)) {
        if (!read_text(theme, key + "_word", word, failed)) {
            return false;
        }
        return plain_word(theme, word, out.text, failed);
    }
    double number = 0.0;
    uint32_t decimals = 0;
    if (!theme.measure(key + "_number", number)) {
        failed = key + "_number or " + key + "_word: one of the two";
        return false;
    }
    if (!read_whole(theme, key + "_decimals", decimals, failed)) {
        return false;
    }
    out.text = hud_decimals(number, static_cast<int32_t>(decimals));
    return true;
}

bool VivariumTrailBridge::load_event(const UiTheme& theme, uint32_t index, TrailEvent& out, double& share,
                                     std::string& failed) const {
    const std::string prefix = "hud_bridge_trail_" + std::to_string(index + 1);
    if (!read_whole(theme, prefix + "_lun", out.lun, failed) || !read_whole(theme, prefix + "_tum", out.tum, failed)) {
        return false;
    }
    // The share of the year as the port files it: ase-calendar places the day, the adapter reckons no date.
    if (!theme.measure(prefix + "_year_share", share) || share < 0.0 || share > 1.0) {
        failed = prefix + "_year_share: a share 0..1";
        return false;
    }
    if (!read_text(theme, prefix + "_kind", out.kind, failed) || !read_text(theme, prefix + "_tone", out.tone, failed) ||
        !read_text(theme, prefix + "_case", out.case_id, failed)) {
        return false;
    }
    // The tone and the words of the kind must stand in the theme, so the view never meets a gap.
    ::godot::Color tone;
    if (!theme.tone(out.tone, tone)) {
        failed = prefix + "_tone: a domain tone of the theme";
        return false;
    }
    std::vector<HudRun> runs;
    if (!theme.words("trail." + out.kind + ".title", runs) || !theme.words("trail." + out.kind + ".line", runs)) {
        failed = prefix + "_kind: texts.hud.trail." + out.kind + ".title and .line";
        return false;
    }
    uint32_t alert = 0;
    if (!read_whole(theme, prefix + "_alert", alert, failed) || alert > 1) {
        failed = prefix + "_alert: 0 or 1";
        return false;
    }
    out.alert = alert == 1;
    // The sign it opens: a key of hud.signs, or rest.
    std::string sign;
    if (!read_text(theme, prefix + "_sign", sign, failed)) {
        return false;
    }
    out.sign = UINT32_MAX;
    if (sign != "rest") {
        const std::vector<HudSignSpec>& signs = theme.spec().signs;
        for (uint32_t at = 0; at < signs.size(); ++at) {
            if (signs[at].name == sign) {
                out.sign = at;
                break;
            }
        }
        if (out.sign == UINT32_MAX) {
            failed = prefix + "_sign: a key of hud.signs or rest";
            return false;
        }
    }
    uint32_t values = 0;
    if (!read_whole(theme, prefix + "_values", values, failed)) {
        return false;
    }
    out.values.clear();
    out.values.resize(values);
    for (uint32_t value = 0; value < values; ++value) {
        if (!load_value(theme, prefix, value, out.values[value], failed)) {
            return false;
        }
    }
    return true;
}

bool VivariumTrailBridge::load(const UiTheme& theme, std::string& failed) {
    uint32_t count = 0;
    if (!read_whole(theme, "hud_bridge_trail_count", count, failed) ||
        !read_whole(theme, "hud_bridge_trail_days", days_, failed) ||
        !read_whole(theme, "hud_bridge_trail_lun", lun_, failed) ||
        !read_whole(theme, "hud_bridge_trail_from", from_, failed) ||
        !read_whole(theme, "hud_bridge_trail_to", to_, failed)) {
        return false;
    }
    events_.clear();
    shares_.clear();
    events_.resize(count);
    shares_.resize(count);
    for (uint32_t index = 0; index < count; ++index) {
        if (!load_event(theme, index, events_[index], shares_[index], failed)) {
            return false;
        }
    }
    return true;
}

void VivariumTrailBridge::trail(TrailViewFrame& trail) const {
    trail.events = events_;
    trail.days = days_;
    trail.lun = lun_;
    trail.from = from_;
    trail.to = to_;
}

void VivariumTrailBridge::ring(YearRingFrame& year) const {
    year.events.clear();
    for (uint32_t index = 0; index < events_.size(); ++index) {
        year.events.push_back(YearRingEvent{shares_[index], events_[index].tone});
    }
}

}  // namespace ase::adp::godot
