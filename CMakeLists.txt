cmake_minimum_required(VERSION 3.25)

#[[
ASE GODOT ADAPTER BUILD CONFIGURATION

@file        CMakeLists.txt
@brief       Build configuration for ase-adp-godot
@description 3rd-party isolation module for Godot 4.5 (godot-cpp). The ONLY place that registers a
             Godot class (AseVivariumView, through the GDExtension class interface - no C++ type
             derives from a Godot type) and that holds the GDExtension entry
             ase::adp::godot::vivarium_library_init (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.2).

             Built inside the embedded closure of clients/avt-client-android: it binds godot::cpp
             and ase::kernel-embedded - never a unit of the set the host loads, never a module or
             plugin header. The host part (GodotHostResourceManager) is a separate static library
             WITHOUT any Godot type, so its tests run without a Godot process.

@module      ase-adp-godot
@layer       adapter (3rd-party isolation, consumed by L5 clients)
@design      DSGN_021
@created     2026-10-05
@modified    2026-11-10
]]

#[[ VERBOTEN: no module or plugin header and no unit of the embedded set - the adapter binds
    godot::cpp, ase::kernel-embedded and header-only L0 libraries only, never ase::log (the log
    runtime lives once, inside libase_kernel_embedded.so) ]]

# Parse local VERSION (SSOT) if not building from root
if(NOT DEFINED ASE_ADP_GODOT_VERSION)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
        message(FATAL_ERROR "VERSION file not found: ${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    endif()
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _VERSION_LINES)
    foreach(_LINE ${_VERSION_LINES})
        if(_LINE MATCHES "^ASE_ADP_GODOT=(.*)$")
            set(ASE_ADP_GODOT_VERSION "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(NOT DEFINED ASE_ADP_GODOT_VERSION)
        message(FATAL_ERROR "ASE_ADP_GODOT version not found in VERSION file")
    endif()
endif()

project(ase-adp-godot VERSION ${ASE_ADP_GODOT_VERSION} LANGUAGES CXX)

message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")

# ── Inputs from the embedding closure - named, never silently replaced ─────
foreach(_adp_godot_dep godot::cpp ase::kernel-embedded ase::fileio ase::utils ase::math tomlplusplus::tomlplusplus)
    if(NOT TARGET ${_adp_godot_dep})
        message(FATAL_ERROR "ase-adp-godot: target ${_adp_godot_dep} missing - the embedding CMake (clients/avt-client-android) adds it before this adapter")
    endif()
endforeach()

# ── Host part: one KernelEmbeddedHost behind a resource manager, the placement of the view's layout
#    on a surface of any aspect ratio, the plant geometry from the organ views and the template ports, the visible orders by
#    frame time and the asset binding (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1, 03.2) - no Godot type
#    in any of them, so all are tested without a Godot process ──
add_library(ase-adp-godot-host STATIC
    src/godot_host_resource_manager.cpp
    src/godot_host_record.cpp        # the value under a key of a port record (usecase:101344 carry host records to app)
    src/godot_organ_records.cpp      # organ, bark, tile, sway and instance records as views (usecase:100974 lay out organ records)
    src/godot_terrain_samples.cpp    # terrain records as ground samples (usecase:100971 lay out terrain samples)
    src/godot_view_layout.cpp
    src/godot_plant_geometry.cpp
    src/godot_plant_instances.cpp    # the instance buffers of the plants (usecase:101348 hand organ instances to app, 03.11)
    src/godot_plant_templates.cpp    # the organ templates from the template ports (usecase:101342 hand organ shapes to app, 03.11)
    src/godot_plant_ground.cpp       # the ground mesh and the marks of its shader (usecase:101339 show the ground around plant, 03.8)
    src/godot_plant_water.cpp        # the ribbon of water in every bed (usecase:100399 carry brook water, 03.11)
    src/godot_scene_exposure.cpp     # the exposure of the scene camera for the sky's light (usecase:101415 hand sky light to app, D1721)
    src/godot_order_budget.cpp
    src/godot_cycle_measure.cpp      # the measurement line of one cycle, also compiled into avt-headless (03.6)
    src/godot_garden_return.cpp      # the return from the background and its catch-up (usecase:100390 return to the garden, 03.6)
    src/godot_patch_focus.cpp        # the focused patch and the camera's near view onto it (usecase:100386 select a patch, 03.1)
    src/godot_new_seed.cpp           # the seed of every sowing, a new one with every Reiniciar (usecase:100388 sow a new seed, 03.1)
    src/godot_client_window.cpp      # whether the client window opened and its log line (usecase:101019 start the client window)
    src/godot_asset_binding.cpp
    src/godot_asset_showcase.cpp     # the plan of the asset showcase (PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7)
    src/godot_hud_context_resolver.cpp  # the brain of the HUD: mode, layers, fan, signs, offers, level (usecase:101440 grow the interface with skills, 06.2)
    src/godot_hud_context.cpp           # the rules of the chip spine the six buttons still run on, until ContextResolver carries them (usecase:101440, 06.11)

    include/ase/adp/godot/godot_host_resource_manager.hpp
    include/ase/adp/godot/godot_host_record.hpp
    include/ase/adp/godot/godot_organ_records.hpp
    include/ase/adp/godot/godot_terrain_samples.hpp
    include/ase/adp/godot/godot_view_layout.hpp
    include/ase/adp/godot/godot_plant_geometry.hpp
    include/ase/adp/godot/godot_plant_instances.hpp
    include/ase/adp/godot/godot_plant_templates.hpp
    include/ase/adp/godot/godot_plant_ground.hpp
    include/ase/adp/godot/godot_plant_water.hpp
    include/ase/adp/godot/godot_scene_exposure.hpp
    include/ase/adp/godot/godot_order_budget.hpp
    include/ase/adp/godot/godot_cycle_measure.hpp
    include/ase/adp/godot/godot_garden_return.hpp
    include/ase/adp/godot/godot_patch_focus.hpp
    include/ase/adp/godot/godot_new_seed.hpp
    include/ase/adp/godot/godot_client_window.hpp
    include/ase/adp/godot/godot_hud_context_resolver.hpp
    include/ase/adp/godot/godot_hud_context.hpp
    include/ase/adp/godot/godot_asset_binding.hpp
    include/ase/adp/godot/godot_asset_showcase.hpp
    include/ase/adp/godot/types.hpp
)
add_library(ase::adp::godot-host ALIAS ase-adp-godot-host)

target_include_directories(ase-adp-godot-host
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(ase-adp-godot-host PUBLIC cxx_std_20)
target_link_libraries(ase-adp-godot-host
    PUBLIC
        ase::kernel-embedded   # host facade and port contract
    PRIVATE
        ase::fileio            # /proc/self/smaps_rollup for the PSS of acceptance A10 (header-only, L0)
        ase::math              # floor of the whole-number test of a record's place and codes (header-only, L0)
        ase::utils             # str_copy / str_equal for the port keys, number text (header-only, L0)
        tomlplusplus::tomlplusplus  # the self-test table config/vivarium_selftest.toml (header-only)
)
# NEVER ase::log here: the log runtime exists ONCE, linked whole into libase_kernel_embedded.so.
# A second copy in this library would own a logger slot nobody installs - the adapter's own lines
# go through KernelEmbeddedHost::note instead. On Android the sink writes to logcat itself
# (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.1): the platform's log library, bound here and nowhere
# else - it is a system library of every Android, never shipped in the APK.
if(ANDROID)
    find_library(ASE_ADP_GODOT_ANDROID_LOG log REQUIRED)
    target_link_libraries(ase-adp-godot-host PRIVATE ${ASE_ADP_GODOT_ANDROID_LOG})
endif()

# ── GDExtension library loaded by Godot ─────────────────────────────────────
add_library(ase-adp-godot SHARED
    src/godot_registration.cpp   # GDExtension entry vivarium_library_init + class binding
    src/godot_vivarium_node.cpp  # VivariumView: lifecycle, host boot, frame, log
    src/godot_vivarium_config.cpp       # VivariumView: start configuration, stage, modes, view section
    src/godot_vivarium_host_config.cpp  # VivariumView: ports, operations, patches, ground, brooks
    src/godot_vivarium_layout.cpp       # VivariumView: logical surface, bands, places, buttons, texts
    src/godot_vivarium_assets.cpp       # VivariumView: asset list of the play stage and its manifest (03.2)
    src/godot_vivarium_input.cpp        # VivariumView: taps and the selection of a patch
    src/godot_vivarium_water.cpp        # VivariumView: Regar, the pour to the patch port
    src/godot_vivarium_level.cpp        # VivariumView: Nivelar, the level request to the level port (03.8)
    src/godot_vivarium_prune.cpp        # VivariumView: cutting mode, the internode under the finger, the cut
    src/godot_vivarium_bed.cpp          # VivariumView: bed mode, the ground under the finger, the bed the player lays (03.8)
    src/godot_vivarium_flatten.cpp      # VivariumView: flatten mode, the disc the player levels where he taps (03.8)
    src/godot_vivarium_debug.cpp        # VivariumView: debug buttons, the time lapse of this host's clock (usecase:101120 clock the growth cycle)
    src/godot_vivarium_clock.cpp        # VivariumView: the opening of every boot and the debug seek to an hour of the game day (usecase:101120)
    src/godot_vivarium_jump.cpp         # VivariumView: the debug jump to a stage, an hour or a SURA date (usecase:101120 clock the growth cycle)
    src/godot_vivarium_date.cpp         # VivariumView: the free date entry of the debug mode, any SURA date ahead or back (usecase:101120)
    src/godot_vivarium_date_type.cpp    # VivariumView: the typed date entry, a field picked and its value typed key by key (usecase:101120)
    src/godot_vivarium_moonrise.cpp     # VivariumView: the debug jump to the next rise of a moon into the fixed view (usecase:100390)
    src/godot_vivarium_rewind.cpp       # VivariumView: the free date entry's jump behind the clock, a rewound host and the seek ahead (usecase:101120)
    src/godot_vivarium_draw.cpp        # VivariumView: placement on the visible surface and the canvas
    src/godot_vivarium_scene.cpp        # VivariumView: what a frame fills of the 3D scene (03.1)
    src/godot_vivarium_measure.cpp      # VivariumView: self-test and measurement modes (02.3)
    src/godot_vivarium_window.cpp       # VivariumView: the client window it opened in (usecase:101019 start the client window)
    src/godot_vivarium_hud.cpp          # VivariumView: the HUD's frame from the ports and its drawing (usecase:101440, 06.5)
    src/godot_vivarium_hud_config.cpp   # VivariumView: the HUD's configuration at load and the still mode of the platform (usecase:101440, 06.5, T09)
    src/godot_vivarium_hud_world.cpp    # VivariumView: the frames of the blocks placed in the world, the cut under the trunk among them (usecase:101440, 06.5)
    src/godot_vivarium_hud_bridge.cpp   # VivariumView: the mockup's values the bridge writes while a port is silent (usecase:101440, 06.5)
    src/godot_vivarium_hud_signs.cpp    # VivariumView: the signs the mockup's signs() reports while no port reports a sign case (usecase:100370, 06.10)
    src/godot_vivarium_hud_hand.cpp     # VivariumHandBridge: the hand's readings by the mockup's rules while their ports are silent (usecase:100367, 06.4)
    src/godot_vivarium_hud_loose.cpp    # VivariumLooseBridge: the mockup's loosened soil point by point while flora.patch.v1 is silent, N0456 (usecase:5939, 06.4 N08)
    src/godot_vivarium_hud_sign_rules.cpp  # VivariumSignRules: the thresholds and values of the mockup's signs() while no port reports a sign (usecase:100370, 06.10)
    src/godot_vivarium_hud_trail.cpp    # VivariumTrailBridge: the mockup's S.log while flora.trail.v1 files no event (usecase:100390, 06.4)
    src/godot_vivarium_hud_carry.cpp    # VivariumCarryBridge: the mockup's carry rules and acts while flora.stow.v1 judges no move (usecase:101440, 06.7-06.9)
    src/godot_vivarium_hud_carry_frame.cpp  # VivariumCarryBridge: the mockup's inventory read from SHA into the HUD's frame (usecase:101440, 06.7-06.9)
    src/godot_vivarium_hud_input.cpp   # VivariumView: the touches through the HUD, its clock and its requests over the ports (usecase:101440, 06.5)
    src/godot_vivarium_hud_place.cpp    # VivariumView: the places a tap reaches the HUD through, logged on change (usecase:101440, 06.5/06.11)
    src/godot_vivarium_return.cpp       # VivariumView: background, foreground and the return to the garden, the HUD opens on the trail (usecase:100390, 03.6/06.4 A08)
    src/godot_config_read.cpp           # readers of one key of the start configuration
    src/godot_view_support.cpp          # text conversion, font and editor check of the view
    src/godot_hud_theme.cpp             # the section hud: colours, strokes, sizes and chips of the HUD (usecase:101440, 06.1)
    src/godot_hud_ui_theme.cpp          # UiTheme: SHA's colours, tokens, words and fonts, the zones and the items with their widget class (usecase:101440, 06.1)
    src/godot_hud_ui_theme_spec.cpp     # UiTheme::read_spec: the brain rules of the section hud, modes, layer rules, tools, fan, signs, perception (usecase:101440, 06.1)
    src/godot_hud_item_widget.cpp       # ItemWidget: the item of a widget class, its drawing, tone, words, reading, count and gap mark (usecase:101440, 06.7)
    src/godot_hud_dibber_tool.cpp       # DibberTool: the plantador, the tool of the mode sow (usecase:100388 sow a new seed, 06.7)
    src/godot_hud_digging_stick_tool.cpp  # DiggingStickTool: the grabstock, ground verb furar (usecase:5939 adapt area terrain, 06.7)
    src/godot_hud_spade_tool.cpp        # SpadeTool: the pa, ground verbs cavar, amontoar, leito (usecase:5939, 06.7)
    src/godot_hud_hoe_tool.cpp          # HoeTool: the enxada, ground verbs soltar, amontoar (usecase:5939, 06.7)
    src/godot_hud_rake_tool.cpp         # RakeTool: the ancinho, ground verb aplanar (usecase:100389 level the ground, 06.7)
    src/godot_hud_level_tool.cpp        # LevelTool: the nivel, ground verbs nivelar, medir (usecase:100389, 06.7)
    src/godot_hud_watering_pot.cpp      # WateringPot: the regador, its water line from the vessel state, gap N0451 (usecase:100367 water a patch, 06.7)
    src/godot_hud_billhook_tool.cpp     # BillhookTool: the podao, the tool of the mode prune (usecase:100365 prune a branch, 06.7)
    src/godot_hud_knife_tool.cpp        # KnifeTool: the faca, held, no care case of its own (usecase:101440, 06.7)
    src/godot_hud_auger_tool.cpp        # AugerTool: the trado, the tool of the mode sap (usecase:101609 tap birch sap, 06.7)
    src/godot_hud_spout_stack.cpp       # SpoutStack: the bica, a stack with its count, gap N0451 (usecase:101609 tap birch sap, 06.7)
    src/godot_hud_peg_stack.cpp         # PegStack: the cavilha, a stack of the mode plug with its count, gap N0451 (usecase:101609 tap birch sap, 06.7)
    src/godot_hud_bark_vessel.cpp       # BarkVessel: the cocho, its sap body from the vessel state, gaps N0451, N0457 (usecase:101609 tap birch sap, 06.7)
    src/godot_hud_seed_stack.cpp        # SeedStack: the sementes, a stack in the pouch with its count, gap N0451 (usecase:100388 sow a new seed, 06.7)
    src/godot_hud_linen_pouch.cpp       # LinenPouch: the bolsa, its weight with what it holds, double tap opens it, gap N0451 (usecase:100388 sow a new seed, 06.7)
    src/godot_hud_leather_pack.cpp      # LeatherPack: the ranzen, worn, its load, tap opens it, gap N0451 (usecase:101440, 06.7)
    src/godot_hud_compose.cpp           # VivariumHud: binds every block, resolves the mode, fills the frame (usecase:101440, 06.5)
    src/godot_hud_compose_draw.cpp      # VivariumHud: the veil, the world's things, the layers of the mode, the toast (usecase:101440, 06.5)
    src/godot_hud_compose_hit.cpp       # VivariumHud: what a touch hits, topmost layer down to the ground (usecase:101440, 06.5)
    src/godot_hud_compose_gesture.cpp   # VivariumHud: press, move, release, tick and the mockup's acts (usecase:101440, 06.5)
    src/godot_hud_compose_place.cpp     # VivariumHudPlaces: where a tap lands, the places the place lines name (usecase:101440, 06.5)
    src/godot_hud_replay.cpp            # HudReplay: the mockup's lastKey, when fades and drawings play again (usecase:101440, 06.5)
    src/godot_hud_ui_icon.cpp          # UiIcon: an ICON_* of ui_icons.json in the Nerd Font, centred as the mockup's G (usecase:101440, 06.3)
    src/godot_hud_ui_anchor.cpp         # UiAnchor: a thing, crown, trunk or brook point through the scene camera onto the HUD surface (usecase:101440, 06.3)
    src/godot_hud_ui_motion.cpp         # UiMotion: duration and CSS easing of a hud_anim_* token, the one curve of fade, draw and pulse (usecase:101440, 06.3)
    src/godot_hud_ui_refresh.cpp        # UiRefresh: hud_refresh_ms and hud_frame_dt_max, when the HUD samples the simulation again and the longest step of its clock (usecase:101440, 06.3, T10)
    src/godot_hud_ui_pen.cpp            # UiPen: every HUD shape laid in mockup units, rendered anew at the screen's pixels (usecase:101440, 06.3)
    src/godot_hud_ui_ring.cpp           # UiRing: ring, arc from start to end angle, stepped arc, pulse (usecase:101440, 06.3)
    src/godot_hud_ui_leader_line.cpp    # UiLeaderLine: polyline from anchor over knee to row, growing or dashed (usecase:101440, 06.3)
    src/godot_hud_ui_text_block.cpp     # UiTextBlock: one shaped line of t, tb or d with tracking, shadow and fade (usecase:101440, 06.3)
    src/godot_hud_ui_spine.cpp          # UiSpine: the focus column of rows with dot, tick, label, value, bar, pips (usecase:101440, 06.3)
    src/godot_hud_ui_hex_chip.cpp       # UiHexChip: the hexagon chip HX with a glyph and HXI around a thing, with tap target (usecase:101440, 06.3)
    src/godot_hud_ui_edge_rail.cpp      # UiEdgeRail: the rail of signs at the left edge with alert bar, pulse and system glyph (usecase:101440, 06.3)
    src/godot_hud_ui_glyph_chain.cpp    # UiGlyphChain: the day's phases on the sky band with daylight and the wandering sun or moon (usecase:101440, 06.3)
    src/godot_hud_ui_item_shadow.cpp    # UiItemShadow: the drop-shadow of a thing's drawing, its mask blurred by a Gaussian and kept as a texture (usecase:101440, 06.3)
    src/godot_hud_ui_item_sprite.cpp    # UiItemSprite: a carried thing's SVG paths from item_art.json, fitted into its box with fill, stroke and level (usecase:101440, 06.3)
    src/godot_hud_ui_svg_path.cpp       # read_svg_path: SVG path data and the curve pieces of every HUD shape, flattened by one rule (usecase:101440, 06.3)
    src/godot_hud_ui_word_fill.cpp      # fill_words: the one filler of a word's {name} port values, refusing an unfilled one (usecase:101440, 06.0)
    src/godot_hud_container_def.cpp     # ContainerDef: a container's drawing side from SHA and its physical side from flora.carry.v1, bridge gap N0451 (usecase:101440, 06.7)
    src/godot_hud_ui_svg_paint.cpp      # svg_fill, svg_stroke, svg_stroke_dashed: the one painter of a flattened outline and the one dash rule (usecase:101440, 06.3)
    src/godot_hud_bag_contents.cpp      # BagContents: the things in one open container, unturned where let go, shadow, selection frame, stack count, hit (usecase:101440, 06.7)
    src/godot_hud_bag_window_look.cpp   # read_bag_window_look: the measures, frame limits and colours of the pack and pouch windows from SHA (usecase:101440, 06.7)
    src/godot_hud_bag_window.cpp        # BagWindow: the leather pack and the linen pouch opened, drag, hit, drop zones, load and mouth, bridge gap N0451 (usecase:101440, 06.7)
    src/godot_hud_belt_holster.cpp      # BeltHolster: the belt strap and its two holsters, one tool each, hit and drop, bridge gap N0451 (usecase:101440, 06.7)
    src/godot_hud_bag_icon.cpp          # BagIcon: the leather pack closed at the hip, tap to open, drop place, load from levels[1], bridge gap N0451 (usecase:101440, 06.7)
    src/godot_hud_fit_check.cpp         # FitCheck: at the finger, the place a drag goes or the module's refusal, bridge gap N0452 (usecase:101440, 06.7)
    src/godot_hud_screen_shade.cpp      # ScreenShade: the shade over the scene and under every overlay, vignette, head and foot of the screen (usecase:101440, 06.5)
    src/godot_hud_select_label.cpp      # SelectLabel: the reading of a single tap, leader, name, facts from levels[1], hint, asset from levels[2], bridge gap N0451 (usecase:101440, L01)
    src/godot_hud_weather_veil.cpp      # WeatherVeil: fog, rain, drizzle and snow falling through the scene, bridge gap N0449 (usecase:101440, 06.5)
    src/godot_hud_cloud_bank.cpp        # CloudBank: the cloud banks of an overcast sky, laid for the 3D sky, bridged N0449 (usecase:101440, 06.4 N04)
    src/godot_hud_sky_overcast.cpp      # SkyOvercast: the grey and the dimmed light of an overcast sky over the 3D scene, bridged N0449 (usecase:5606, 06.4 N01)
    src/godot_hud_horizon_ridge.cpp     # HorizonRidge: the two ridges of hills on the horizon, derived for the 3D scene (usecase:5606, 06.4 N05)
    src/godot_hud_ground_season.cpp     # GroundSeason: the colour of the ground by season, snow, depth and horizon (usecase:100387, 06.4 N06)
    src/godot_hud_leaf_season.cpp       # LeafSeason: the colour of the leaf by season and drought (usecase:100387, B0278)
    src/godot_hud_brook_reflection.cpp  # BrookReflection: the sky the brook reflects and the strokes of its current (usecase:101339, 06.4 N07)
    src/godot_hud_root_cut.cpp          # RootCut: the cut under the trunk in the mode water, soil water, pour and roots, always whole, the bridge fills N0454 and N0463 (usecase:100367, 06.5)
    src/godot_hud_world_drop.cpp        # WorldDrop: the things on the ground at their world place, darkened with the light, hits, bridge gap N0451 (usecase:101440, 06.7)
    src/godot_hud_tool_rack.cpp         # ToolRack: the tool place in the world, its list in the mode rack and the drop on it, bridge gap N0451 (usecase:101440, 06.7)
    src/godot_hud_tap_hole.cpp          # TapHole: the elder spout, the bark vessel at the foot and the drop while a tap runs, bridge gap N0457 (usecase:101609, 06.7 N13)
    src/godot_hud_sow_hole.cpp          # SowHole: the hole the dibber made until the seed closes it, bridge gap N0451 (usecase:100388, 06.7 N14)
    src/godot_hud_tree_shade.cpp        # TreeShade: the shade under the birch by the stage scale, or the seed mound (usecase:100387, 06.7 N19)
    src/godot_hud_toast.cpp             # Toast: the short message over the hand in the domain colour its sender names (usecase:101440, 06.4)
    src/godot_hud_drag_layer.cpp        # DragLayer: the ghost under the finger, the place it stands over, the verdict and the release, bridge gap N0452 (usecase:101440, 06.7)
    src/godot_hud_sky_band.cpp          # SkyBand: the sky band at the top, the day's phases, the light and the wandering sun or moon, bridge gap N0460 (usecase:5606 observe the sky, 06.4)
    src/godot_hud_year_ring.cpp         # YearRing: the year of the calendar sheet, seasons, events and the next stage, bridge gap N0460, N0450, N0454 (usecase:5606 observe the sky, 06.4)
    src/godot_hud_day_ring.cpp          # DayRing: the day of the calendar sheet, light, six phases, sun or moon and the date, bridge gap N0460 (usecase:5606 observe the sky, 06.4)
    src/godot_hud_forecast_strip.cpp    # ForecastStrip: the next phases with glyph, temperature and millimetres, bridge gap N0449 (usecase:5606 observe the sky, 06.4)
    src/godot_hud_hourglass.cpp         # Hourglass: the time lapse of the game range as a row of factor chips, bridge gap N0459 (usecase:101120 clock the growth cycle, 06.4)
    src/godot_hud_calendar_sheet.cpp    # CalendarSheet: the sky mode's sheet of year ring, day ring, weather strip and hourglass (usecase:5606 observe the sky, 06.4)
    src/godot_hud_focus_ring.cpp        # FocusRing: the ring of the focus, ticks, arcs of soil water and growth, the stage glyph, bridge gap N0454 (usecase:100387 view the plant, 06.4)
    src/godot_hud_focus_cluster.cpp     # FocusCluster: head, spine rows and stage forecast around the chosen plant, bridge gap N0454, N0456, N0457 (usecase:100387 view the plant, 06.4)
    src/godot_hud_action_fan.cpp        # ActionFan: the fan of tools beside the focus ring, take into the hand or open the tool place (usecase:100387 view the plant, 06.4)
    src/godot_hud_sign_marker.cpp       # SignMarker: the glyph of each sign at its place, pulse, tap, bridge gap N0461, N0449 (usecase:100370 wilt from drought, 06.4)
    src/godot_hud_sign_reading.cpp      # SignReading: the open sign's leader, title, sentence, bar, reading, case and offer, bridge gap N0451 (usecase:100370 wilt from drought, 06.4)
    src/godot_hud_hand_slot.cpp         # HandSlot: the hand, the thing in it, its name, instruction, case, reading and stow button, bridge gap N0451 (usecase:100367 water a patch, 06.4)
    src/godot_hud_hand_mode.cpp         # HandMode: what each mode adds to the hand, pour arcs, rain, verbs, readings, bridge gap N0454, N0449, N0462, N0461, N0463, N0457 (usecase:100367 water a patch, 06.4)
    src/godot_hud_ui_ground.cpp         # UiGround: the ground as the terrain port laid it, height and point on the HUD at a world place (usecase:5939 adapt area terrain, 06.3)
    src/godot_hud_ground_reticle.cpp    # GroundReticle: the brush ring in the breadth of the verb, the level patch, the first point of a bed (usecase:5939 adapt area terrain, 06.4)
    src/godot_hud_ground_crumbs.cpp     # GroundCrumbs: the crumbs of loosened soil in every mode, m.html drawGround 441 to 444 (usecase:5939 adapt area terrain, 06.4 N08)
    src/godot_hud_terrain_kit.cpp       # TerrainKit: the ground mode, lines of the mesh, brook arrows, root zone and reticle, always whole, the bridge fills N0463 (usecase:5939 adapt area terrain, 06.4)
    src/godot_hud_prune_marks.cpp       # PruneMarks: the marks at the internodes that may be cut, pulse, disc, glyph, hit, always whole, the bridge fills N0462 (usecase:100365 prune a branch, 06.4)
    src/godot_hud_trail_view.cpp        # TrailView: the trail on arrival, dim, title, span, spine of the last events, play chip, bridge gap N0450 (usecase:100390 return to the garden, 06.4)
    src/godot_hud_system_ring.cpp       # SystemRing: the mode sys, dim, hold ring with its arc toward the reset, words, close (usecase:101019 start the client window, 06.4)
    src/godot_hud_debug_layer.cpp       # DebugLayer: three status lines, the handle, the drawer with title, fold, tabs, chips and note (usecase:101120 clock the growth cycle, 06.4)
    src/godot_hud_debug_layer_lay.cpp   # DebugLayer::lay: where the tabs, the chips of TEMPO, SALTO, DATA, RELEVO and the date fields stand
    src/godot_asset_loader.cpp   # the one loader of bound asset files: GLTFDocument, template mesh, material (03.2)
    src/godot_asset_showcase_scene.cpp  # the asset showcase in Godot: sphere, plane, model (04.7)
    src/godot_plant_scene.cpp    # the 3D scene of the care view: camera, sun, sky, ground, tubes, instances (03.1)
    src/godot_plant_environment.cpp  # sky colour as a Sky shader, ambient light, fog and exposure of that scene (B0184)
    src/godot_plant_sky.cpp      # the moon light and the discs of sun and moon in that scene (usecase:101384 place sun and moon spatially, 03.11)
    src/godot_plant_sky_disc.cpp # the middle of the sky dome and the discs of the sky on it (usecase:101384, B0262)
    src/godot_plant_stars.cpp    # the starfield of that scene, turned with the sky (usecase:5606 observe the sky, 03.11)
    src/godot_plant_clouds.cpp   # the cloud banks of an overcast sky in that scene, behind hills and tree (usecase:5606, 06.4 N04)
    src/godot_plant_sun_halo.cpp # the halo the mockup draws around the sun in that scene (usecase:5606, 06.4 N03)
    src/godot_plant_horizon_ridge.cpp # the two ridges of hills the mockup draws on the horizon of that scene (usecase:5606, 06.4 N05)
    src/godot_plant_ground_season.cpp # the ground of that scene in the colour of its season, snow, depth and horizon (usecase:100387, 06.4 N06)
    src/godot_plant_leaf_season.cpp # the leaf of that scene in the colour of its season and drought (usecase:100387, B0278)
    src/godot_plant_ground_mesh.cpp # where Godot keeps the ground's vertices, and the spans a dig hands it (usecase:101339, P0690)
    src/godot_plant_brook_reflection.cpp # the sky the water of that scene reflects and the strokes of its current (usecase:101339, 06.4 N07)
    src/godot_plant_haze.cpp     # the shimmer of hot air over that scene (usecase:101515 veil sight with weather, 03.11)
    src/godot_plant_brook.cpp    # the water of the brooks in that scene (usecase:100399 carry brook water, 03.11)
    src/godot_plant_look.cpp    # the materials and organ template files of that scene, bound once (03.2)
    src/godot_plant_shader.cpp  # every shader of that scene, loaded from its file under shaders/ (usecase:100387 view the plant)

    include/ase/adp/godot/godot_registration.hpp
    include/ase/adp/godot/godot_vivarium_node.hpp
    include/ase/adp/godot/godot_vivarium_view_config.hpp  # VivariumLayout, VivariumModes, VivariumTexts, VivariumHudVerb (usecase:100387 view the plant)
    include/ase/adp/godot/godot_vivarium_hud_carry.hpp  # VivariumCarryBridge (usecase:101440 grow the interface with skills)
    include/ase/adp/godot/godot_vivarium_hud_hand.hpp   # VivariumHandBridge (usecase:100367 water a patch)
    include/ase/adp/godot/godot_vivarium_hud_loose.hpp  # VivariumLooseBridge (usecase:5939 adapt area terrain)
    include/ase/adp/godot/godot_hud_ui_rng.hpp          # ui_rng_draw, the mockup's seeded draw (usecase:101440 grow the interface with skills)
    include/ase/adp/godot/godot_vivarium_hud_sign_rules.hpp  # VivariumSignRules (usecase:100370 wilt from drought)
    include/ase/adp/godot/godot_vivarium_hud_trail.hpp  # VivariumTrailBridge (usecase:100390 return to the garden)
    include/ase/adp/godot/godot_vivarium_debug.hpp
    include/ase/adp/godot/godot_vivarium_watch.hpp  # VivariumMeasurement, VivariumGroundWatch (usecase:100387 view the plant)
    include/ase/adp/godot/godot_asset_loader.hpp
    include/ase/adp/godot/godot_asset_showcase_scene.hpp
    include/ase/adp/godot/godot_plant_scene.hpp
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
    include/ase/adp/godot/godot_hud_compose_place.hpp  # VivariumHudPlaces: the friend that names where a tap lands on each part of VivariumHud (usecase:101440, 06.5)
    include/ase/adp/godot/godot_hud_replay.hpp  # HudReplay: the mockup's lastKey and the clock the HUD's animations play from (usecase:101440, 06.5)
    include/ase/adp/godot/godot_hud_ui_icon.hpp
    include/ase/adp/godot/godot_hud_ui_anchor.hpp
    include/ase/adp/godot/godot_hud_ui_motion.hpp
    include/ase/adp/godot/godot_hud_ui_refresh.hpp
    include/ase/adp/godot/godot_hud_ui_pen.hpp
    include/ase/adp/godot/godot_hud_ui_ring.hpp
    include/ase/adp/godot/godot_hud_ui_leader_line.hpp
    include/ase/adp/godot/godot_hud_ui_text_block.hpp
    include/ase/adp/godot/godot_hud_ui_spine.hpp
    include/ase/adp/godot/godot_hud_ui_hex_chip.hpp
    include/ase/adp/godot/godot_hud_ui_edge_rail.hpp
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
add_library(ase::adp::godot ALIAS ase-adp-godot)

set_target_properties(ase-adp-godot PROPERTIES OUTPUT_NAME ase_adp_godot)
target_compile_features(ase-adp-godot PRIVATE cxx_std_20)
target_compile_definitions(ase-adp-godot
    PRIVATE
        ASE_ADP_GODOT_VERSION="${PROJECT_VERSION}"
)
target_link_libraries(ase-adp-godot
    PRIVATE
        ase::adp::godot-host
        ase::math              # sqrt / floor of the drawing (header-only, L0)
        ase::utils             # boot_nanos: the real time of a frame, a suspend of the device included (header-only, L0)
        godot::cpp
)

# ── Installation: the headers of both libraries, for a client that embeds the adapter ─────
include(GNUInstallDirs)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(ASE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
