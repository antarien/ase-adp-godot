#pragma once

/**
 * ASE GODOT ADAPTER - HUD REPLAY
 *
 * @file        godot_hud_replay.hpp
 * @brief       The mockup's lastKey: when the fades and drawings of the HUD play again and when they stand at their end
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, inventory row T05. The mockup's render (m.html 903, 904)
 *              joins eight values into one key, [S.mode, S.sign, S.perc, S.debug, S.drawer, S.dtab, S.stage,
 *              S.dead], and lays the class .still over the whole HUD while the key is the one of the last render:
 *              every .fade and .draw (m.html 38, 39) then stands at its end. When one of the eight changes, the
 *              animations run again from the start.
 *
 *              THE KEY. HudReplayKey carries the eight values as the view and the HUD hold them: the mode by its
 *              name in hud.modes, the open sign by its id, the perception level of hud.perception.levels, the
 *              debug view, its drawer and its tab (GODOT_HUD_DEBUG_TAB_*), the stage of the patch in focus
 *              (GODOT_STAGE_* of flora.patch.v1, GODOT_STAGE_NONE without a patch) and whether ase-plant marks
 *              that stand GODOT_COND_DEAD.
 *
 *              THE CLOCK. HudReplay holds the key of the last call and the clock the animations play from.
 *              take() compares a new key with the held one: on any difference it takes the new key and the
 *              given clock. mark() takes a key and a clock without comparing - the HUD uses it where it moves
 *              the mode itself after the view asked for the frame, so the next take() finds nothing new.
 *
 *              The class holds no tokens and computes no place; how long a fade or a drawing runs is UiMotion's,
 *              read from SHA. The reduced motion of the platform (the frame's still) holds every animation at its
 *              end whatever this key says; that is the caller's.
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

#include <ase/adp/godot/types.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

/** The eight values of the mockup's lastKey (m.html 903). */
struct HudReplayKey {
    std::string mode;                                // S.mode: the mode's name in hud.modes
    std::string sign;                                // S.sign: the open sign's id, empty = none
    uint32_t level = 0;                              // S.perc: the perception level
    bool debug = false;                              // S.debug: the debug view is on
    bool drawer = false;                             // S.drawer: its drawer is open
    uint32_t tab = GODOT_HUD_DEBUG_TAB_TEMPO;        // S.dtab: GODOT_HUD_DEBUG_TAB_* the drawer shows
    uint8_t stage = GODOT_STAGE_NONE;                // S.stage: GODOT_STAGE_* of the patch in focus
    bool dead = false;                               // S.dead: ase-plant marks the stand GODOT_COND_DEAD
};

/** The clock the HUD's fades and drawings play from, and the key it was taken at. */
class HudReplay {
public:
    /** Takes `now` and `clock` when `now` differs from the held key in any value; answers the clock held after. */
    double take(const HudReplayKey& now, double clock);

    /** Takes `now` and `clock` without comparing: the HUD moved the mode itself. */
    void mark(const HudReplayKey& now, double clock);

    /** The clock the animations play from. */
    double since() const { return since_; }

    /** The key of the last take or mark. */
    const HudReplayKey& key() const { return key_; }

private:
    HudReplayKey key_;
    double since_ = 0.0;
};

}  // namespace ase::adp::godot
