#pragma once

/**
 * ASE GODOT ADAPTER - VIEW SUPPORT
 *
 * @file        godot_view_support.hpp
 * @brief       What every part of the Vivarium view uses: text conversion, its font, the editor check
 * @description The view is one class whose behaviour stands in several files by responsibility
 *              (godot_vivarium_node.hpp). These functions are the ones more than one of those files
 *              needs: the conversion between Godot's String and the UTF-8 text of the host's ports and
 *              log, the one font the view draws with and the largest size at which a line fits, and the
 *              check that keeps the editor simulation-free (L0).
 *
 *              AN ICON IS NEVER DRAWN RAW. The view takes it from SHA's icon SSOT the way Icon.tsx does:
 *              the console bridge generate-ui-icons.cjs writes ui_icons.json (every ICON_* of
 *              icon-definitions.ts with its codepoint and the offset icon-replacer.ts gives it, in CSS
 *              pixels at web_size_px, the size Icon.tsx draws its default size with), the SHA cascade
 *              brings it and the Nerd Font Icon.tsx draws with into the client, and avt-stage stages both
 *              into res://config/. The view reads the table, loads the font at run time and draws a glyph
 *              shifted by its offset scaled to the size it draws at - the same share of the glyph the web
 *              shifts it by.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-11-06
 * @version     00.00.01.00002
 */

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

/** Godot's String as UTF-8 text. */
std::string utf8_of(const ::godot::String& text);

/** UTF-8 text as Godot's String - never through String(const char*), which reads Latin-1. */
::godot::String text_of(const std::string& text);

/** The font the view draws every text with: the theme's fallback font. */
::godot::Ref<::godot::Font> view_font();

/** The largest size from `size` down to GODOT_FONT_MIN at which `text` fits into `width`. */
int32_t fitting_size(const ::godot::String& text, float width, int32_t size);

/** True while the editor imports or edits the scene: then no host starts and no unit loads (L0). */
bool in_editor();

/**
 * The icon table at `path` (ui_icons.json of SHA's console bridge): its `icons` object, one entry per
 * ICON_* name, and web_size_px, the font size its offsets are meant for. False when the file does not
 * parse or either part is missing.
 */
bool read_icon_table(const ::godot::String& path, ::godot::Dictionary& icons, double& web_size_px);

/**
 * The glyph and the offset of the icon `name` in a table of read_icon_table. False when the table does
 * not name it or `font` does not carry its glyph - a glyph the font lacks would draw as a blank box.
 */
bool read_icon(const ::godot::Dictionary& icons, const ::godot::String& name,
               const ::godot::Ref<::godot::FontFile>& font, ::godot::String& glyph, ::godot::Vector2& offset_css);

/** The Nerd Font at `path`, loaded at run time; an invalid reference when it does not load. */
::godot::Ref<::godot::FontFile> icon_font_of(const ::godot::String& path);

/**
 * Draws `glyph` and `label` centred in `rect` at `size` in `color`: the glyph in `icon_font`, shifted
 * by `offset_css` scaled from `web_size_px` to `size` as Icon.tsx shifts it, then a space and the label
 * in the view font.
 */
void draw_icon_label(::godot::CanvasItem& canvas, const ::godot::Ref<::godot::FontFile>& icon_font,
                     const ::godot::Rect2& rect, const ::godot::String& glyph, const ::godot::Vector2& offset_css,
                     double web_size_px, const ::godot::String& label, int32_t size, const ::godot::Color& color);

}  // namespace ase::adp::godot
