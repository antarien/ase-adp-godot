#pragma once

/**
 * ASE GODOT ADAPTER - CONFIGURATION READERS
 *
 * @file        godot_config_read.hpp
 * @brief       The readers of one key of the start configuration, each refusing a missing key or a wrong type
 * @description The view reads res://config/vivarium_start.json and res://config/vivarium_assets.json as
 *              data (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3). Every key is read through one of these
 *              functions: each answers false for a missing key or a value of the wrong type, so a broken
 *              configuration stops the view with the key on its error panel and never falls back to a
 *              number of the code. The parts of the view that read the configuration
 *              (godot_vivarium_config.cpp, godot_vivarium_host_config.cpp, godot_vivarium_layout.cpp,
 *              godot_vivarium_assets.cpp) share them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100360 - parse a json document
 * @actor       Entity Component System (ECS)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

/** A finite number under `key` (an int or a float of the JSON). */
bool read_number(const ::godot::Dictionary& from, const char* key, double& out);

/** A non-empty text under `key`. */
bool read_text(const ::godot::Dictionary& from, const char* key, ::godot::String& out);

/** An object under `key`. */
bool read_section(const ::godot::Dictionary& from, const char* key, ::godot::Dictionary& out);

/** A rectangle [x, y, width, height] of positive size under `key`. */
bool read_rect(const ::godot::Dictionary& from, const char* key, ::godot::Rect2& out);

/** A point [x, y] under `key`. */
bool read_point(const ::godot::Dictionary& from, const char* key, ::godot::Vector2& out);

}  // namespace ase::adp::godot
