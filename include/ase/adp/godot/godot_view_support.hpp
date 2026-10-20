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
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>

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

}  // namespace ase::adp::godot
