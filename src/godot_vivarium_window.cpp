/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW WINDOW
 *
 * @file        godot_vivarium_window.cpp
 * @brief       VivariumView - the client window it opened in, read from the display server and reported once
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS, case "start the client window": the first thing the view does
 *              in ready() is to read the window the window manager gave it - display server, window size,
 *              the surface of its viewport, screen, scale and input method - and to write the line of
 *              godot_client_window.hpp. A window that is not open (the headless display, a size of zero)
 *              is written as such; the view goes on, since a headless run draws nothing but still hosts.
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

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_client_window.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/display_server.hpp>

namespace ase::adp::godot {

void VivariumView::announce_window() {
    ::godot::DisplayServer* display = ::godot::DisplayServer::get_singleton();
    const ::godot::Vector2i size = display->window_get_size(::godot::DisplayServer::MAIN_WINDOW_ID);
    const ::godot::Rect2 surface = node_->get_viewport_rect();

    ClientWindowReading reading;
    reading.display = utf8_of(display->get_name());
    reading.width = size.x;
    reading.height = size.y;
    reading.surface_width = surface.size.x;
    reading.surface_height = surface.size.y;
    reading.screen = display->window_get_current_screen(::godot::DisplayServer::MAIN_WINDOW_ID);
    reading.screens = display->get_screen_count();
    reading.scale = display->screen_get_scale(reading.screen);
    reading.input_method = display->has_feature(::godot::DisplayServer::FEATURE_IME);

    note(text_of(client_window_line(reading)));
}

}  // namespace ase::adp::godot
