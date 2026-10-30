/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW DEBUG MODE
 *
 * @file        godot_vivarium_debug.cpp
 * @brief       VivariumView - the debug buttons: the time lapse of this host's clock, and the lapse to a stage
 * @description Operator order of 2026-10-26: the birch grows over game years, a game day lasts twenty minutes,
 *              and the life cycle must be seen up to the adult tree - "vorspulen ... ohne Änderung der
 *              Serverzeit". Launched with modes.debug.argument the view draws the buttons of the section
 *              "debug" in their own colours below the info lines, unlike every game button. A button asks this
 *              host's game clock for a time scale through the pace port (ase-pl-flora → TIM_REQ_TIME_SCALE →
 *              ase-time); the request goes to the embedded host of this process only, so no server clock moves,
 *              and every tick still runs - the shoot, the roots and the soil pass every day of the lapse in their
 *              own systems, the parity of the client holds. A button that names a stage runs the lapse until the
 *              selected patch reached it, waters the patch whenever its soil falls below debug.keep_soil_rel - the
 *              keeper's care the lapse would otherwise skip - and gives the default scale back at the stage, or
 *              when the stand died. Without the argument nothing of this is drawn or answered.
 *
 *              A LAPSE TO A STAGE IS NOT BOUND TO THE FRAMES. The scale alone runs one tick per frame, and the
 *              years of a tree do not fit into that: PLT_SPEC_GROWTH_TIME of betula_pendula is 22 growth
 *              seasons, 7722 game days of 30 hours, 833 976 000 game seconds - at the scale 9000 and ticks of
 *              0.25 s that is 370 656 ticks, 25.7 hours at one tick per frame. So during a lapse to a stage the
 *              frame hands the host debug.lapse_s on top of its real time and lets it run debug.lapse_steps more
 *              ticks (godot_vivarium_node.cpp, GodotHostResourceManager::advance). Every one of them is an
 *              ordinary tick of the client's own clock; how many a frame gets through is what the client's
 *              processor can compute, nothing is loaded and nothing skipped.
 *
 *              WHEN THE STAGES COME (catalogue of ase-plant, betula_pendula; one game day = 30 hours):
 *              Semente from the start; Broto with the first pour that lifts the soil to PLT_SPEC_GERM_REL 0.4;
 *              Crescendo once the shoot reaches PLANT_SPRT_SIZE 0.1 of the largest size of the species, measured
 *              by the lapse itself; Madura when ase-plant marks the stand mature (plant_stge_mtur_sys.cpp, which
 *              reads PLT_SPEC_GROWTH_TIME against the age of the stand).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00002
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/array.hpp>

#include <cstdint>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

