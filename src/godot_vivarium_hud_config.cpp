/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD CONFIGURATION
 *
 * @file        godot_vivarium_hud_config.cpp
 * @brief       VivariumView - the HUD's configuration once at load, and the still mode the platform asks for
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5: what the HUD reads once, when the view is configured. The theme
 *              and the blocks of the section hud (VivariumHud::load), the measures the scene's own passes take from
 *              the same theme (ridges N05, ground by season N06, brook reflection N07), the frame's width in mockup
 *              units, the bridge's values (load_hud_bridge), the inventory and the hand's readings, and what each
 *              ground tool of hud.verbs does. The frame of every drawn picture is filled in godot_vivarium_hud.cpp.
 *
 *              THE STILL MODE IS THE PLATFORM'S (row T09). The mockup holds every motion under
 *              @media (prefers-reduced-motion): no line draws itself, no fade runs, every pulse ring rests at
 *              hud_anim_still_pulse_alpha. The app asks the platform the same question
 *              (DisplayServer::accessibility_should_reduce_animation) when the HUD loads and again on every return
 *              of the focus, since the setting can change while the app lies in the background; the launch argument
 *              modes.still holds it whatever the platform answers. fill_hud hands the answer to every block.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/array.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

bool VivariumView::load_hud(const ::godot::Dictionary& root) {
    std::string failed;
    if (!hud_.load(root, failed)) {
        return fail_config(("hud " + failed).c_str());
    }
    // the ridges of hills on the horizon take their measures from the same theme (row N05)
    if (!hud_ridge_.bind(hud_.theme(), failed)) {
        return fail_config(("hud " + failed).c_str());
    }
    // the ground's colour by season, snow, depth and horizon from the same theme (row N06)
    if (!hud_ground_season_.bind(hud_.theme(), failed)) {
        return fail_config(("hud " + failed).c_str());
    }
    // the leaf's colour by season and the olive it turns toward while its stand wilts from the same theme (B0278)
    if (!hud_leaf_season_.bind(hud_.theme(), failed)) {
        return fail_config(("hud " + failed).c_str());
    }
    // the sky the brook reflects and the strokes of its current from the same theme (row N07)
    if (!hud_brook_reflection_.bind(hud_.theme(), failed)) {
        return fail_config(("hud " + failed).c_str());
    }
    // the cadence the HUD samples the simulation at and the longest step of its clock from the same theme (row T10)
    if (!hud_refresh_.bind(hud_.theme(), failed)) {
        return fail_config(("hud " + failed).c_str());
    }
    // The mockup's units onto the logical surface: hud_frame_width of them span its width.
    double frame_w = 0.0;
    if (!hud_.theme().measure("hud_frame_width", frame_w) || !(frame_w > 0.0) || !(layout_.size.x > 0.0f)) {
        return fail_config("hud_frame_width");
    }
    hud_unit_ = static_cast<double>(layout_.size.x) / frame_w;

    // What each ground tool does: its operation and the radius of its disc, view.flatten_m where it names none.
    ::godot::Dictionary hud;
    ::godot::Dictionary verbs;
    if (!read_section(root, "hud", hud) || !read_section(hud, "verbs", verbs)) {
        return fail_config("hud.verbs");
    }
    if (!load_hud_bridge(hud_.theme())) {
        return false;  // the failing hud_bridge_* token of SHA stands on the error panel already
    }
    if (!load_figure_hud(hud_.theme())) {
        return false;  // the failing hud_figure_* token of SHA stands on the error panel already (07.9)
    }
    read_hud_still();
    std::string carry_failed;
    // The start screen's choice (operator order 2026-11-19): its chips, words and measures are SHA's like the HUD's.
    if (!start_choice_.bind(hud_.theme(), carry_failed)) {
        return fail_config(("hud " + carry_failed).c_str());
    }
    if (!hud_carry_.load(hud_.theme(), carry_failed)) {
        return fail_config(carry_failed.c_str());  // the mockup's inventory: a hud_bridge_* token SHA does not state
    }
    if (!hud_hand_.load(hud_.theme(), carry_failed)) {
        return fail_config(carry_failed.c_str());  // the hand's readings: a hud_bridge_hand_* token SHA does not state
    }
    // the loosened soil on the ground square load_host_config read before (N0456, row N08)
    if (!hud_loose_.load(hud_.theme(), start_.ground, hud_bridge_loose_, carry_failed)) {
        return fail_config(carry_failed.c_str());  // a hud_bridge_loose_* token SHA does not state
    }
    hud_verbs_.clear();
    const ::godot::Array names = verbs.keys();
    for (int64_t i = 0; i < names.size(); ++i) {
        const ::godot::String name = names[i];
        if (name.begins_with("_")) {
            continue;  // _description and the other notes of the configuration
        }
        const std::string key = name.utf8().get_data();
        ::godot::Dictionary entry;
        ::godot::String op;
        if (!read_section(verbs, key.c_str(), entry) || !read_text(entry, "op", op)) {
            return fail_config(("hud.verbs." + key + ".op").c_str());
        }
        VivariumHudVerb verb;
        verb.op = op.utf8().get_data();
        if (verb.op != VivariumHud::OP_LOWER && verb.op != VivariumHud::OP_RAISE && verb.op != VivariumHud::OP_BED &&
            verb.op != VivariumHud::OP_LOOSEN && verb.op != VivariumHud::OP_FLATTEN &&
            verb.op != VivariumHud::OP_LEVEL && verb.op != VivariumHud::OP_MEASURE) {
            return fail_config(("hud.verbs." + key + ".op").c_str());
        }
        // The radius of a brush verb is SHA's token hud_verb_<v>_brush_m, the same number the ground reticle draws
        // (GroundReticle::radius): the disc the ground moves under and the ring on the ground are one measure, so a
        // spade stab of 0.25 m digs a hole of 0.25 m. The verbs without a brush (bed, level, measure) keep
        // view.flatten_m as the hand's reading (hud.verbs._description).
        if (verb.op == VivariumHud::OP_LOWER || verb.op == VivariumHud::OP_RAISE ||
            verb.op == VivariumHud::OP_LOOSEN || verb.op == VivariumHud::OP_FLATTEN) {
            const std::string token = GODOT_HUD_VERB_TOKEN + key + GODOT_HUD_VERB_BRUSH_TAIL;
            double brush_m = 0.0;
            if (!hud_.theme().measure(token, brush_m) || !(brush_m > 0.0)) {
                return fail_config(token.c_str());  // a hud_verb_<v>_brush_m token SHA does not state
            }
            verb.brush_m = brush_m;
        } else {
            verb.brush_m = static_cast<double>(flatten_m_);
        }
        // A spade stroke moves the ground by a set amount (applyTerr, m.html 987: elevar and rebaixar move the
        // height by 0.3 under the brush); the amount is the verb's depth_m, required where the verb digs or heaps.
        // A loosen verb carries the tilth depth its tool turns the same way, the depth_m of flora.loosen.v1 (B0649).
        if (verb.op == VivariumHud::OP_LOWER || verb.op == VivariumHud::OP_RAISE ||
            verb.op == VivariumHud::OP_LOOSEN) {
            double depth_m = 0.0;
            if (!read_number(entry, "depth_m", depth_m) || !(depth_m > 0.0)) {
                return fail_config(("hud.verbs." + key + ".depth_m").c_str());
            }
            verb.depth_m = depth_m;
        }
        // applyTerr's word and colour belong to the verb: hud.verbs.<name>.toast and .tone, both required, so a
        // verb that acts always has the toast the mockup shows for it.
        ::godot::String tone;
        ::godot::String toast;
        if (!read_text(entry, "tone", tone) || tone.is_empty()) {
            return fail_config(("hud.verbs." + key + ".tone").c_str());
        }
        if (!read_text(entry, "toast", toast) || toast.is_empty()) {
            return fail_config(("hud.verbs." + key + ".toast").c_str());
        }
        // .roots is required too, and may be empty: the verbs that warn of nothing name no word on purpose, a
        // missing field is a configuration that forgot the root zone.
        // read_text refuses an empty text, so the field is read as the string it must be.
        const ::godot::Variant roots_field = entry.get("roots", ::godot::Variant());
        if (roots_field.get_type() != ::godot::Variant::STRING) {
            return fail_config(("hud.verbs." + key + ".roots").c_str());
        }
        const ::godot::String roots = static_cast<::godot::String>(roots_field);
        verb.tone = tone.utf8().get_data();
        verb.toast = toast.utf8().get_data();
        verb.roots = roots.utf8().get_data();
        hud_verbs_[key] = verb;
    }
    return true;
}

void VivariumView::read_hud_still() {
    // 1 asks for the still mode, 0 does not, -1 the platform cannot say, which leaves the motion running; the launch
    // argument holds it whatever the answer. Logged on each change of the answer or of the mode.
    ::godot::DisplayServer* display = ::godot::DisplayServer::get_singleton();
    const int32_t platform = display != nullptr ? display->accessibility_should_reduce_animation() : -1;
    const bool launched = launched_with(modes_.still_argument);
    const bool still = launched || platform == 1;
    if (hud_still_read_ && still == hud_still_ && platform == hud_still_platform_) {
        return;
    }
    hud_still_read_ = true;
    hud_still_ = still;
    hud_still_platform_ = platform;
    note(::godot::String(("hud still=" + std::to_string(still ? 1 : 0) + " launch=" + std::to_string(launched ? 1 : 0) +
                          " platform=" + std::to_string(platform)).c_str()));
}

}  // namespace ase::adp::godot
