#pragma once

/**
 * ASE GODOT ADAPTER - START THE CLIENT WINDOW
 *
 * @file        godot_client_window.hpp
 * @brief       The reading of the client window once it opened, and the line that reports it
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS, case "start the client window": the window of the app opens.
 *              The window manager owns the window (project.godot sets its size, its stretch and the
 *              portrait orientation of a handheld); the adapter reads what it got: the display server, the
 *              window size and the surface the view draws on, the screen it stands on, its scale and whether
 *              the display offers an input method. From that reading this file decides whether the window
 *              is open - a size above zero on a display that is not the headless one - and builds the one
 *              line the view writes to its log, so that every start names the window it drew into.
 *
 *              NO GODOT TYPE: the view fills ClientWindowReading from the DisplayServer and its viewport
 *              (godot_vivarium_window.cpp); the decision and the line are tested without a Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_016
 * @category    render/pipeline/stage
 * @usecase     usecase:101019 - start the client window
 * @actor       Vivarium Steward (VIVARIUSTE)
 * @created     2026-11-05
 * @modified    2026-11-05
 * @version     00.00.01.00000
 */

#include <cstdint>
#include <string>

namespace ase::adp::godot {

/**
 * What the display server reports about the client window after it opened.
 */
struct ClientWindowReading {
    std::string display;          // name of the display server (GODOT_DISPLAY_HEADLESS without a window)
    int32_t width = 0;            // window width, window pixels
    int32_t height = 0;           // window height, window pixels
    float surface_width = 0.0f;   // width of the surface the view draws on, logical units
    float surface_height = 0.0f;  // height of that surface, logical units
    int32_t screen = 0;           // index of the screen the window stands on
    int32_t screens = 0;          // number of screens of the display
    float scale = 0.0f;           // scale of that screen
    bool input_method = false;    // the display offers an input method (FEATURE_IME)
};

/**
 * The window is open: it has a size above zero and stands on a display that is not the headless one.
 */
[[nodiscard]] bool client_window_open(const ClientWindowReading& reading);

/**
 * The window stands upright: its height reaches at least its width (the handheld orientation of project.godot).
 */
[[nodiscard]] bool client_window_portrait(const ClientWindowReading& reading);

/**
 * The log line of the window: "window open display=... size=WxH surface=WxH screen=s/n scale=x ime=on|off
 * portrait=yes|no", or "window closed display=... size=WxH" when client_window_open refuses it.
 */
[[nodiscard]] std::string client_window_line(const ClientWindowReading& reading);

}  // namespace ase::adp::godot