bool VivariumView::load_debug(const ::godot::Dictionary& root) {
    // The section stands in every configuration and is read always - a broken debug section is a broken
    // configuration on a normal start too; it is drawn and answered only with the launch argument.
    debug_ = VivariumDebug{};
    ::godot::Dictionary debug;
    ::godot::String fill;
    ::godot::String text;
    if (!read_section(root, "debug", debug) || !read_text(debug, "fill", fill) || !read_text(debug, "text", text) ||
        !fill.is_valid_html_color() || !text.is_valid_html_color()) {
        return fail_config("debug");
    }
    debug_.fill = ::godot::Color::html(fill);
    debug_.text = ::godot::Color::html(text);
    if (!read_number(debug, "keep_soil_rel", debug_.keep_soil_rel) || !(debug_.keep_soil_rel > 0.0) ||
        debug_.keep_soil_rel > 1.0) {
        return fail_config("debug.keep_soil_rel");
    }
    // The lapse to a stage: the real seconds a frame hands the host beyond its own, and the ticks it may
    // run for them on top of catch_up_steps. Both above 0, the steps whole - a lapse that hands time and
    // may run no tick for it only piles up a backlog.
    double lapse_steps = 0.0;
    if (!read_number(debug, "lapse_s", debug_.lapse_s) || !(debug_.lapse_s > 0.0)) {
        return fail_config("debug.lapse_s");
    }
    if (!read_number(debug, "lapse_steps", lapse_steps) || !(lapse_steps >= 1.0) ||
        lapse_steps > static_cast<double>(GODOT_DEBUG_LAPSE_STEPS_MAX) ||
        ase::math::floor(static_cast<float>(lapse_steps)) != static_cast<float>(lapse_steps)) {
        return fail_config("debug.lapse_steps");
    }
    debug_.lapse_steps = static_cast<uint32_t>(lapse_steps);
    const ::godot::Variant buttons_value = debug.get("buttons", ::godot::Variant());
    if (buttons_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("debug.buttons");
    }
    const ::godot::Array buttons = buttons_value;
    for (int64_t i = 0; i < buttons.size(); ++i) {
        if (buttons[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("debug.buttons[]");
        }
        const ::godot::Dictionary entry = buttons[i];
        VivariumDebugButton button;
        double stage = 0.0;
        if (!read_rect(entry, "rect", button.rect) || !read_text(entry, "label", button.label) ||
            !read_number(entry, "scale", button.scale) || !(button.scale >= GODOT_PACE_NONE)) {
            return fail_config("debug.buttons[]");
        }
        // A stage below 0 names none: the button switches the time lapse only.
        if (!read_number(entry, "stage", stage) || stage >= static_cast<double>(GODOT_STAGE_COUNT) ||
            ase::math::floor(static_cast<float>(stage)) != static_cast<float>(stage)) {
            return fail_config("debug.buttons[].stage");
        }
        button.stage = stage < 0.0 ? GODOT_STAGE_NONE : static_cast<uint8_t>(stage);
        if (!in_one_band(layout_.bands, button.rect.position.y, button.rect.get_end().y)) {
            return fail_config("layout.bands: debug.buttons[].rect");
        }
        debug_.buttons.push_back(button);
    }
    debug_.on = launched_with(modes_.debug_argument);
    return true;
}

bool VivariumView::debug_tap(const ::godot::Vector2& at) {
    if (!debug_.on) {
        return false;
    }
    for (const VivariumDebugButton& button : debug_.buttons) {
        if (placed(button.rect).has_point(at)) {
            ask_pace(button);
            return true;
        }
    }
    return false;
}

void VivariumView::ask_pace(const VivariumDebugButton& button) {
    // A lapse to a stage watches the selected patch: without one there is nothing to run to.
    const bool to_stage = button.stage != GODOT_STAGE_NONE;
    const ::godot::String stage_word = to_stage ? ::godot::String::num_uint64(button.stage) : ::godot::String("none");
    if (to_stage && (selected_place_ == GODOT_NO_PLACE || resources_.get_patch_at(selected_place_) == nullptr)) {
        note("debug pace scale=" + ::godot::String::num(button.scale, GODOT_LOG_DECIMALS) + " stage=" + stage_word +
             " result=disabled");
        return;
    }
    const HostStatus status = resources_.pace(button.scale);
    note("debug pace scale=" + ::godot::String::num(button.scale, GODOT_LOG_DECIMALS) + " stage=" + stage_word +
         (to_stage ? " " + measure(selected_place_) : ::godot::String()) + " result=" +
         GodotHostResourceManager::status_name(status));
    if (status != ase::kernel::HostStatusOk) {
        return;
    }
    debug_.scale = button.scale;
    debug_.target_stage = to_stage ? button.stage : GODOT_STAGE_NONE;
    debug_.target_place = to_stage ? selected_place_ : GODOT_NO_PLACE;
    node_->queue_redraw();
}

void VivariumView::follow_debug() {
    if (debug_.target_stage == GODOT_STAGE_NONE) {
        return;
    }
    const VivariumPatchView* patch = resources_.get_patch_at(debug_.target_place);
    if (patch == nullptr) {
        return;  // no record yet in this frame; the next snapshot carries it
    }
    // THE END OF THE LAPSE: the stage reached, or the stand dead - a dead birch grows to no stage.
    const bool reached = patch->stage != GODOT_CODE_UNKNOWN && patch->stage >= debug_.target_stage;
    const bool dead = patch->condition == GODOT_COND_DEAD;
    if (reached || dead) {
        const HostStatus status = resources_.pace(GODOT_PACE_NONE);
        note(::godot::String("debug ") + (reached ? "reached" : "ended dead") + " stage=" +
             ::godot::String::num_uint64(patch->stage) + " " + measure(debug_.target_place) + " result=" +
             GodotHostResourceManager::status_name(status));
        debug_.scale = GODOT_PACE_NONE;
        debug_.target_stage = GODOT_STAGE_NONE;
        debug_.target_place = GODOT_NO_PLACE;
        node_->queue_redraw();
        return;
    }
    // THE KEEPER'S CARE DURING THE LAPSE: a pour whenever the soil fell below keep_soil_rel and no pour is on
    // its way - through Regar, the way the player waters, on the patch the lapse watches.
    if (patch->soil_rel < debug_.keep_soil_rel && selected_place_ == debug_.target_place && can_water()) {
        water();
    }
}

void VivariumView::draw_debug() {
    if (!debug_.on) {
        return;
    }
    // THE DEBUG BUTTONS LOOK LIKE NONE OF THE GAME: their own face and text colour, the small font, a wider
    // outline on the button whose lapse is running.
    const ::godot::Ref<::godot::Font> font = view_font();
    for (const VivariumDebugButton& button : debug_.buttons) {
        use_band(button.rect.position.y);
        const bool running = debug_.scale > GODOT_PACE_NONE && button.scale == debug_.scale &&
                             button.stage == debug_.target_stage;
        node_->draw_rect(button.rect, debug_.fill);
        node_->draw_rect(button.rect, debug_.text, false, running ? GODOT_BORDER_WIDTH_ACTIVE : GODOT_BORDER_WIDTH);
        const float baseline = button.rect.get_center().y +
                               (font->get_ascent(GODOT_FONT_SMALL) - font->get_descent(GODOT_FONT_SMALL)) * GODOT_HALF;
        node_->draw_string(font, ::godot::Vector2(button.rect.position.x, baseline), button.label,
                           ::godot::HORIZONTAL_ALIGNMENT_CENTER, button.rect.size.x, GODOT_FONT_SMALL, debug_.text);
    }
}

}  // namespace ase::adp::godot
