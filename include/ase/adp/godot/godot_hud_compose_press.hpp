#pragma once

/**
 * ASE GODOT ADAPTER - HUD COMPOSE PRESS
 *
 * @file        godot_hud_compose_press.hpp
 * @brief       HudPress: the record of the press under way, which VivariumHud's gestures write and the view reads
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5 (gestures) and inventory row T10. The mockup keeps what a
 *              finger does in its state S: where and when it came down, what it came down on, whether it holds the
 *              can (S.holding) or the system ring (S.sysHold). VivariumHud's gestures (godot_hud_compose_gesture.cpp)
 *              write this record on press, move, release and tick; its draw reads how long the system ring has been
 *              held. The view reads it too: the mockup's frame loop renders in every frame while a finger holds the
 *              can or the system ring (m.html 1037), and the view's refresh cadence (UiRefresh) asks the record for
 *              exactly that. The record holds no tokens and computes nothing; the thresholds of the gestures are
 *              VivariumHud's, read from SHA.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-12
 * @modified    2026-11-13
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_compose_frame.hpp>

#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

/** The press under way. */
struct HudPress {
    bool down = false;                 // a finger is down
    ::godot::Vector2 at;               // where it came down
    double clock = 0.0;                // when
    double wall = 0.0;                 // when, in the real seconds the long press counts (the mockup's setTimeout)
    VivariumHudHit hit;                // what it came down on
    double long_after = 0.0;           // the long press fires this many seconds after; 0: none
    bool long_fired = false;           // the long press fired: the release does nothing more
    bool thing = false;                // it came down on a thing: a drag may start
    bool window = false;               // it came down on a window's grip
    bool pouring = false;              // it holds the can (S.holding)
    double poured_at = 0.0;            // the clock of the last measure
    bool system = false;               // it holds the system ring (S.sysHold)
};

}  // namespace ase::adp::godot
