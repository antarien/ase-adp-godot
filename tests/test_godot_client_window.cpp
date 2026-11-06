/**
 * ASE ADAPTER TESTS - THE CLIENT WINDOW WITHOUT GODOT
 *
 * @file        test_godot_client_window.cpp
 * @design      DSGN_016
 * @brief       Pins when the client window counts as open and the line every start writes about it
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS, case "start the client window", the part that carries no Godot
 *              type (godot_client_window.hpp): a window with a size on a real display is open, the headless
 *              display and a window without a size are not, and the line names display, size, surface,
 *              screen, scale, input method and orientation.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101019 - start the client window
 * @actor       Vivarium Steward (VIVARIUSTE)
 * @created     2026-11-05
 * @modified    2026-11-05
 * @version     00.00.01.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_client_window.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

namespace {

constexpr int32_t kWindowWidth = 450;     // window override of project.godot
constexpr int32_t kWindowHeight = 800;
constexpr float kSurfaceWidth = 720.0f;   // viewport of project.godot, stretched by canvas_items
constexpr float kSurfaceHeight = 1280.0f;

ClientWindowReading opened_window() {
    ClientWindowReading reading;
    reading.display = "X11";
    reading.width = kWindowWidth;
    reading.height = kWindowHeight;
    reading.surface_width = kSurfaceWidth;
    reading.surface_height = kSurfaceHeight;
    reading.screen = 0;
    reading.screens = 1;
    reading.scale = 1.0f;
    reading.input_method = true;
    return reading;
}

}  // namespace

TEST_CASE("start the client window: a window with a size on a real display is open and upright") {
    const ClientWindowReading reading = opened_window();
    CHECK(client_window_open(reading));
    CHECK(client_window_portrait(reading));
    CHECK(client_window_line(reading) ==
          "window open display=X11 size=450x800 surface=720.00x1280.00 screen=0/1 scale=1.00 ime=on portrait=yes");
}

TEST_CASE("start the client window: the headless display and a window without a size are not open") {
    ClientWindowReading headless = opened_window();
    headless.display = GODOT_DISPLAY_HEADLESS;
    CHECK_FALSE(client_window_open(headless));
    CHECK(client_window_line(headless) == "window closed display=headless size=450x800");

    ClientWindowReading empty = opened_window();
    empty.width = 0;
    CHECK_FALSE(client_window_open(empty));
    CHECK(client_window_line(empty) == "window closed display=X11 size=0x800");
}

TEST_CASE("start the client window: a window wider than high is open but not upright") {
    ClientWindowReading wide = opened_window();
    wide.width = kWindowHeight;
    wide.height = kWindowWidth;
    wide.input_method = false;
    CHECK(client_window_open(wide));
    CHECK_FALSE(client_window_portrait(wide));
    const std::string line = client_window_line(wide);
    CHECK(line.find(" ime=off") != std::string::npos);
    CHECK(line.find(" portrait=no") != std::string::npos);
}

}  // namespace ase::adp::godot
