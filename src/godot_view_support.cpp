/**
 * ASE GODOT ADAPTER - VIEW SUPPORT IMPLEMENTATION
 *
 * @file        godot_view_support.cpp
 * @brief       Text conversion, fonts, icons and editor check of the Vivarium view
 * @description See godot_view_support.hpp.
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

#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
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

bool read_icon_table(const ::godot::String& path, ::godot::Dictionary& icons, double& web_size_px) {
    const ::godot::Variant parsed = ::godot::JSON::parse_string(::godot::FileAccess::get_file_as_string(path));
    if (parsed.get_type() != ::godot::Variant::DICTIONARY) {
        return false;
    }
    const ::godot::Dictionary root = parsed;
    return read_number(root, "web_size_px", web_size_px) && web_size_px > 0.0 && read_section(root, "icons", icons);
}

bool read_icon(const ::godot::Dictionary& icons, const ::godot::String& name,
               const ::godot::Ref<::godot::FontFile>& font, ::godot::String& glyph, ::godot::Vector2& offset_css) {
    const ::godot::CharString key = name.utf8();
    ::godot::Dictionary icon;
    double codepoint = 0.0;
    if (font.is_null() || !read_section(icons, key.get_data(), icon) || !read_number(icon, "codepoint", codepoint) ||
        !(codepoint > 0.0) || codepoint > static_cast<double>(UINT32_MAX)) {
        return false;
    }
    // A codepoint is a whole number: one with a fraction does not survive the way back.
    const char32_t point = static_cast<char32_t>(codepoint);
    if (static_cast<double>(point) != codepoint || !font->has_char(point) || !read_point(icon, "offset", offset_css)) {
        return false;
    }
    glyph = ::godot::String::chr(point);
    return true;
}

::godot::Ref<::godot::FontFile> icon_font_of(const ::godot::String& path) {
    // Loaded from the file, not imported as a resource: the export carries it through its include
    // filter (godot/export_presets.cfg), so the same call reads it on Linux and in the APK.
    ::godot::Ref<::godot::FontFile> font;
    font.instantiate();
    if (font->load_dynamic_font(path) != ::godot::OK) {
        return ::godot::Ref<::godot::FontFile>();
    }
    return font;
}

void draw_icon_label(::godot::CanvasItem& canvas, const ::godot::Ref<::godot::FontFile>& icon_font,
                     const ::godot::Rect2& rect, const ::godot::String& glyph, const ::godot::Vector2& offset_css,
                     double web_size_px, const ::godot::String& label, int32_t size, const ::godot::Color& color) {
    // Glyph and label are one line, centred together; they share the baseline of the view font, as the
    // glyph of Icon.tsx stands inline beside its text. The offset is CSS pixels at web_size_px, so at
    // `size` it moves the glyph by the same share of its size.
    const ::godot::Ref<::godot::Font> font = view_font();
    const ::godot::String text = " " + label;
    const float glyph_width = icon_font->get_string_size(glyph, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, size).x;
    const float text_width = font->get_string_size(text, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, size).x;
    const float left = rect.get_center().x - (glyph_width + text_width) * GODOT_HALF;
    const float baseline = rect.get_center().y + (font->get_ascent(size) - font->get_descent(size)) * GODOT_HALF;
    const float share = static_cast<float>(static_cast<double>(size) / web_size_px);
    canvas.draw_string(icon_font, ::godot::Vector2(left + offset_css.x * share, baseline + offset_css.y * share),
                       glyph, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, size, color);
    canvas.draw_string(font, ::godot::Vector2(left + glyph_width, baseline), text,
                       ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, size, color);
}

}  // namespace ase::adp::godot
