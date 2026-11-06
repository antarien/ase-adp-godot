/**
 * ASE GODOT ADAPTER - START THE CLIENT WINDOW
 *
 * @file        godot_client_window.cpp
 * @brief       The reading of the client window once it opened, and the line that reports it
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS, case "start the client window", see godot_client_window.hpp.
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

#include <ase/adp/godot/godot_client_window.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/utils/strops.hpp>

namespace ase::adp::godot {

namespace {

// a measured value of the window with the decimals of a window line
std::string window_number(float value) {
    char text[GODOT_WINDOW_NUMBER_CHARS] = {};
    ase::utils::str_append_f32(text, GODOT_WINDOW_NUMBER_CHARS, value, GODOT_WINDOW_DECIMALS);
    return std::string(text);
}

}  // namespace

bool client_window_open(const ClientWindowReading& reading) {
    if (reading.width <= 0 || reading.height <= 0) {
        return false;
    }
    return reading.display != GODOT_DISPLAY_HEADLESS;
}

bool client_window_portrait(const ClientWindowReading& reading) {
    return reading.height >= reading.width;
}

std::string client_window_line(const ClientWindowReading& reading) {
    const std::string size = std::to_string(reading.width) + "x" + std::to_string(reading.height);
    if (!client_window_open(reading)) {
        return "window closed display=" + reading.display + " size=" + size;
    }
    return "window open display=" + reading.display + " size=" + size +
           " surface=" + window_number(reading.surface_width) + "x" + window_number(reading.surface_height) +
           " screen=" + std::to_string(reading.screen) + "/" + std::to_string(reading.screens) +
           " scale=" + window_number(reading.scale) + " ime=" + (reading.input_method ? "on" : "off") +
           " portrait=" + (client_window_portrait(reading) ? "yes" : "no");
}

}  // namespace ase::adp::godot
