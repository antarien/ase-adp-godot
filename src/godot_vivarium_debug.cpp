/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW DEBUG MODE
 *
 * @file        godot_vivarium_debug.cpp
 * @brief       VivariumView - the debug buttons: the time lapse, the jump to a stage or an hour, the relief
 * @description Operator order of 2026-10-26: the birch grows over game years, and its life course must be seen
 *              up to the adult tree without changing any server time. Launched with modes.debug.argument the
 *              view draws the buttons of the section "debug" in their own colours below the info lines, unlike
 *              every game button. Without the argument nothing of this is drawn or answered.
 *
 *              TWO CASES OF TIME, NOTHING ELSE (operator order of 2026-11-06):
 *
 *              THE TIME LAPSE (debug.buttons) runs this host's clock at a multiple of ase-time's base pace. A
 *              button names its factor only; the view sends it through the pace port, ase-pl-flora multiplies
 *              it with the base pace it reads from the star (FloraHostPaceSystem, usecase:101120 clock the growth
 *              cycle) and files the product as TIM_REQ_TIME_SCALE for ase-time. The view carries no constant of
 *              ase-time: what a game second is stands in ase-time's design and on the star, never here. The
 *              lapse has no target and no end; it holds until another factor is asked.
 *
 *              THE JUMP (debug.jumps) moves the clock to a target: a stage of the selected patch, or an hour of
 *              the game day. Both are the same case - only the recognition of the target differs. Every tick up
 *              to the target is an ordinary tick of the client's own clock, computed by every system of the host,
 *              across the day change too; the graphics then show the state at the new time. The pace stays as it
 *              was before the jump - the jump never touches the pace port. During a jump the frame hands the host
 *              up to debug.lapse_s beyond its own real time and lets it run debug.lapse_steps more ticks
 *              (VivariumView::jump_feed in godot_vivarium_clock.cpp, GodotHostResourceManager::advance); how
 *              many ticks a frame gets through is what the client's processor computes, nothing is loaded and
 *              nothing skipped. The ticks a jump needs are the game seconds to its target over the game seconds
 *              of one tick at the running pace; for Madura that is PLT_SPEC_GROWTH_TIME of the species.
 *
 *              WHEN THE STAGES COME (catalogue of ase-plant, betula_pendula): Semente from the start; Broto with
 *              the first pour that lifts the soil to PLT_SPEC_GERM_REL; Crescendo once the shoot reaches
 *              PLANT_SPRT_SIZE of the largest size of the species; Madura when ase-plant marks the stand mature
 *              (plant_stge_mtur_sys.cpp, which reads PLT_SPEC_GROWTH_TIME against the age of the stand). During
 *              a stage jump the patch is watered whenever its soil falls below debug.keep_soil_rel - the keeper's
 *              care the jump would otherwise skip.
 *
 *              THE RELIEF BUTTONS (debug.reliefs, usecase:6018 derive terrain heights) change the relief of the
 *              whole ground at runtime. A button asks the elevation port for its height amplitude and ground level
 *              (GodotHostResourceManager::shape); ase-pl-flora writes them as TER_GEN_HGT_SCL and TER_GEN_BAS_HGT
 *              on the star, ase-terrain moves every generated chunk onto the new profile and lays the brook beds
 *              and the flattened patches anew. The seed is not touched, so the landscape keeps its shape and only
 *              its height changes.
 *
 *              THE ICONS OF THE BUTTONS are never raw glyphs of the configuration: a button names an ICON_*,
 *              the view resolves it in SHA's icon table (debug.icon_table) and draws it in the Nerd Font
 *              Icon.tsx draws with (debug.icon_font), shifted by the offset Icon.tsx gives it
 *              (godot_view_support.hpp draw_icon_label). A name the table lacks, or a glyph the font lacks,
 *              refuses the configuration. A jump may name no icon; it is drawn with its label alone.
 *
 *              The running factor, the running jump and the standing relief are drawn with the wider outline.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066, DSGN_017
 * @category    ecs/module
 * @usecase     usecase:100390 - return to the garden
 * @usecase     usecase:6018 - derive terrain heights
 * @actor       Plant Keeper (PLNTKEEPER), Terrain Height Shaper (TERRHEISHA)
 * @created     2026-10-26
 * @modified    2026-11-06
 * @version     00.00.01.00008
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
    // THE ICONS OF THE BUTTONS come from SHA's icon SSOT, never as a raw glyph of this file: the table
    // names every ICON_* with its codepoint and the offset Icon.tsx gives it, the font is the Nerd Font
    // Icon.tsx draws with (godot_view_support.hpp). Both are staged beside this configuration.
    ::godot::String icon_table;
    ::godot::String icon_font;
    ::godot::Dictionary icons;
    if (!read_text(debug, "icon_table", icon_table) ||
        !read_icon_table(icon_table, icons, debug_.icon_web_px)) {
        return fail_config("debug.icon_table");
    }
    if (!read_text(debug, "icon_font", icon_font)) {
        return fail_config("debug.icon_font");
    }
    debug_.icon_font = icon_font_of(icon_font);
    if (debug_.icon_font.is_null()) {
        return fail_config("debug.icon_font");
    }
    if (!read_number(debug, "keep_soil_rel", debug_.keep_soil_rel) || !(debug_.keep_soil_rel > 0.0) ||
        debug_.keep_soil_rel > 1.0) {
        return fail_config("debug.keep_soil_rel");
    }
    // The feed of a jump: the real seconds a frame hands the host beyond its own, and the ticks it may run
    // for them on top of catch_up_steps. Both above 0, the steps whole - a feed that hands time and may run
    // no tick for it only piles up a backlog.
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
    // THE TIME LAPSE BUTTONS: a factor of the base pace, 0 or above; 0 gives the default pace back.
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
        ::godot::String icon_name;
        if (!read_rect(entry, "rect", button.rect) || !read_text(entry, "label", button.label) ||
            !read_number(entry, "factor", button.factor) || !(button.factor >= GODOT_PACE_NONE)) {
            return fail_config("debug.buttons[]");
        }
        if (!read_text(entry, "icon", icon_name) ||
            !read_icon(icons, icon_name, debug_.icon_font, button.icon, button.icon_offset)) {
            return fail_config("debug.buttons[].icon");
        }
        if (!in_one_band(layout_.bands, button.rect.position.y, button.rect.get_end().y)) {
            return fail_config("layout.bands: debug.buttons[].rect");
        }
        debug_.buttons.push_back(button);
    }
    // THE JUMP BUTTONS: each names exactly one target, a stage of the catalogue or an hour of the game day,
    // 0 or above. How long a day is stands in ase-time, never in this view. The icon is optional.
    const ::godot::Variant jumps_value = debug.get("jumps", ::godot::Variant());
    if (jumps_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("debug.jumps");
    }
    const ::godot::Array jumps = jumps_value;
    for (int64_t i = 0; i < jumps.size(); ++i) {
        if (jumps[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("debug.jumps[]");
        }
        const ::godot::Dictionary entry = jumps[i];
        VivariumDebugJump jump;
        if (!read_rect(entry, "rect", jump.rect) || !read_text(entry, "label", jump.label)) {
            return fail_config("debug.jumps[]");
        }
        const bool names_stage = entry.has("stage");
        const bool names_hour = entry.has("hour");
        if (names_stage == names_hour) {
            return fail_config("debug.jumps[]: stage or hour");
        }
        if (names_stage) {
            double stage = 0.0;
            if (!read_number(entry, "stage", stage) || !(stage >= 0.0) ||
                stage >= static_cast<double>(GODOT_STAGE_COUNT) ||
                ase::math::floor(static_cast<float>(stage)) != static_cast<float>(stage)) {
                return fail_config("debug.jumps[].stage");
            }
            jump.stage = static_cast<uint8_t>(stage);
        } else if (!read_number(entry, "hour", jump.hour) || !(jump.hour >= 0.0)) {
            return fail_config("debug.jumps[].hour");
        }
        if (entry.has("icon")) {
            ::godot::String icon_name;
            if (!read_text(entry, "icon", icon_name) ||
                !read_icon(icons, icon_name, debug_.icon_font, jump.icon, jump.icon_offset)) {
                return fail_config("debug.jumps[].icon");
            }
        }
        if (!in_one_band(layout_.bands, jump.rect.position.y, jump.rect.get_end().y)) {
            return fail_config("layout.bands: debug.jumps[].rect");
        }
        debug_.jumps.push_back(jump);
    }
    // The relief buttons: a height amplitude above 0 and a ground level, both in metres. The section is
    // required like the buttons - a configuration without it names no relief, an empty array is allowed.
    const ::godot::Variant reliefs_value = debug.get("reliefs", ::godot::Variant());
    if (reliefs_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("debug.reliefs");
    }
    const ::godot::Array reliefs = reliefs_value;
    for (int64_t i = 0; i < reliefs.size(); ++i) {
        if (reliefs[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("debug.reliefs[]");
        }
        const ::godot::Dictionary entry = reliefs[i];
        VivariumDebugRelief relief;
        if (!read_rect(entry, "rect", relief.rect) || !read_text(entry, "label", relief.label) ||
            !read_number(entry, "height_scale_m", relief.height_scale_m) || !(relief.height_scale_m > 0.0) ||
            !read_number(entry, "base_height_m", relief.base_height_m)) {
            return fail_config("debug.reliefs[]");
        }
        if (!in_one_band(layout_.bands, relief.rect.position.y, relief.rect.get_end().y)) {
            return fail_config("layout.bands: debug.reliefs[].rect");
        }
        debug_.reliefs.push_back(relief);
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
    for (uint32_t index = 0u; index < static_cast<uint32_t>(debug_.jumps.size()); ++index) {
        if (placed(debug_.jumps[index].rect).has_point(at)) {
            ask_jump(debug_.jumps[index], index);
            return true;
        }
    }
    for (const VivariumDebugRelief& relief : debug_.reliefs) {
        if (placed(relief.rect).has_point(at)) {
            ask_shape(relief);
            return true;
        }
    }
    return false;
}

void VivariumView::ask_pace(const VivariumDebugButton& button) {
    // THE TIME LAPSE: the factor alone goes through the pace port; the base pace and the product are
    // ase-time's, multiplied on the star. A running jump goes on - it rides whatever pace stands.
    const HostStatus status = resources_.pace(button.factor);
    note("debug pace factor=" + ::godot::String::num(button.factor, GODOT_LOG_DECIMALS) + " result=" +
         GodotHostResourceManager::status_name(status));
    if (status != ase::kernel::HostStatusOk) {
        return;
    }
    debug_.factor = button.factor;
    node_->queue_redraw();
}

void VivariumView::ask_jump(const VivariumDebugJump& jump, uint32_t index) {
    // THE JUMP never touches the pace port: the clock reaches the target by computing every tick up to it,
    // the feed of jump_feed carries the frames there, and the pace asked before stays.
    const VivariumClockView& clock = resources_.get_clock();
    if (jump.stage != GODOT_STAGE_NONE) {
        // A stage jump watches the selected patch: without one there is nothing to jump to.
        const ::godot::String stage_word = ::godot::String::num_uint64(jump.stage);
        if (selected_place_ == GODOT_NO_PLACE || resources_.get_patch_at(selected_place_) == nullptr) {
            note("debug jump stage=" + stage_word + " result=disabled");
            return;
        }
        VivariumJump next;
        next.running = true;
        next.button = index;
        next.stage = jump.stage;
        next.place = selected_place_;
        next.asked_day = clock.known ? clock.day : 0.0;
        debug_.jump = next;
        note("debug jump stage=" + stage_word + " " + measure(selected_place_) + " result=ok");
        node_->queue_redraw();
        return;
    }
    // AN HOUR JUMP computes its target from the clock port: the measured game seconds of one hour, the
    // clock's game seconds less the hour of the day give the start of the running day. Day 0 starts at game
    // second 0, so that start over the day count is the length of a day. The feed needs the measured rate.
    const ::godot::String hour_word = ::godot::String::num(jump.hour, GODOT_LOG_DECIMALS);
    const double per_hour_s = hour_length_.seconds_per_hour;
    if (!clock.known || !(per_hour_s > 0.0) || !(hour_length_.game_per_real > 0.0)) {
        note("debug jump hour=" + hour_word + " result=unmeasured");
        return;
    }
    const double day_start_s = clock.elapsed_s - clock.hour * per_hour_s;
    const bool ahead = jump.hour > clock.hour;
    VivariumJump next;
    next.running = true;
    next.button = index;
    next.hour = jump.hour;
    next.asked_day = clock.day;
    if (ahead) {
        next.target_s = day_start_s + jump.hour * per_hour_s;  // the hour of the running day
    } else if (clock.day >= 1.0) {
        next.target_s = day_start_s + day_start_s / clock.day + jump.hour * per_hour_s;  // the same hour tomorrow
    }
    // Behind the clock on day 0 the length of a day is not measurable yet: target_s stays below 0, the jump
    // runs on to the day change one game hour per feed at most, and follow_debug computes the target there.
    debug_.jump = next;
    note("debug jump hour=" + hour_word + " day=" + ::godot::String::num(clock.day, 0) +
         " clock_hour=" + ::godot::String::num(clock.hour, GODOT_LOG_DECIMALS) +
         " when=" + (ahead ? "this_day" : "next_day") +
         " target_s=" + ::godot::String::num(next.target_s, GODOT_LOG_DECIMALS) +
         " elapsed_s=" + ::godot::String::num(clock.elapsed_s, GODOT_LOG_DECIMALS) +
         " game_per_real=" + ::godot::String::num(hour_length_.game_per_real, GODOT_LOG_DECIMALS) + " result=ok");
    node_->queue_redraw();
}

void VivariumView::ask_shape(const VivariumDebugRelief& relief) {
    // The relief goes through the elevation port of the host, the same port the start configuration uses
    // for the ground: ase-pl-flora files it on the star, ase-terrain derives the heights from it.
    const HostStatus status = resources_.shape(relief.height_scale_m, relief.base_height_m);
    note("debug shape height_scale_m=" + ::godot::String::num(relief.height_scale_m, GODOT_LOG_DECIMALS) +
         " base_height_m=" + ::godot::String::num(relief.base_height_m, GODOT_LOG_DECIMALS) + " result=" +
         GodotHostResourceManager::status_name(status));
    if (status != ase::kernel::HostStatusOk) {
        return;
    }
    debug_.height_scale_m = relief.height_scale_m;
    node_->queue_redraw();
}

void VivariumView::follow_debug() {
    VivariumJump& jump = debug_.jump;
    if (!jump.running) {
        return;
    }
    // THE KEEPER'S CARE DURING A JUMP: a pour whenever the soil fell below keep_soil_rel and no pour is on
    // its way - through Regar, the way the player waters, on the patch the camera shows.
    const uint32_t cared = jump.stage != GODOT_STAGE_NONE ? jump.place : selected_place_;
    const VivariumPatchView* patch = resources_.get_patch_at(cared);
    if (jump.stage != GODOT_STAGE_NONE) {
        if (patch == nullptr) {
            return;  // no record yet in this frame; the next snapshot carries it
        }
        // THE END OF A STAGE JUMP: the stage reached, or the stand dead - a dead birch grows to no stage.
        const bool reached = patch->stage != GODOT_CODE_UNKNOWN && patch->stage >= jump.stage;
        const bool dead = patch->condition == GODOT_COND_DEAD;
        if (reached || dead) {
            note(::godot::String("debug jump ") + (reached ? "reached" : "ended reason=dead") +
                 " stage=" + ::godot::String::num_uint64(patch->stage) + " " + measure(jump.place));
            jump = VivariumJump{};
            node_->queue_redraw();
            return;
        }
    } else {
        const VivariumClockView& clock = resources_.get_clock();
        if (!clock.known) {
            return;
        }
        if (jump.target_s < 0.0) {
            // On to the day change: the new day's start is the clock's game seconds less its hour.
            if (!(clock.day > jump.asked_day) || !(hour_length_.seconds_per_hour > 0.0)) {
                return;
            }
            jump.target_s = clock.elapsed_s - clock.hour * hour_length_.seconds_per_hour +
                            jump.hour * hour_length_.seconds_per_hour;
            note("debug jump day_turned day=" + ::godot::String::num(clock.day, 0) +
                 " target_s=" + ::godot::String::num(jump.target_s, GODOT_LOG_DECIMALS));
        }
        // THE END OF AN HOUR JUMP: the clock's game seconds reached the target.
        if (clock.elapsed_s >= jump.target_s) {
            note("debug jump reached hour=" + ::godot::String::num(jump.hour, GODOT_LOG_DECIMALS) +
                 " day=" + ::godot::String::num(clock.day, 0) +
                 " clock_hour=" + ::godot::String::num(clock.hour, GODOT_LOG_DECIMALS) +
                 " target_s=" + ::godot::String::num(jump.target_s, GODOT_LOG_DECIMALS) +
                 " elapsed_s=" + ::godot::String::num(clock.elapsed_s, GODOT_LOG_DECIMALS) + " " +
                 measure(selected_place_));
            jump = VivariumJump{};
            node_->queue_redraw();
            return;
        }
    }
    if (patch != nullptr && patch->soil_rel < debug_.keep_soil_rel && selected_place_ == cared && can_water()) {
        water();
    }
}

void VivariumView::draw_debug() {
    if (!debug_.on) {
        return;
    }
    // THE DEBUG BUTTONS LOOK LIKE NONE OF THE GAME: their own face and text colour, the small font, a wider
    // outline on the button whose factor runs, on the jump that runs and on the relief button whose relief
    // stands.
    const ::godot::Ref<::godot::Font> font = view_font();
    const float text_lift = (font->get_ascent(GODOT_FONT_SMALL) - font->get_descent(GODOT_FONT_SMALL)) * GODOT_HALF;
    for (const VivariumDebugButton& button : debug_.buttons) {
        use_band(button.rect.position.y);
        const bool running = debug_.factor > GODOT_PACE_NONE && button.factor == debug_.factor;
        node_->draw_rect(button.rect, debug_.fill);
        node_->draw_rect(button.rect, debug_.text, false, running ? GODOT_BORDER_WIDTH_ACTIVE : GODOT_BORDER_WIDTH);
        draw_icon_label(*node_, debug_.icon_font, button.rect, button.icon, button.icon_offset, debug_.icon_web_px,
                        button.label, GODOT_FONT_SMALL, debug_.text);
    }
    for (uint32_t index = 0u; index < static_cast<uint32_t>(debug_.jumps.size()); ++index) {
        const VivariumDebugJump& jump = debug_.jumps[index];
        use_band(jump.rect.position.y);
        const bool running = debug_.jump.running && debug_.jump.button == index;
        node_->draw_rect(jump.rect, debug_.fill);
        node_->draw_rect(jump.rect, debug_.text, false, running ? GODOT_BORDER_WIDTH_ACTIVE : GODOT_BORDER_WIDTH);
        if (!jump.icon.is_empty()) {
            draw_icon_label(*node_, debug_.icon_font, jump.rect, jump.icon, jump.icon_offset, debug_.icon_web_px,
                            jump.label, GODOT_FONT_SMALL, debug_.text);
            continue;
        }
        node_->draw_string(font, ::godot::Vector2(jump.rect.position.x, jump.rect.get_center().y + text_lift),
                           jump.label, ::godot::HORIZONTAL_ALIGNMENT_CENTER, jump.rect.size.x, GODOT_FONT_SMALL,
                           debug_.text);
    }
    for (const VivariumDebugRelief& relief : debug_.reliefs) {
        use_band(relief.rect.position.y);
        const bool standing = debug_.height_scale_m > 0.0 && relief.height_scale_m == debug_.height_scale_m;
        node_->draw_rect(relief.rect, debug_.fill);
        node_->draw_rect(relief.rect, debug_.text, false, standing ? GODOT_BORDER_WIDTH_ACTIVE : GODOT_BORDER_WIDTH);
        node_->draw_string(font, ::godot::Vector2(relief.rect.position.x, relief.rect.get_center().y + text_lift),
                           relief.label, ::godot::HORIZONTAL_ALIGNMENT_CENTER, relief.rect.size.x, GODOT_FONT_SMALL,
                           debug_.text);
    }
}

}  // namespace ase::adp::godot
