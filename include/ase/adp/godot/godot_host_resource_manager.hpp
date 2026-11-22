#pragma once

/**
 * ASE RESOURCE MANAGER (NOT A COMPONENT!)
 *
 * @file        godot_host_resource_manager.hpp
 * @brief       GodotHostResourceManager - collector of the views the external resource manager of the embedded ASE host keeps
 * @description Collects the sub-headers this header was cut into by purpose and reader circle
 *              (WRFL_ASE_TIER_WIPE, the header ratchet, rule 6): start configuration, terrain, stream,
 *              templates, organs, bark, bundle, self-test, patch, clock, sky, seed, rack, fire and figure.
 *              Every reader includes the sub-header of the types it uses directly, never this collector
 *              (rule 4). The manager itself - the ONE std::unique_ptr<ase::kernel::KernelEmbeddedHost> of a
 *              Vivarium view, its start configuration, its bundle, the views of the last snapshots and the
 *              queue of host log lines - stands in godot_host_resource_manager_class.hpp. It lives OUTSIDE
 *              every ECS registry and carries no Godot type.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-10-05
 * @modified    2026-11-21
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * FLYWEIGHT PATTERN (INST_ASE_ECS_SER)
 *
 *   Godot node (OOP)             ResourceManager (NOT ECS!)
 *   ┌────────────────┐          ┌────────────────────────────────────┐
 *   │ uint32_t       │ ───────> │ map<uint32_t, VivariumPatchView>   │
 *   │ selected_id    │          │ unique_ptr<KernelEmbeddedHost>     │
 *   └────────────────┘          └────────────────────────────────────┘
 *                                          ▲
 *   Host log sink (any thread)             │
 *   ┌────────────────┐                     │
 *   │ queue_log_line │ ────────────────────┘
 *   │                │   under log_mutex_, drained by the node
 *   └────────────────┘   .get_patch(selected_id)
 *
 * USAGE:
 *   // VivariumView::ready() (Godot's _ready) - play mode only, never in the editor
 *   GodotHostResourceManager resources;          // member of the view: no global, no singleton
 *   resources.boot(config);                      // factory: create, data root, load the set, start
 *
 *   // VivariumView::process (Godot's _process)
 *   resources.advance(elapsed_s);                // the real time since the last frame, never capped
 *   const VivariumPatchView* patch = resources.get_patch(selected_id);
 *
 *   // Reiniciar and VivariumView::exit_tree() (Godot's _exit_tree)
 *   resources.clear_all();                       // the binding teardown, idempotent
 *
 * THIS IS NOT A COMPONENT!
 * This class holds an external C++ object (the embedded host, its units, its logger). The node
 * stores ONLY uint32_t object ids, the manager owns everything behind them.
 *
 * ECS RESOURCE MANAGER HEADER COMPLIANCE
 *
 * [ ] NOT a Component - lives outside ECS registry
 * [ ] Accessed via registry.ctx().get<ResourceManager&>()
 * [ ] Components store ONLY uint32_t IDs
 * [ ] Header contains ONLY declarations (implementations in .cpp!)
 * [ ] Thread-safe method signatures (const where possible)
 * [ ] store_*(), get_*(), remove_*(), has_*() method pattern
 * [ ] clear_all() for shutdown cleanup
 * [ ] Private mutex for thread safety
 * [ ] Private maps for resource storage
 * [ ] NO inline implementations (prevents mass rebuilds!)
 */

#include <ase/adp/godot/godot_host_bark_view.hpp>
#include <ase/adp/godot/godot_host_bundle_info.hpp>
#include <ase/adp/godot/godot_host_clock_view.hpp>
#include <ase/adp/godot/godot_host_figure_view.hpp>
#include <ase/adp/godot/godot_host_fire_view.hpp>
#include <ase/adp/godot/godot_host_organ_view.hpp>
#include <ase/adp/godot/godot_host_patch_view.hpp>
#include <ase/adp/godot/godot_host_rack_view.hpp>
#include <ase/adp/godot/godot_host_seed_view.hpp>
#include <ase/adp/godot/godot_host_self_test.hpp>
#include <ase/adp/godot/godot_host_sky_view.hpp>
#include <ase/adp/godot/godot_host_start_config.hpp>
#include <ase/adp/godot/godot_host_stream_view.hpp>
#include <ase/adp/godot/godot_host_template_view.hpp>
#include <ase/adp/godot/godot_host_terrain_view.hpp>
