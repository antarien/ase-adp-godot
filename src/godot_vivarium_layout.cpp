/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW LAYOUT CONFIGURATION
 *
 * @file        godot_vivarium_layout.cpp
 * @brief       VivariumView - the logical surface, the bands, the places, the buttons and the texts of the start configuration
 * @description PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3: where things stand on the logical surface is data of
 *              res://config/vivarium_start.json, never a number of the code. Every rectangle and every
 *              text line lies inside ONE of the three bands (godot_view_layout.hpp); load_bands refuses a
 *              configuration where one crosses a band edge, because the bands move apart on a taller
 *              surface. The texts are Brazilian Portuguese, read from the same configuration.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/variant/array.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

bool VivariumView::load_places(const ::godot::Dictionary& root) {
    // The patches are the plugin's - it creates them and names each one's place. The view only
    // knows where a place is drawn: one rectangle per place, the places 0..n-1 each once.
    const ::godot::Variant places_value = root.get("places", ::godot::Variant());
    if (places_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("places");
    }
    const ::godot::Array places = places_value;
    layout_.place_rects.assign(static_cast<std::size_t>(places.size()), ::godot::Rect2());
    std::vector<bool> seen(static_cast<std::size_t>(places.size()), false);
    for (int64_t i = 0; i < places.size(); ++i) {
        if (places[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("places[]");
        }
        const ::godot::Dictionary entry = places[i];
        double place = 0.0;
        ::godot::Rect2 rect;
        if (!read_number(entry, "place", place) || place < 0.0 ||
            place >= static_cast<double>(places.size()) ||
            ase::math::floor(static_cast<float>(place)) != static_cast<float>(place) ||
            seen[static_cast<std::size_t>(place)]) {
            return fail_config("places[].place");
        }
        if (!read_rect(entry, "rect", rect)) {
            return fail_config("places[].rect");
        }
        seen[static_cast<std::size_t>(place)] = true;
        layout_.place_rects[static_cast<std::size_t>(place)] = rect;
    }
    if (layout_.place_rects.empty()) {
        return fail_config("places");
    }
    return true;
}

bool VivariumView::load_layout(const ::godot::Dictionary& root) {
    double number = 0.0;
    ::godot::Dictionary size;
    if (!read_section(root, "logical_size", size)) {
        return fail_config("logical_size");
    }
    if (!read_number(size, "width", number)) {
        return fail_config("logical_size.width");
    }
    layout_.size.x = static_cast<float>(number);
    if (!read_number(size, "height", number)) {
        return fail_config("logical_size.height");
    }
    layout_.size.y = static_cast<float>(number);

    ::godot::Dictionary layout;
    if (!read_section(root, "layout", layout)) {
        return fail_config("layout");
    }
    if (!read_number(layout, "header_height", number)) {
        return fail_config("layout.header_height");
    }
    layout_.header_height = static_cast<float>(number);
    if (!read_point(layout, "title", layout_.title_at)) {
        return fail_config("layout.title");
    }
    if (!read_point(layout, "build", layout_.build_at)) {
        return fail_config("layout.build");
    }
    if (!read_point(layout, "info", layout_.info_at)) {
        return fail_config("layout.info");
    }
    if (!read_number(layout, "info_line", number)) {
        return fail_config("layout.info_line");
    }
    layout_.info_line = static_cast<float>(number);
    if (!read_point(layout, "note", layout_.note_at)) {
        return fail_config("layout.note");
    }
    if (!load_places(root)) {
        return false;
    }

    ::godot::Dictionary buttons;
    ::godot::Dictionary button;
    if (!read_section(root, "buttons", buttons)) {
        return fail_config("buttons");
    }
    if (!read_section(buttons, "water", button) || !read_rect(button, "rect", layout_.water_rect) ||
        !read_text(button, "label", texts_.water)) {
        return fail_config("buttons.water");
    }
    if (!read_section(buttons, "prune", button) || !read_rect(button, "rect", layout_.prune_rect) ||
        !read_text(button, "label", texts_.prune)) {
        return fail_config("buttons.prune");
    }
    if (!read_section(buttons, "reset", button) || !read_rect(button, "rect", layout_.reset_rect) ||
        !read_text(button, "label", texts_.reset)) {
        return fail_config("buttons.reset");
    }
    // The bands come last: they are checked against every element read above.
    return load_bands(layout);
}

bool VivariumView::load_bands(const ::godot::Dictionary& layout) {
    ::godot::Dictionary bands;
    double top_end = 0.0;
    double bottom_start = 0.0;
    if (!read_section(layout, "bands", bands) || !read_number(bands, "top_end", top_end) ||
        !read_number(bands, "bottom_start", bottom_start)) {
        return fail_config("layout.bands");
    }
    ViewBands& placed_bands = layout_.bands;
    placed_bands.width = layout_.size.x;
    placed_bands.height = layout_.size.y;
    placed_bands.top_end = static_cast<float>(top_end);
    placed_bands.bottom_start = static_cast<float>(bottom_start);
    if (!(placed_bands.width > 0.0f) || !(placed_bands.top_end > 0.0f) ||
        !(placed_bands.top_end <= placed_bands.bottom_start) || !(placed_bands.bottom_start < placed_bands.height)) {
        return fail_config("layout.bands");
    }
    // EVERY ELEMENT IN ONE BAND: on a taller surface the bands move apart, and an element across
    // an edge would be drawn in two pieces. A text line belongs to the band of its baseline; the
    // info block is three lines, so its third baseline is checked with the first.
    if (!in_one_band(placed_bands, 0.0f, layout_.header_height)) {
        return fail_config("layout.bands: header_height");
    }
    if (!in_one_band(placed_bands, layout_.title_at.y, layout_.title_at.y)) {
        return fail_config("layout.bands: title");
    }
    if (!in_one_band(placed_bands, layout_.build_at.y, layout_.build_at.y)) {
        return fail_config("layout.bands: build");
    }
    if (!in_one_band(placed_bands, layout_.info_at.y, layout_.info_at.y + 2.0f * layout_.info_line)) {
        return fail_config("layout.bands: info");
    }
    if (!in_one_band(placed_bands, layout_.note_at.y, layout_.note_at.y)) {
        return fail_config("layout.bands: note");
    }
    for (const ::godot::Rect2& rect : layout_.place_rects) {
        if (!in_one_band(placed_bands, rect.position.y, rect.get_end().y)) {
            return fail_config("layout.bands: places[].rect");
        }
    }
    if (!in_one_band(placed_bands, layout_.water_rect.position.y, layout_.water_rect.get_end().y)) {
        return fail_config("layout.bands: buttons.water");
    }
    if (!in_one_band(placed_bands, layout_.prune_rect.position.y, layout_.prune_rect.get_end().y)) {
        return fail_config("layout.bands: buttons.prune");
    }
    if (!in_one_band(placed_bands, layout_.reset_rect.position.y, layout_.reset_rect.get_end().y)) {
        return fail_config("layout.bands: buttons.reset");
    }
    return true;
}

bool VivariumView::load_texts(const ::godot::Dictionary& root) {
    ::godot::Dictionary texts;
    if (!read_section(root, "texts", texts)) {
        return fail_config("texts");
    }
    struct {
        const char*      key;
        ::godot::String* out;
    } const wanted[] = {
        {"title", &texts_.title},
        {"selection", &texts_.selection},
        {"selection_none", &texts_.selection_none},
        {"area", &texts_.area},
        {"simulation", &texts_.simulation},
        {"ticks", &texts_.ticks},
        {"day", &texts_.day},
        {"hour", &texts_.hour},
        {"coverage", &texts_.coverage},
        {"soil", &texts_.soil},
        {"stage_seed", &texts_.stage_seed},
        {"stage_sprout", &texts_.stage_sprout},
        {"stage_grow", &texts_.stage_grow},
        {"stage_mature", &texts_.stage_mature},
        {"condition_wilted", &texts_.condition_wilted},
        {"condition_dead", &texts_.condition_dead},
        {"unknown", &texts_.unknown},
        {"restart_note", &texts_.restart_note},
        {"error", &texts_.error},
        {"separator", &texts_.separator},
    };
    for (const auto& entry : wanted) {
        if (!read_text(texts, entry.key, *entry.out)) {
            const std::string key = std::string("texts.") + entry.key;
            return fail_config(key.c_str());
        }
    }
    return true;
}

}  // namespace ase::adp::godot
