/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW LOG STREAM
 *
 * @file        godot_vivarium_log.cpp
 * @brief       VivariumView - the one log stream: the patch baseline, the events of stage and condition, the measure
 * @design      DSGN_066
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3/01.4. THE LOG IS ONE STREAM. The host's logger (ase-log inside
 *              the runtime library) writes the file under user://logs and hands every line to the resource manager;
 *              the view prints them once per frame (drain_log; on Android the manager writes them to logcat itself).
 *              The view's own lines go through the SAME logger via KernelEmbeddedHost::note (note), never around it.
 *              Nothing is logged per render frame.
 *
 *              A CHANGE OF STAGE OR CONDITION IS AN EVENT (L4): announce_patches takes the first published values
 *              of every place as the baseline, follow_stages writes one line per change after it. The condition is
 *              the raw code of ase-plant as the patch record carries it (VivariumPatchView::condition), and every
 *              line names the marks read_snapshot set against the codes of flora.cond.v1 (wilted=, dead=), so the
 *              acceptance reads the drying and the death from the marks and never compares against a number of the
 *              adapter (register D2454, usecase:101340 - show the bed condition).
 *
 *              The lifecycle, the host boot, the boot line and the frame stand in godot_vivarium_node.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    D2454 (the condition marks of flora.cond.v1 in the log line)
 * @created     2026-11-19
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/hud/types.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/kernel/kernel_types.hpp>

#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

namespace ase::adp::godot {

// THE PATCH BASELINE

void VivariumView::announce_patches() {
    // L1: one line per patch, once every place has its record with a stage the modules published.
    // The boot runs no tick, and ase-pl-flora writes a value only while the input row behind it
    // stands (PLAN 01.3 point 1), so the lines come with the first frame that has them all - never
    // a line about a patch the port does not carry yet.
    if (patches_announced_) {
        return;
    }
    for (std::size_t place = 0; place < layout_.place_rects.size(); ++place) {
        const VivariumPatchView* patch = resources_.get_patch_at(static_cast<uint32_t>(place));
        if (patch == nullptr || patch->stage == GODOT_CODE_UNKNOWN || !patch->cond_known) {
            return;
        }
    }
    patches_announced_ = true;
    for (std::size_t place = 0; place < layout_.place_rects.size(); ++place) {
        const VivariumPatchView* patch = resources_.get_patch_at(static_cast<uint32_t>(place));
        stages_[place] = patch->stage;
        conditions_[place] = patch->condition;
        note("patch " + measure(static_cast<uint32_t>(place)));
    }
}

// THE EVENTS

void VivariumView::follow_stages() {
    // A change of stage or condition is an EVENT and gets one line - the acceptance reads growth,
    // ripening and drying from these lines (L4), never from a line per frame. Before the patches
    // were announced there is no baseline, and the first published values are no change.
    if (!patches_announced_) {
        return;
    }
    for (std::size_t place = 0; place < stages_.size() && place < conditions_.size(); ++place) {
        const VivariumPatchView* patch = resources_.get_patch_at(static_cast<uint32_t>(place));
        if (patch == nullptr) {
            continue;
        }
        if (patch->stage != stages_[place]) {
            note("stage " + measure(static_cast<uint32_t>(place)) + " from=" +
                 ::godot::String::num_uint64(stages_[place]));
            stages_[place] = patch->stage;
        }
        // the raw code of ase-plant; NaN (no condition carried) equals NaN here, so an unmeasured patch logs once
        const bool same = patch->condition == conditions_[place] ||
                          (std::isnan(patch->condition) && std::isnan(conditions_[place]));
        if (!same) {
            note("condition " + measure(static_cast<uint32_t>(place)) + " from=" +
                 ::godot::String::num(conditions_[place], 0));
            conditions_[place] = patch->condition;
        }
    }
}

// THE LINE

::godot::String VivariumView::measure(uint32_t place) const {
    const VivariumClockView& clock = resources_.get_clock();
    ::godot::String line = "t=" + ::godot::String::num(resources_.simulation_seconds(), GODOT_LOG_DECIMALS) +
                           " ticks=" + ::godot::String::num_uint64(resources_.tick_count());
    // Steps are the scheduler's count of the modules' step schedule, never t divided by its
    // interval. A host that cannot answer (none yet, stopped) gives no steps= at all instead of a
    // false 0; the host logs its own refusal.
    uint64_t steps = 0u;
    if (resources_.schedule_runs(GODOT_LOG_STEP_SCHEDULE, &steps) == ase::kernel::HostStatusOk) {
        line += " steps=" + ::godot::String::num_uint64(steps);
    }
    if (clock.known) {
        line += " game_s=" + ::godot::String::num(clock.elapsed_s, GODOT_LOG_DECIMALS) +
                " day=" + ::godot::String::num(clock.day, GODOT_LOG_DECIMALS) +
                " hour=" + ::godot::String::num(clock.hour, GODOT_LOG_DECIMALS);
        if (clock.shoot_known) {
            line += " shoot_s=" + ::godot::String::num(clock.shoot_s, GODOT_LOG_DECIMALS);
        }
    }
    line += " patches=" + ::godot::String::num_uint64(resources_.patch_count());
    const VivariumPatchView* patch = place == GODOT_NO_PLACE ? nullptr : resources_.get_patch_at(place);
    if (patch == nullptr) {
        return line;
    }
    return "place=" + ::godot::String::num_uint64(place) + " object=" + ::godot::String::num_uint64(patch->object_id) +
           " " + line + " coverage=" + ::godot::String::num(patch->coverage, GODOT_LOG_DECIMALS) +
           " stage=" + ::godot::String::num_uint64(patch->stage) +
           " condition=" + ::godot::String::num(patch->condition, 0) +
           " wilted=" + ::godot::String::num_uint64(patch->wilted ? 1u : 0u) +
           " dead=" + ::godot::String::num_uint64(patch->dead ? 1u : 0u) +
           " soil_mm=" + ::godot::String::num(patch->soil_mm, GODOT_LOG_DECIMALS) +
           " soil_rel=" + ::godot::String::num(patch->soil_rel, GODOT_LOG_DECIMALS) +
           " capacity_mm=" + ::godot::String::num(patch->capacity_mm, GODOT_LOG_DECIMALS);
}

// THE STREAM

void VivariumView::drain_log() {
    log_lines_.clear();
    if (resources_.remove_log_lines(log_lines_) == 0u) {
        return;
    }
    for (const std::string& line : log_lines_) {
        ::godot::UtilityFunctions::print(text_of(line));
    }
}

void VivariumView::note(const ::godot::String& line) {
    const ::godot::CharString text = line.utf8();
    if (resources_.note(text.get_data()) != ase::kernel::HostStatusOk) {
        // No host (before the first boot, after a failed create): Godot's own output carries it.
        ::godot::UtilityFunctions::print(::godot::String("[") + GODOT_LOG_SOURCE + "] " + line);
    }
}

}  // namespace ase::adp::godot
