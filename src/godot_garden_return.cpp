/**
 * ASE GODOT ADAPTER - THE RETURN TO THE GARDEN
 *
 * @file        godot_garden_return.cpp
 * @brief       A span the app spent away is noticed in one frame and caught up by the host; its end carries the duration
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6 point 2 and V6, see godot_garden_return.hpp. Reads only the
 *              three numbers of a frame; the ticks and the backlog are the host's.
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

#include <ase/adp/godot/godot_garden_return.hpp>
#include <ase/adp/godot/types.hpp>

namespace ase::adp::godot {

GardenReturnFrame garden_return_frame(GardenReturn& state, double now_s, double elapsed_s, double backlog_s) {
    GardenReturnFrame frame;
    if (elapsed_s > GODOT_CATCH_UP_NOTE_S) {
        frame.noticed = true;
        if (backlog_s > 0.0) {
            // The first frame of a return starts its clock; a later long frame of the same catch-up does not.
            if (!state.catching_up) {
                state.since_s = now_s;
            }
            state.catching_up = true;
            return frame;
        }
    }
    if (state.catching_up && !(backlog_s > 0.0)) {
        state.catching_up = false;
        frame.caught_up = true;
        frame.catch_up_s = now_s - state.since_s;
    }
    return frame;
}

}  // namespace ase::adp::godot
