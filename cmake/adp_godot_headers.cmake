#[[
ASE ADAPTER BUILD GROUP

@file        adp_godot_headers.cmake
@brief       The headers of the GDExtension library ase-adp-godot, attached to its target
@description Every header of the GDExtension library: the view and its registration, the HUD with
             its tools, containers, rings and pens, the 3D scene of the care view and the figures.
             Included by the CMakeLists.txt of the adapter after add_library(ase-adp-godot);
             include() runs in the scope of its caller, so every relative path resolves against the
             adapter directory. A header attached here is a header of the library, never a unit:
             no compiler command line changes with this list.

@module      ase-adp-godot
@layer       adapter (3rd-party isolation, consumed by L5 clients)
@actor       Unit Lifecycle Conductor (UNITLIFCON)
@usecase     usecase:100653 - publish the module catalog
@design      DSGN_021
@created     2026-11-21
@modified    2026-11-21
@version     00.00.26.00026 [init]

Copyright (c) Jan Ohlmann. All rights reserved.
Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
SPDX-License-Identifier: LicenseRef-ASE-LIC
]]

target_sources(ase-adp-godot PRIVATE
    include/ase/adp/godot/godot_registration.hpp
    include/ase/adp/godot/godot_vivarium_node.hpp
    include/ase/adp/godot/godot_vivarium_hud_bridge_read.hpp  # read_bridge_in: one hud_bridge_* token of SHA within its range, shared by the bridge units (usecase:101440, 06.5)
    include/ase/adp/godot/godot_vivarium_view_config.hpp  # VivariumLayout, VivariumModes, VivariumTexts, VivariumHudVerb (usecase:100387 view the plant)
    include/ase/adp/godot/godot_vivarium_figure_controls.hpp  # VivariumFigureControls: stick, orbit, the figure's focus (usecase:101554 receive player input)
    include/ase/adp/godot/godot_vivarium_hud_carry.hpp  # VivariumCarryBridge (usecase:101440 grow the interface with skills)
    include/ase/adp/godot/godot_vivarium_hud_hand.hpp   # VivariumHandBridge (usecase:100367 water a patch)
    include/ase/adp/godot/godot_vivarium_hud_loose.hpp  # VivariumLooseBridge (usecase:5939 adapt area terrain)
    include/ase/adp/godot/godot_hud_ui_rng.hpp          # ui_rng_draw, the mockup's seeded draw (usecase:101440 grow the interface with skills)
    include/ase/adp/godot/godot_vivarium_hud_sign_rules.hpp  # VivariumSignRules (usecase:100370 wilt from drought)
    include/ase/adp/godot/godot_vivarium_hud_trail.hpp  # VivariumTrailBridge (usecase:100390 return to the garden)
    include/ase/adp/godot/godot_vivarium_debug.hpp
    include/ase/adp/godot/godot_vivarium_watch.hpp  # VivariumMeasurement, VivariumGroundWatch (usecase:100387 view the plant)
    include/ase/adp/godot/godot_vivarium_scene_counts.hpp  # scene_counts, the scene's count line (usecase:100387 view the plant)
    include/ase/adp/godot/godot_vivarium_start_choice.hpp  # VivariumStartChoice: online/offline and new world/last state on the start screen (usecase:100387 view the plant)
    include/ase/adp/godot/godot_vivarium_start_mode.hpp    # GODOT_START_MODE_*, the four values of VivariumView::start_mode_ (usecase:100387 view the plant)
    include/ase/adp/godot/godot_asset_loader.hpp
    include/ase/adp/godot/godot_asset_showcase_scene.hpp
    include/ase/adp/godot/godot_plant_scene.hpp
    include/ase/adp/godot/godot_plant_scene_parts.hpp
    include/ase/adp/godot/godot_plant_environment.hpp
    include/ase/adp/godot/godot_plant_sky.hpp
    include/ase/adp/godot/godot_plant_sky_disc.hpp
    include/ase/adp/godot/godot_plant_stars.hpp
    include/ase/adp/godot/godot_plant_clouds.hpp
    include/ase/adp/godot/godot_plant_sun_halo.hpp
    include/ase/adp/godot/godot_plant_horizon_ridge.hpp
    include/ase/adp/godot/godot_plant_ground_season.hpp
    include/ase/adp/godot/godot_plant_leaf_season.hpp
    include/ase/adp/godot/godot_plant_brook_reflection.hpp
    include/ase/adp/godot/godot_plant_haze.hpp
    include/ase/adp/godot/godot_plant_brook.hpp
    include/ase/adp/godot/godot_plant_look.hpp
    include/ase/adp/godot/godot_plant_look_shader.hpp
    include/ase/adp/godot/godot_figure_scene.hpp  # FigureSceneLook, fill_figures, aim_figure_camera (usecase:101611 animate character motion)
    include/ase/adp/godot/godot_figure_load.hpp   # FigureSceneLoad, start_figure_load, take_figure_load (usecase:101611 animate character motion)
    include/ase/adp/godot/godot_camp_load.hpp     # CampMake, CampRoleLoad, start_camp_load, take_camp_load (usecase:5436 kindle fire)
    include/ase/adp/godot/godot_vivarium_camp.hpp # VivariumCampState, read_camp_config, restart_camp, end_camp (usecase:5436 kindle fire)
    include/ase/adp/godot/godot_vivarium_water_pending.hpp  # VivariumWaterPending, the watering the view waits on (usecase:100367 water a patch)
    include/ase/adp/godot/godot_vivarium_water_marks.hpp    # VivariumWaterMarks, the port generations the water was last laid from (usecase:100387 view the plant)
    include/ase/adp/godot/godot_figure_rig.hpp    # add_figure_motion, drive_figure_motion, figure_eye_level (usecase:101611 animate character motion)
    include/ase/adp/godot/godot_figure_hand.hpp   # hand_kind, carry_tool (usecase:101625 carry a tool in hand)
    include/ase/adp/godot/godot_figure_glass.hpp  # FigureSceneGlass, bind_figure_glass, flash_figure_edges (usecase:101612 keep figure in loaded area)
    include/ase/adp/godot/godot_figure_node.hpp   # holder, name prefix and metas of a figure's node, shared by figure scene and figure queries (usecase:101611 animate character motion)
    include/ase/adp/godot/godot_plant_shader.hpp
    include/ase/adp/godot/godot_config_read.hpp
    include/ase/adp/godot/godot_view_support.hpp
    include/ase/adp/godot/godot_hud_theme.hpp
    include/ase/adp/godot/godot_hud_ui_theme.hpp
    include/ase/adp/godot/godot_hud_item_widget.hpp
    include/ase/adp/godot/godot_hud_dibber_tool.hpp
    include/ase/adp/godot/godot_hud_digging_stick_tool.hpp
    include/ase/adp/godot/godot_hud_spade_tool.hpp
    include/ase/adp/godot/godot_hud_hoe_tool.hpp
    include/ase/adp/godot/godot_hud_rake_tool.hpp
    include/ase/adp/godot/godot_hud_level_tool.hpp
    include/ase/adp/godot/godot_hud_watering_pot.hpp
    include/ase/adp/godot/godot_hud_billhook_tool.hpp
    include/ase/adp/godot/godot_hud_knife_tool.hpp
    include/ase/adp/godot/godot_hud_auger_tool.hpp
    include/ase/adp/godot/godot_hud_spout_stack.hpp
    include/ase/adp/godot/godot_hud_peg_stack.hpp
    include/ase/adp/godot/godot_hud_bark_vessel.hpp
    include/ase/adp/godot/godot_hud_seed_stack.hpp
    include/ase/adp/godot/godot_hud_linen_pouch.hpp
    include/ase/adp/godot/godot_hud_leather_pack.hpp
    include/ase/adp/godot/godot_hud_compose.hpp
    include/ase/adp/godot/godot_hud_compose_frame.hpp  # the frame, the hit and the request between the view and VivariumHud (usecase:101440, 06.5)
    include/ase/adp/godot/godot_hud_compose_press.hpp  # HudPress: the press under way, written by the gestures, read by the refresh cadence (usecase:101440, 06.5, T10)
    include/ase/adp/godot/godot_hud_compose_share.hpp  # HudDrawShare, HudDrawStep: the steps of VivariumHud::draw and their CPU time (usecase:101440, P0692)
    include/ase/adp/godot/godot_hud_compose_place.hpp  # VivariumHudPlaces: the friend that names where a tap lands on each part of VivariumHud (usecase:101440, 06.5)
    include/ase/adp/godot/godot_hud_replay.hpp  # HudReplay: the mockup's lastKey and the clock the HUD's animations play from (usecase:101440, 06.5)
    include/ase/adp/godot/godot_hud_ui_icon.hpp
    include/ase/adp/godot/godot_hud_ui_anchor.hpp
    include/ase/adp/godot/godot_hud_ui_motion.hpp
    include/ase/adp/godot/godot_hud_ui_refresh.hpp
    include/ase/adp/godot/godot_hud_ui_pen.hpp
    include/ase/adp/godot/godot_hud_ui_pen_cover.hpp    # pen_core, pen_feather: the core and feather of every HUD stroke (usecase:101440, B0678)
    include/ase/adp/godot/godot_hud_ui_stroke.hpp       # ui_stroke: a polyline with round joins and caps as one antialiased mesh (usecase:101440, B0678)
    include/ase/adp/godot/godot_hud_ui_ring.hpp
    include/ase/adp/godot/godot_hud_ui_leader_line.hpp
    include/ase/adp/godot/godot_hud_ui_text_block.hpp
    include/ase/adp/godot/godot_hud_ui_spine.hpp
    include/ase/adp/godot/godot_hud_ui_hex_chip.hpp
    include/ase/adp/godot/godot_hud_ui_edge_rail.hpp
    include/ase/adp/godot/godot_hud_survival_body_rail.hpp
    include/ase/adp/godot/godot_hud_survival_warmth_scale.hpp
    include/ase/adp/godot/godot_hud_survival_rail_states.hpp
    include/ase/adp/godot/godot_hud_survival_fire_card.hpp
    include/ase/adp/godot/godot_hud_survival_rest_card.hpp
    include/ase/adp/godot/godot_hud_survival_air_mark.hpp
    include/ase/adp/godot/godot_hud_survival_spoil_ring.hpp
    include/ase/adp/godot/godot_hud_survival_water_mark.hpp
    include/ase/adp/godot/godot_hud_ui_glyph_chain.hpp
    include/ase/adp/godot/godot_hud_ui_item_shadow.hpp
    include/ase/adp/godot/godot_hud_ui_item_sprite.hpp
    include/ase/adp/godot/godot_hud_ui_svg_path.hpp
    include/ase/adp/godot/godot_hud_ui_word_fill.hpp
    include/ase/adp/godot/godot_hud_container_def.hpp
    include/ase/adp/godot/godot_hud_ui_svg_paint.hpp
    include/ase/adp/godot/godot_hud_bag_contents.hpp
    include/ase/adp/godot/godot_hud_bag_window_look.hpp
    include/ase/adp/godot/godot_hud_bag_window.hpp
    include/ase/adp/godot/godot_hud_belt_holster.hpp
    include/ase/adp/godot/godot_hud_bag_icon.hpp
    include/ase/adp/godot/godot_hud_fit_check.hpp
    include/ase/adp/godot/godot_hud_screen_shade.hpp
    include/ase/adp/godot/godot_hud_select_label.hpp
    include/ase/adp/godot/godot_hud_weather_veil.hpp
    include/ase/adp/godot/godot_hud_cloud_bank.hpp
    include/ase/adp/godot/godot_hud_sky_overcast.hpp
    include/ase/adp/godot/godot_hud_horizon_ridge.hpp
    include/ase/adp/godot/godot_hud_ground_season.hpp
    include/ase/adp/godot/godot_hud_leaf_season.hpp
    include/ase/adp/godot/godot_hud_brook_reflection.hpp
    include/ase/adp/godot/godot_hud_root_cut.hpp
    include/ase/adp/godot/godot_hud_world_drop.hpp
    include/ase/adp/godot/godot_hud_tool_rack.hpp
    include/ase/adp/godot/godot_hud_tap_hole.hpp
    include/ase/adp/godot/godot_hud_sow_hole.hpp
    include/ase/adp/godot/godot_hud_tree_shade.hpp
    include/ase/adp/godot/godot_hud_toast.hpp
    include/ase/adp/godot/godot_hud_drag_layer.hpp
    include/ase/adp/godot/godot_hud_sky_band.hpp
    include/ase/adp/godot/godot_hud_year_ring.hpp
    include/ase/adp/godot/godot_hud_day_ring.hpp
    include/ase/adp/godot/godot_hud_forecast_strip.hpp
    include/ase/adp/godot/godot_hud_hourglass.hpp
    include/ase/adp/godot/godot_hud_calendar_sheet.hpp
    include/ase/adp/godot/godot_hud_focus_ring.hpp
    include/ase/adp/godot/godot_hud_focus_cluster.hpp
    include/ase/adp/godot/godot_hud_action_fan.hpp
    include/ase/adp/godot/godot_hud_figure_focus.hpp
    include/ase/adp/godot/godot_hud_figure_switch.hpp
    include/ase/adp/godot/godot_hud_figure_turn.hpp
    include/ase/adp/godot/godot_hud_figure_zoom.hpp
    include/ase/adp/godot/godot_hud_sign_marker.hpp
    include/ase/adp/godot/godot_hud_sign_reading.hpp
    include/ase/adp/godot/godot_hud_hand_slot.hpp
    include/ase/adp/godot/godot_hud_hand_mode.hpp
    include/ase/adp/godot/godot_hud_ui_ground.hpp
    include/ase/adp/godot/godot_hud_ground_reticle.hpp
    include/ase/adp/godot/godot_hud_ground_crumbs.hpp
    include/ase/adp/godot/godot_hud_terrain_kit.hpp
    include/ase/adp/godot/godot_hud_prune_marks.hpp
    include/ase/adp/godot/godot_hud_trail_view.hpp
    include/ase/adp/godot/godot_hud_system_ring.hpp
    include/ase/adp/godot/godot_hud_debug_layer.hpp
)
