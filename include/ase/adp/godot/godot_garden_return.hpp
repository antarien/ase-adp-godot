#pragma once

/**
 * ASE GODOT ADAPTER - THE RETURN TO THE GARDEN
 *
 * @file        godot_garden_return.hpp
 * @brief       A span the app spent away is noticed in one frame and laid by ase-time; its end carries the duration
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6 point 2 and V6: the keeper leaves the app and comes back.
 *              The world did not stop (PLAN 01.3 point 4): the first frame after the return brings the whole
 *              span as one tick and one filed real position on flora.real.v1, and ase-time sets its clock
 *              to it in one write - the same case as a jump, every module computes the span in its own
 *              pass. The backlog is the span whose filed position the clock record does not show laid yet
 *              (real_laid_s). This unit decides what each frame says about it - the frame that noticed a
 *              return, and the frame in which its backlog ran dry together with the seconds until ase-time
 *              laid it - so the log names the duration V6 asks for. It computes no simulation time: the
 *              ticks are the host's, the backlog it reads is the resource manager's answer.
 *
 *              NO GODOT TYPE: the view writes the lines, this unit only decides them, so the plant test
 *              drives it without a Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    time/period/cycle
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-12
 * @modified    2026-11-12
 * @version     00.00.01.00003
 */

namespace ase::adp::godot {

/** The return to the garden: whether the backlog of a return is still waiting to be laid, and since when. */
struct GardenReturn {
    bool   catching_up = false;  // the backlog of a return has not run dry yet: ase-time has not laid it
    double since_s = 0.0;        // clock seconds of the frame that noticed the return
    bool   away = false;         // the platform sent the app to the background and has not brought it back
};

/** What one frame says about the return. */
struct GardenReturnFrame {
    bool   noticed = false;      // the frame was a span the app spent away: one catch_up line
    bool   caught_up = false;    // the backlog of a return ran dry in this frame: one caught_up line
    double catch_up_s = 0.0;     // clock seconds from the frame that noticed the return to this one
};

/**
 * One frame of the host, after advance(): `now_s` is the frame clock in seconds, `elapsed_s` the real
 * time the frame handed the host, `backlog_s` the seconds of a return whose filed real position the
 * clock record does not show laid after it (GodotHostResourceManager::backlog_seconds).
 *
 * A frame longer than GODOT_CATCH_UP_NOTE_S is noticed as a return. A return whose backlog is not
 * empty is waiting to be laid from that frame on; a second long frame while it is does not move its
 * start. The frame in which a return that was being caught up has no backlog left is its end, also
 * when that frame is itself a long one - on a loaded device every frame lasts longer than the
 * threshold, and a backlog that only ended in such a frame would otherwise never be reported.
 */
[[nodiscard]] GardenReturnFrame garden_return_frame(GardenReturn& state, double now_s, double elapsed_s,
                                                    double backlog_s);

/**
 * The platform's word on the app (usecase:100390 return to the garden): `foreground` false when it sent the app to
 * the background (application paused, focus out), true when it brought it to the front. True only for the front
 * that follows a background: that is the keeper coming back, and the view begins it with the trail (mode arrive,
 * as at the start). The first front of a boot follows no background and answers false - the view already starts
 * in arrive.
 */
[[nodiscard]] bool garden_came_back(GardenReturn& state, bool foreground);

}  // namespace ase::adp::godot
