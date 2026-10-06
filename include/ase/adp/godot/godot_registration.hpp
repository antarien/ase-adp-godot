#pragma once

/**
 * ASE GODOT ADAPTER - GDEXTENSION REGISTRATION
 *
 * @file        godot_registration.hpp
 * @brief       Entry function and module callbacks of the GDExtension library libase_adp_godot.so
 * @description Godot opens the library named in native/avt.gdextension and calls the function
 *              named there as entry_symbol: vivarium_library_init. It hands initializer and
 *              terminator to godot::GDExtensionBinding::InitObject; the initializer registers the
 *              class AseVivariumView at MODULE_INITIALIZATION_LEVEL_SCENE, the terminator removes
 *              it at the same level (godot_registration.cpp).
 *
 *              THE ENTRY HAS C++ LINKAGE. Godot looks the entry up by name in the library it
 *              opened, and that name is the mangled one of ase::adp::godot::vivarium_library_init.
 *              scripts/stage_native.py of the client reads it from the built library's dynamic
 *              symbol table and writes it into avt.gdextension. The symbol stays inside the
 *              adapter's namespace; no process-wide C name is taken.
 *
 *              NOT "register_types": godot-cpp's examples call this pair register_types, the plan
 *              took the name over (PLAN_ASE_VIVARIUM_PHASE_01_INTEG, Neue Dateien). In ASE a file
 *              ending in types.hpp IS the constant SSOT of its module - the validator and the
 *              codegen read it as one. This file holds no constant, so it carries the name of
 *              what it does.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-05
 * @version     00.00.01.00001
 */

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>

namespace ase::adp::godot {

/** Registers the class AseVivariumView once Godot initializes the scene level. */
void initialize_vivarium_module(::godot::ModuleInitializationLevel level);

/** Removes the class AseVivariumView again at the same level. */
void uninitialize_vivarium_module(::godot::ModuleInitializationLevel level);

/**
 * The library's entry, called once by Godot after it opened the library. Answers whether the
 * godot-cpp binding initialized; the classes follow at the scene level.
 */
GDExtensionBool GDE_EXPORT vivarium_library_init(GDExtensionInterfaceGetProcAddress get_proc_address,
                                                 GDExtensionClassLibraryPtr library,
                                                 GDExtensionInitialization* initialization);

}  // namespace ase::adp::godot
