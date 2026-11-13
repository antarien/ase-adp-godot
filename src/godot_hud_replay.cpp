/**
 * ASE GODOT ADAPTER - HUD REPLAY
 *
 * @file        godot_hud_replay.cpp
 * @brief       Compares the mockup's lastKey with the held one and keeps the clock the HUD's animations play from
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, see godot_hud_replay.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_replay.hpp>

namespace ase::adp::godot {

double HudReplay::take(const HudReplayKey& now, double clock) {
    // The mockup joins the eight values and compares the joined key; a difference in any one plays again.
    if (now.mode != key_.mode || now.sign != key_.sign || now.level != key_.level || now.debug != key_.debug ||
        now.drawer != key_.drawer || now.tab != key_.tab || now.stage != key_.stage || now.dead != key_.dead) {
        mark(now, clock);
    }
    return since_;
}

void HudReplay::mark(const HudReplayKey& now, double clock) {
    key_ = now;
    since_ = clock;
}

}  // namespace ase::adp::godot
