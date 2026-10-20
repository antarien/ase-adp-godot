#pragma once

/**
 * ASE GODOT ADAPTER - THE RETURN TO THE GARDEN
 *
 * @file        godot_garden_return.hpp
 * @brief       A span the app spent away is noticed in one frame and caught up by the host; its end carries the duration
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6 point 2 and V6: the keeper leaves the app and comes back.
 *              The world did not stop (PLAN 01.3 point 4): the first frame after the return brings the whole
 *              span, and KernelEmbeddedHost::advance ticks it in ordinary steps over the coming frames until
 *              the backlog is empty. This unit decides what each frame says about it - the frame that
 *              noticed a return, and the frame in which its backlog ran dry together with the seconds the
 *              catch-up took - so the log names the duration V6 asks for. It computes no simulation time:
 *              the ticks are the host's, the backlog it reads is the host's answer.
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
 * @modified    2026-10-12
 * @version     00.00.01.00001
 */

namespace ase::adp::godot {

/** The return to the garden: whether the backlog of a return is still being caught up, and since when. */
struct GardenReturn {
    bool   catching_up = false;  // the backlog of a return has not run dry yet
    double since_s = 0.0;        // clock seconds of the frame that noticed the return
};

/** What one frame says about the return. */
struct GardenReturnFrame {
    bool   noticed = false;      // the frame was a span the app spent away: one catch_up line
    bool   caught_up = false;    // the backlog of a return ran dry in this frame: one caught_up line
    double catch_up_s = 0.0;     // clock seconds from the frame that noticed the return to this one
};

/**
 * One frame of the host, after advance(): `now_s` is the frame clock in seconds, `elapsed_s` the real
 * time the frame handed the host, `backlog_s` what the host still holds after it.
 *
 * A frame longer than GODOT_CATCH_UP_NOTE_S is noticed as a return. A return whose backlog is not
 * empty is being caught up from that frame on; a second long frame while it is does not move its
 * start. The frame in which a return that was being caught up has no backlog left is its end, also
 * when that frame is itself a long one - on a loaded device every frame lasts longer than the
 * threshold, and a backlog that only ended in such a frame would otherwise never be reported.
 */
[[nodiscard]] GardenReturnFrame garden_return_frame(GardenReturn& state, double now_s, double elapsed_s,
                                                    double backlog_s);

}  // namespace ase::adp::godot
