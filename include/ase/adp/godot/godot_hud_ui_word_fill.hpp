#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI WORD FILL
 *
 * @file        godot_hud_ui_word_fill.hpp
 * @brief       The one filler of a word's port values: every {name} of a text run gets the value a block measured
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.0 and 06.3. A word of translations_hud.json is a list of
 *              HudRun (UiTheme::words); a lower-case {name} inside a text run stands for a port value
 *              ("{kg} / {cap_kg} KG", "BACKEND EM ABERTO · {id}"). Every block that draws such a word hands its
 *              runs and its values to fill_words before UiTextBlock::set, so the placeholder rule lives once.
 *
 *              NOTHING STANDS IN. A placeholder no value names refuses the whole word with that placeholder in
 *              `failed`, and so does an opening brace without its closing one: a word is never drawn with a
 *              brace standing in it, and no default text takes the place of a missing measure. Icon runs pass
 *              unchanged; the order of the values does not matter, a name may stand more than once.
 *
 *              THE ONE WRITER OF A NUMBER. A port value reaches a word as text, and every form a number takes
 *              in the HUD is written here and nowhere else: exactly N places (hud_decimals), at most N places
 *              (hud_rounded), the shortest form a record gives (hud_shortest), a whole count (hud_whole), a
 *              count padded with zeros (hud_padded) and a share as whole per cent (hud_percent). A block hands
 *              the places it read from SHA; it never formats a number of its own.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One port value of a word: the {name} it stands for and the text a block fills in. */
struct HudPortValue {
    std::string name;  // the placeholder without braces (kg, cap_kg, id, ...)
    std::string text;  // what the block measured, already written as it is shown
};

/**
 * Fills every {name} of the text runs of `runs` with its value of `values`. False with the first placeholder
 * no value names (or the run whose brace never closes) in `failed`.
 */
bool fill_words(std::vector<HudRun>& runs, const std::vector<HudPortValue>& values, std::string& failed);

/**
 * The text of the word `runs` made of text runs only, for a port value or a label. False with the reason in
 * `failed` when a run carries a glyph: a glyph cannot stand inside a value written as text.
 */
bool word_text(const std::vector<HudRun>& runs, std::string& out, std::string& failed);

/** The word texts.hud.<key> as plain text; false with the missing key, or the glyph it carries, in `failed`. */
bool plain_word(const UiTheme& theme, const std::string& key, std::string& out, std::string& failed);

/**
 * The text runs of the word texts.hud.<key> as one line, its glyphs left out: the form a word takes inside a
 * toast's port value (an item's name, a place). False with the missing key in `failed`.
 */
bool word_line(const UiTheme& theme, const std::string& key, std::string& out, std::string& failed);

/**
 * The icon name (ICON_*) of the word texts.hud.<key>, which is exactly one glyph run. False with the reason in
 * `failed` when the word is missing or is anything but one glyph.
 */
bool word_icon(const UiTheme& theme, const std::string& key, std::string& icon, std::string& failed);

/** Binds `icon` to the one glyph of the word texts.hud.<key>; false with the reason in `failed`. */
bool bind_word_icon(const UiTheme& theme, const std::string& key, UiIcon& icon, std::string& failed);

/**
 * The words texts.hud.phases.<code> in the order of GODOT_PERIOD_FIRST .. GODOT_PERIOD_LAST, one per period.
 * False with the missing key in `failed`, or when the list does not name every period.
 */
bool bind_phase_words(const UiTheme& theme, std::vector<std::vector<HudRun>>& out, std::string& failed);

/**
 * `value` written with exactly `decimals` places after the point, the form of a port value the mockup's f1 and
 * kgf print (the count comes from hud_load_decimals or hud_load_fine_decimals, never from the caller's code).
 */
std::string hud_decimals(double value, int32_t decimals);

/** `value` with at most `decimals` places after the point and no trailing zeros (the mockup's soil mm and litres). */
std::string hud_rounded(double value, int32_t decimals);

/** `value` in its shortest form, as a record or a token writes it: no trailing zeros, no fixed places. */
std::string hud_shortest(double value);

/** A whole count. */
std::string hud_whole(int64_t value);

/** A whole count padded with leading zeros to `digits` places (the clock's hours and minutes). */
std::string hud_padded(int64_t value, int32_t digits);

/** A share of 0 to 1 as whole per cent, rounded to the nearest. */
std::string hud_percent(double share);

}  // namespace ase::adp::godot
