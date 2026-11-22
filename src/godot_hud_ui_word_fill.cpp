/**
 * ASE GODOT ADAPTER - HUD UI WORD FILL
 *
 * @file        godot_hud_ui_word_fill.cpp
 * @brief       The one filler of a word's port values
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.0 and 06.3, see godot_hud_ui_word_fill.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace ase::adp::godot {

namespace {

/** The value of `values` named `name`; nullptr when none is. */
const HudPortValue* value_named(const std::vector<HudPortValue>& values, const std::string& name) {
    for (const HudPortValue& value : values) {
        if (value.name == name) {
            return &value;
        }
    }
    return nullptr;
}

/**
 * `value` rounded to `decimals` places as the mockup rounds it (f1, kgf: Math.round(v * 10^n) / 10^n): a half goes
 * toward plus infinity, so 0.25 is 0.3. String::num alone rounds a half to the even digit and writes 0.2. Math.round
 * is built as floor plus one where x - floor(x) is at least 0.5, never floor(x + 0.5): that addition rounds already,
 * and 0.49999999999999994 would become 1 where the mockup writes 0 (syntheses/synt_half_rounds_up).
 */
double mockup_rounded(double value, int32_t decimals) {
    const double scale = ::godot::Math::pow(10.0, static_cast<double>(decimals));
    const double scaled = value * scale;
    const double low = ::godot::Math::floor(scaled);
    const double whole = scaled - low >= 0.5 ? low + 1.0 : low;
    return whole / scale;
}

/** Fills one text run into `out`; false with the unfilled placeholder in `failed`. */
bool fill_run(const std::string& text, const std::vector<HudPortValue>& values, std::string& out,
              std::string& failed) {
    out.clear();
    std::size_t cursor = 0;
    while (cursor < text.size()) {
        const std::size_t open = text.find('{', cursor);
        if (open == std::string::npos) {
            out.append(text, cursor, std::string::npos);
            return true;
        }
        const std::size_t close = text.find('}', open);
        if (close == std::string::npos) {
            failed = text;
            return false;
        }
        const std::string name = text.substr(open + 1, close - open - 1);
        const HudPortValue* value = value_named(values, name);
        if (value == nullptr) {
            failed = "{" + name + "}";
            return false;
        }
        out.append(text, cursor, open - cursor);
        out.append(value->text);
        cursor = close + 1;
    }
    return true;
}

}  // namespace

bool fill_words(std::vector<HudRun>& runs, const std::vector<HudPortValue>& values, std::string& failed) {
    for (HudRun& run : runs) {
        if (run.text.empty()) {
            continue;
        }
        std::string filled;
        if (!fill_run(run.text, values, filled, failed)) {
            return false;
        }
        run.text = filled;
    }
    return true;
}

bool word_text(const std::vector<HudRun>& runs, std::string& out, std::string& failed) {
    out.clear();
    for (const HudRun& run : runs) {
        if (!run.icon.empty()) {
            failed = "a word carries the glyph " + run.icon + " and cannot stand as text";
            return false;
        }
        out += run.text;
    }
    return true;
}

bool plain_word(const UiTheme& theme, const std::string& key, std::string& out, std::string& failed) {
    std::vector<HudRun> runs;
    if (!theme.words(key, runs)) {
        failed = "texts.hud." + key;
        return false;
    }
    if (!word_text(runs, out, failed)) {
        failed = "texts.hud." + key + " must be plain text: " + failed;
        return false;
    }
    return true;
}

bool word_line(const UiTheme& theme, const std::string& key, std::string& out, std::string& failed) {
    std::vector<HudRun> runs;
    if (!theme.words(key, runs)) {
        failed = "texts.hud." + key;
        return false;
    }
    out.clear();
    for (const HudRun& run : runs) {
        out += run.text;
    }
    return true;
}

bool word_icon(const UiTheme& theme, const std::string& key, std::string& icon, std::string& failed) {
    std::vector<HudRun> runs;
    if (!theme.words(key, runs)) {
        failed = "texts.hud." + key;
        return false;
    }
    if (runs.size() != 1 || runs.front().icon.empty()) {
        failed = "texts.hud." + key + " must be one icon";
        return false;
    }
    icon = runs.front().icon;
    return true;
}

bool bind_word_icon(const UiTheme& theme, const std::string& key, UiIcon& icon, std::string& failed) {
    std::string name;
    return word_icon(theme, key, name, failed) && icon.bind(theme, name, failed);
}

bool bind_phase_words(const UiTheme& theme, std::vector<std::vector<HudRun>>& out, std::string& failed) {
    out.clear();
    for (const char* key : {"phases.aso", "phases.vel", "phases.ral", "phases.cre", "phases.pex", "phases.nox"}) {
        std::vector<HudRun> word;
        if (!theme.words(key, word)) {
            failed = std::string("texts.hud.") + key;
            return false;
        }
        out.push_back(word);
    }
    if (static_cast<int32_t>(out.size()) != GODOT_PERIOD_LAST - GODOT_PERIOD_FIRST + 1) {
        failed = "texts.hud.phases must name every period of GODOT_PERIOD_FIRST .. GODOT_PERIOD_LAST";
        return false;
    }
    return true;
}

std::string hud_decimals(double value, int32_t decimals) {
    const double rounded = mockup_rounded(value, decimals);
    return std::string(::godot::String::num(rounded, decimals).pad_decimals(decimals).utf8().get_data());
}

std::string hud_rounded(double value, int32_t decimals) {
    return std::string(::godot::String::num(mockup_rounded(value, decimals), decimals).utf8().get_data());
}

std::string hud_shortest(double value) {
    // A record's own number as the mockup prints it (String(v) in JS): a whole value carries no decimal point,
    // where String::num would print 1.0 for the 1 the mockup writes ("1×", "12 h").
    if (::godot::Math::is_finite(value) && ::godot::Math::floor(value) == value) {
        return hud_whole(static_cast<int64_t>(value));
    }
    return std::string(::godot::String::num(value).utf8().get_data());
}

std::string hud_whole(int64_t value) {
    return std::string(::godot::String::num_int64(value).utf8().get_data());
}

std::string hud_padded(int64_t value, int32_t digits) {
    return std::string(::godot::String::num_int64(value).pad_zeros(digits).utf8().get_data());
}

std::string hud_percent(double share) {
    return hud_whole(static_cast<int64_t>(::godot::Math::round(share * GODOT_PERCENT)));
}

}  // namespace ase::adp::godot
