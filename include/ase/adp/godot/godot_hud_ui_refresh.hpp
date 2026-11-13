#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI REFRESH
 *
 * @file        godot_hud_ui_refresh.hpp
 * @brief       The cadence the HUD samples the simulation at and the longest step its own clock takes in a frame
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, building block Basis, inventory row T10. The mockup's
 *              frame loop (m.html 1023-1038) does two things at two paces. drawScene runs every frame, and so do the
 *              CSS animations of the HUD. render(false), which writes every reading of the simulation into the HUD
 *              (the hour, the date, the soil, the stage, the debug numbers), runs only when the frames since the
 *              last render add up to more than 0.25 s, and in every frame while a finger holds the can (S.holding),
 *              holds the system ring (S.sysHold) or the pruning mode is open. An event renders at once on top
 *              (a click, a mode, a new stage, the death). Every frame's step is the frame's real time, never more
 *              than 0.05 s: a stalled frame moves the HUD's clock by that much and no further.
 *
 *              UiRefresh is the one place that holds both: hud_refresh_ms of SHA's design tokens is the period,
 *              hud_frame_dt_max the longest step. The view asks step() for the seconds its HUD clock, the held
 *              pours, the system ring and the cloud banks run on, advance() once a frame with that step and
 *              whether the frame holds something, force() on an event, and take() where it reads the simulation:
 *              true is the frame in which the readings are sampled again, false keeps the readings of the last
 *              sample in the HUD. Geometry, the scene and every animation keep running in every frame.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <string>

namespace ase::adp::godot {

/** The HUD's refresh cadence and the longest step of its clock. */
class UiRefresh {
public:
    /**
     * Binds hud_refresh_ms and hud_frame_dt_max of the theme. False with `failed` naming the token that is missing
     * or not positive.
     */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The seconds a frame of `elapsed` real seconds moves the HUD's clock: never negative, never above the cap. */
    double step(double elapsed) const;

    /**
     * One frame of `step` seconds: the frames since the last sample add up, and once they pass the period the next
     * take() samples again; `held` (the can, the system ring, the pruning mode) samples in this frame in any case.
     */
    void advance(double step, bool held);

    /** An event (a click, a mode, a stage, the death): the next take() samples at once, the period runs on. */
    void force() { due_ = true; }

    /** True once for the frame in which the readings of the simulation are sampled again. */
    bool take();

    /** The period in seconds. */
    double period() const { return period_; }

private:
    double period_ = 0.0;    // hud_refresh_ms in seconds
    double step_max_ = 0.0;  // hud_frame_dt_max
    double since_ = 0.0;     // seconds of the frames since the last timed sample
    bool due_ = true;        // the first frame samples
};

}  // namespace ase::adp::godot
