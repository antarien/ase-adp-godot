/**
 * ASE GODOT ADAPTER - VIEW SUPPORT IMPLEMENTATION
 *
 * @file        godot_view_support.cpp
 * @brief       Text conversion, font and editor check of the Vivarium view
 * @description See godot_view_support.hpp.
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

#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/variant/char_string.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

std::string utf8_of(const ::godot::String& text) {
    return std::string(text.utf8().get_data());
}

::godot::String text_of(const std::string& text) {
    return ::godot::String::utf8(text.c_str(), static_cast<int64_t>(text.size()));
}

::godot::Ref<::godot::Font> view_font() {
    return ::godot::ThemeDB::get_singleton()->get_fallback_font();
}

int32_t fitting_size(const ::godot::String& text, float width, int32_t size) {
    const ::godot::Ref<::godot::Font> font = view_font();
    while (size > GODOT_FONT_MIN &&
           font->get_string_size(text, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, size).x > width) {
        --size;
    }
    return size;
}

bool in_editor() {
    return ::godot::Engine::get_singleton()->is_editor_hint();
}

}  // namespace ase::adp::godot
