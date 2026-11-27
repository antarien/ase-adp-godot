cmake_minimum_required(VERSION 3.20)

#[[
ASE-ADP-GODOT TESTS (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.2/01.3)

ONE TARGET, ONE SOURCE: the doctest entry point stands as a compile definition on the TARGET
(MACRO_DEFINITION_FORBIDDEN in the file head), so two units in one target would give two main.
The same form as plugins/ase-pl-vegetation/tests.

  ase-adp-godot-host-test     GodotHostResourceManager against the REAL bundle of the client, with its
                              one game host: boot with four patches, start values, the log, pouring,
                              the organ and instance ports, the cycle line, the self-test beside the game
                              host and the idempotent teardown. No Godot in the process - that is what the
                              host part carries no Godot type for.
  ase-adp-godot-clock-test    The clock of a game host of its own (PLAN_ASE_VIVARIUM_PHASE_01_INTEG L5):
                              a long frame as one tick and one filed real position, the time lapse as a
                              pace of ase-time's clock, refused dt.
  ase-adp-godot-refusal-test  The same manager without a game host: the bundle index, a missing bundle,
                              every incomplete configuration and every port the set does not offer, the
                              words of the start configuration, a missing unit (L8), and the self-test
                              (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3) with the client's table and its
                              positive control.
  ase-adp-godot-scene-test    The scene ports of a game host of its own (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS
                              03.11): the organ templates of ase-render-data, the sky, the ground and
                              the stem bases.
  ase-adp-godot-tree-test     A birch the clock jumped to MADURA, its stem on the bark port (B0295 point 4,
                              usecase:101613 show the plant bark tubes): the rings meet along each internode,
                              one wave on the whole axis, a share that never falls up the stem.
  ase-adp-godot-figure-test   The figure ports of ase-pl-avatar on a game host of its own
                              (PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.8, 07.10, usecase:101625 carry a tool
                              in hand): the figure of the seat at its spawn place, hold and drop filed as
                              put-on and put-down requests, every gear piece at the right hand of a figure.
  ase-adp-godot-camera-test   The camera ports of ase-pl-avatar on a game host of its own
                              (PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.5, 07.6, 07-A, 07-G, usecase:101588 move
                              player figure over ground): cam.mode there and back to the fixed view, the drag
                              and the zoom of the player camera, the rim of the area on the camera record.
  ase-adp-godot-cap-test      The cap port of ase-pl-flora on a game host of its own (usecase:101362 scale
                              the organ templates, B0605): the death cap on place 1 carries the radius of
                              its catalog row, a species without the key carries no record.
  ase-adp-godot-layout-test   place_view, band_offset and in_one_band on the surfaces of the Android
                              market (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3): logical size, a 9:20
                              handset, camera and gesture bar insets, a 3:4 tablet, a safe area too
                              small, a surface without room, the band edges. Pure computation, needs
                              neither bundle nor Godot.
  ase-adp-godot-plant-test    The stem base and the order budget by frame time (03.1, 03.6). Pure computation.
  ase-adp-godot-instances-test rows_by_body: the instance records to body and order split (03.11). Pure.
  ase-adp-godot-bark-test     The bark share and tiling as custom data of each instance (03.11). Pure.
  ase-adp-godot-tubes-test    tubes_by_body: the bark port laid out into one mesh per body, the knot rings
                              shared, the ring radius and axis order beside every vertex (B0295). Pure.
  ase-adp-godot-stroke-test   stroke_run, stroke_bound, stroke_lay: the mesh of ui_stroke never lays more corners
                              or indices than the bound its arrays are sized to, open, closed, one point, a
                              hairpin, a thick zigzag (P0692, T0357). Pure computation.
  ase-adp-godot-impostor-test impostor_of: from where the card of a far body takes over, by the one-pixel
                              rule of the bark shader, and the size of card and texture (B0295). Pure.
  ase-adp-godot-templates-test build_template from the template ports (03.11). Pure computation.
  ase-adp-godot-capscale-test cap_scale and the cap role and node of a species (usecase:101362 scale the
                              organ templates, B0605). Pure computation.
  ase-adp-godot-groundtops-test GroundTops against the march of every step over a carved bed: the same first
                              place and ground on every line, fewer heights asked (usecase:101650 show the river
                              water, P0733). Pure computation.
  ase-adp-godot-framerate-test HudFrameRate: the frames per second of a window, the floor of the frame budget,
                              one edge each way, the time away no frame (usecase:101440 grow the interface with
                              skills). Pure computation.
  ase-adp-godot-rsmtable-test The RSM table of the debug layer: the clock HH:MM:SS, the column order, the ring shares, the
                              order of the slices of an entry and the view a tap chooses (usecase:101305 show skill
                              progress, MIG_ASE_SKILL_RSM B6). Pure computation.
  ase-adp-godot-registro-test The row REGISTRO of the SALTO tab: every event jump of vivarium_start.json lies inside the
                              debug drawer, hud_debug_registro_per_row to a row, the note above the foot
                              (usecase:101120 clock the growth cycle, PHASE_06_HUD_EXT D01). Pure computation.
  ase-adp-godot-activityart-test activity-art.json of SHA (MIG_ASE_SKILL_RSM B7, usecase:101440): the thirteen
                              symbols in the format of item-art.json, their colours, their words and the way the file
                              reaches UiActivityArt. Pure computation.
  ase-adp-godot-activitybars-test The activity bars (MIG_ASE_SKILL_RSM B5, usecase:101305): the youngest slice left, at
                              most twenty, a slice without a number left out, the height clamped, the lit sum, the
                              strip and the bar a touch chooses. Pure computation.
  ase-adp-godot-stand-test   Soil water and condition of a patch for its materials (03.2). Pure.
  ase-adp-godot-binding-test  bind_assets against a test manifest, stand-in colours (03.2). Pure.
  ase-adp-godot-ground-test   build_ground, mark_ground (levelled disc, brook bed) and the uniforms of
                              the ground shader (03.8, 03.11). Pure computation.
  ase-adp-godot-figlayout-test The tap square of cam.mode against the band of Phase 06: sky field, season
                              word, debug switch, read from SHA's theme.css (07.1, B0352). Pure computation.
  ase-adp-godot-hold-test     start_hold_step: the one order in which the steps hold the start screen,
                              the figure file before a jump (07.9, B0357). Pure computation.
  ase-adp-godot-water-test    build_water, build_water_table and build_water_line in the beds (03.11, B0285).
  ase-adp-godot-waterpit-test The water of the pits ase-hydro fills, pit_water_levels (D2506). Pure computation.
  ase-adp-godot-waterband-test The reaches of the water bands LOD 0 to 4 by the pixel rule, the tiles per
                              chunk and the chunk plane (usecase:101363, H.13). Pure computation.
  ase-adp-godot-waterkinds-test The lake at the top of its box, the sea chunk at NN0 and the ice over the
                              ground of its own chunk, and the records of the three ports (usecase:101339,
                              H.13). Pure computation.
  ase-adp-godot-keeper-test   The return to the garden of the Plant Keeper (03.1, 03.6, V6). Pure computation.
  ase-adp-godot-focus-test    The selected patch: the place the camera looks at, its near view and the
                              zoom (03.1, V2). Pure computation.
  ase-adp-godot-seed-test     The seed of every sowing (03.1, V7). Pure computation.
  ase-adp-godot-exposure-test The exposure of the scene camera for the sky's light (D1721). Pure computation.
  ase-adp-godot-showcase-test The asset showcase (PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7). Pure
                              computation against a test binding.

THE BUNDLE AND THE SELF-TEST TABLE COME FROM THE EMBEDDING CMAKE (clients/avt-client-android): the
directory and the target of the bundle, the path of the table. Without them there is nothing these
tests could run against - a test against an invented bundle or a table of its own would pass every
case without touching the path Godot takes.

@usecase     usecase:100051 - resolve module dependencies

Copyright (c) Jan Ohlmann. All rights reserved.
Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
SPDX-License-Identifier: LicenseRef-ASE-LIC
]]

foreach(_adp_godot_test_input ASE_ADP_GODOT_TEST_BUNDLE_DIR ASE_ADP_GODOT_TEST_BUNDLE_TARGET
                              ASE_ADP_GODOT_TEST_SELFTEST_TABLE)
    if(NOT DEFINED ${_adp_godot_test_input})
        message(FATAL_ERROR "ase-adp-godot tests: ${_adp_godot_test_input} missing - the embedding CMake (clients/avt-client-android) sets it before add_subdirectory")
    endif()
endforeach()

find_package(doctest QUIET)
if(NOT doctest_FOUND)
    include(FetchContent)
    FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git GIT_TAG v2.4.12)
    FetchContent_MakeAvailable(doctest)
endif()

# The targets against the real bundle. A file is named after its target, never after the
# manager it drives: a .cpp whose name carries resource_manager is a resource manager implementation
# (file_types.json, resource_mgr_impl). Each target writes its own log file - ABSOLUTE, like the
# user:// path the node hands over - so no run rotates the file of another.
foreach(_adp_godot_bundle_test host clock refusal scene tree figure camera cap lod skill)
    if(_adp_godot_bundle_test STREQUAL "host")
        set(_adp_godot_bundle_source test_godot_host.cpp)
    else()
        set(_adp_godot_bundle_source test_godot_host_${_adp_godot_bundle_test}.cpp)
    endif()
    set(_adp_godot_bundle_target ase-adp-godot-${_adp_godot_bundle_test}-test)
    add_executable(${_adp_godot_bundle_target} ${_adp_godot_bundle_source})
    target_compile_features(${_adp_godot_bundle_target} PRIVATE cxx_std_20)
    target_compile_definitions(${_adp_godot_bundle_target}
        PRIVATE
            DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
            ASE_ADP_GODOT_TEST_BUNDLE="${ASE_ADP_GODOT_TEST_BUNDLE_DIR}"
            ASE_ADP_GODOT_TEST_SELFTEST="${ASE_ADP_GODOT_TEST_SELFTEST_TABLE}"
            ASE_ADP_GODOT_TEST_LOG="${CMAKE_CURRENT_BINARY_DIR}/test-logs/${_adp_godot_bundle_target}.log"
    )
    target_link_libraries(${_adp_godot_bundle_target}
        PRIVATE
            ase::adp::godot-host
            ase::fileio
            ase::utils
            doctest::doctest
    )
    # The plugin is loaded from the bundle, not linked - without this edge the test would run
    # against a bundle that is not written yet.
    add_dependencies(${_adp_godot_bundle_target} ${ASE_ADP_GODOT_TEST_BUNDLE_TARGET})
    add_test(NAME ${_adp_godot_bundle_target} COMMAND ${_adp_godot_bundle_target})
    set_target_properties(${_adp_godot_bundle_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endforeach()

# The targets of pure computation: neither bundle nor Godot.
foreach(_adp_godot_pure_test layout plant instances bark bend tubes ring stroke knots impostor level templates capscale groundtops framerate stand binding ground figlayout hold water waterpit waterband waterkinds keeper focus seed exposure window context signs chips pose step arc rsmtable registro activityart activitybars showcase)
    if(_adp_godot_pure_test STREQUAL "layout")
        set(_adp_godot_pure_source test_godot_view_layout.cpp)
    elseif(_adp_godot_pure_test STREQUAL "plant")
        set(_adp_godot_pure_source test_godot_plant_view.cpp)
    elseif(_adp_godot_pure_test STREQUAL "instances")
        set(_adp_godot_pure_source test_godot_plant_instances.cpp)
    elseif(_adp_godot_pure_test STREQUAL "bark")
        set(_adp_godot_pure_source test_godot_plant_bark.cpp)
    elseif(_adp_godot_pure_test STREQUAL "bend")
        set(_adp_godot_pure_source test_godot_plant_bend.cpp)  # usecase:6540 generate plant organs, TREEGEN T.6
    elseif(_adp_godot_pure_test STREQUAL "tubes")
        set(_adp_godot_pure_source test_godot_plant_tubes.cpp)  # usecase:101613 show the plant bark tubes
    elseif(_adp_godot_pure_test STREQUAL "ring")
        set(_adp_godot_pure_source test_godot_tube_ring.cpp)  # usecase:101644 count ring corners by size
    elseif(_adp_godot_pure_test STREQUAL "stroke")
        set(_adp_godot_pure_source test_godot_stroke_mesh.cpp)  # usecase:101440 grow the interface with skills, T0357
    elseif(_adp_godot_pure_test STREQUAL "knots")
        set(_adp_godot_pure_source test_godot_knot_store.cpp)  # usecase:101613 show the plant bark tubes, N0543
    elseif(_adp_godot_pure_test STREQUAL "impostor")
        set(_adp_godot_pure_source test_godot_plant_impostor.cpp)  # usecase:101613, the card of a far body
    elseif(_adp_godot_pure_test STREQUAL "level")
        set(_adp_godot_pure_source test_godot_plant_level.cpp)  # usecase:101657 grade plants by picture height, B0897
    elseif(_adp_godot_pure_test STREQUAL "templates")
        set(_adp_godot_pure_source test_godot_plant_templates.cpp)
    elseif(_adp_godot_pure_test STREQUAL "capscale")
        set(_adp_godot_pure_source test_godot_cap_scale.cpp)  # usecase:101362 scale the organ templates
    elseif(_adp_godot_pure_test STREQUAL "groundtops")
        set(_adp_godot_pure_source test_godot_ground_tops.cpp)  # usecase:101650 show the river water, P0733
    elseif(_adp_godot_pure_test STREQUAL "framerate")
        set(_adp_godot_pure_source test_godot_hud_frame_rate.cpp)  # usecase:101440 grow the interface with skills
    elseif(_adp_godot_pure_test STREQUAL "stand")
        set(_adp_godot_pure_source test_godot_plant_stand.cpp)
    elseif(_adp_godot_pure_test STREQUAL "binding")
        set(_adp_godot_pure_source test_godot_asset_binding.cpp)
    elseif(_adp_godot_pure_test STREQUAL "ground")
        set(_adp_godot_pure_source test_godot_plant_ground.cpp)
    elseif(_adp_godot_pure_test STREQUAL "figlayout")
        set(_adp_godot_pure_source test_godot_hud_figure_layout.cpp)  # usecase:101554 receive player input
    elseif(_adp_godot_pure_test STREQUAL "hold")
        set(_adp_godot_pure_source test_godot_start_hold.cpp)  # usecase:100387 view the plant
    elseif(_adp_godot_pure_test STREQUAL "water")
        set(_adp_godot_pure_source test_godot_plant_water.cpp)
    elseif(_adp_godot_pure_test STREQUAL "waterpit")
        set(_adp_godot_pure_source test_godot_plant_water_pit.cpp)  # usecase:101339, the water of the pits (D2506)
    elseif(_adp_godot_pure_test STREQUAL "waterband")
        set(_adp_godot_pure_source test_godot_plant_water_band.cpp)  # usecase:101363 band entities by distance, H.13
    elseif(_adp_godot_pure_test STREQUAL "waterkinds")
        set(_adp_godot_pure_source test_godot_plant_water_kinds.cpp)  # usecase:101339, the lake, sea and glacier water
    elseif(_adp_godot_pure_test STREQUAL "keeper")
        set(_adp_godot_pure_source test_godot_keeper_flow.cpp)
    elseif(_adp_godot_pure_test STREQUAL "focus")
        set(_adp_godot_pure_source test_godot_patch_focus.cpp)
    elseif(_adp_godot_pure_test STREQUAL "seed")
        set(_adp_godot_pure_source test_godot_new_seed.cpp)
    elseif(_adp_godot_pure_test STREQUAL "exposure")
        set(_adp_godot_pure_source test_godot_scene_exposure.cpp)
    elseif(_adp_godot_pure_test STREQUAL "window")
        set(_adp_godot_pure_source test_godot_client_window.cpp)
    elseif(_adp_godot_pure_test STREQUAL "context")
        set(_adp_godot_pure_source test_godot_hud_context_resolver.cpp)  # usecase:101440 grow the interface with skills
    elseif(_adp_godot_pure_test STREQUAL "signs")
        set(_adp_godot_pure_source test_godot_hud_context_signs.cpp)  # usecase:101440 grow the interface with skills
    elseif(_adp_godot_pure_test STREQUAL "chips")
        set(_adp_godot_pure_source test_godot_hud_context.cpp)  # usecase:101440 grow the interface with skills
    elseif(_adp_godot_pure_test STREQUAL "pose")
        set(_adp_godot_pure_source test_godot_figure_pose.cpp)  # usecase:2157 go to Sleep, FIRECAMP T.11: no figure sleeps standing
    elseif(_adp_godot_pure_test STREQUAL "rsmtable")
        set(_adp_godot_pure_source test_godot_hud_debug_rsm.cpp)  # usecase:101305 show skill progress, MIG_ASE_SKILL_RSM B6
    elseif(_adp_godot_pure_test STREQUAL "registro")
        set(_adp_godot_pure_source test_godot_hud_debug_registro.cpp)  # usecase:101120 clock the growth cycle, PHASE_06_HUD_EXT D01: the row REGISTRO of SALTO inside the drawer
    elseif(_adp_godot_pure_test STREQUAL "step")
        set(_adp_godot_pure_source test_godot_figure_step.cpp)  # usecase:101588 move player figure over ground, B0943: smooth between steps, turn free of the frame rate
    elseif(_adp_godot_pure_test STREQUAL "arc")
        set(_adp_godot_pure_source test_godot_hud_arc.cpp)  # usecase:101440 grow the interface with skills, EXT.6: the turn, the fade, the codes and the bridge of the sky arc
    elseif(_adp_godot_pure_test STREQUAL "activityart")
        set(_adp_godot_pure_source test_godot_hud_activity_art.cpp)  # usecase:101440 grow the interface with skills, MIG_ASE_SKILL_RSM B7: the thirteen activity symbols
    elseif(_adp_godot_pure_test STREQUAL "activitybars")
        set(_adp_godot_pure_source test_godot_hud_activity_bars.cpp)  # usecase:101305 show skill progress, MIG_ASE_SKILL_RSM B5: the order, the places and the hit of the activity bars
    else()
        set(_adp_godot_pure_source test_godot_asset_showcase.cpp)
    endif()
    set(_adp_godot_pure_target ase-adp-godot-${_adp_godot_pure_test}-test)
    add_executable(${_adp_godot_pure_target} ${_adp_godot_pure_source})
    target_compile_features(${_adp_godot_pure_target} PRIVATE cxx_std_20)
    target_compile_definitions(${_adp_godot_pure_target} PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
    target_link_libraries(${_adp_godot_pure_target}
        PRIVATE
            ase::adp::godot-host
            doctest::doctest
    )
    if(_adp_godot_pure_test STREQUAL "ground" OR _adp_godot_pure_test STREQUAL "figlayout" OR _adp_godot_pure_test STREQUAL "activityart"
       OR _adp_godot_pure_test STREQUAL "rsmtable" OR _adp_godot_pure_test STREQUAL "registro"
       OR _adp_godot_pure_test STREQUAL "context")
        # the context test reads SHA's theme.css: where the plant card's values end beside the body rail (B0937)
        # the ground test reads shaders/plant_ground.gdshader, the figlayout and the rsmtable test SHA's theme.css (the
        # rsmtable test the place of the debug console at the foot), the registro test theme.css and the start config
        # vivarium_start.json (the row REGISTRO of SALTO inside the drawer) and the activityart test
        # SHA's activity-art.json with its cascade files from the tree;
        # only these targets take ase-fileio, so no other unit's command line changes (usecase:101339 show the
        # ground around plant, usecase:101554 receive player input)
        target_link_libraries(${_adp_godot_pure_target} PRIVATE ase::fileio)
    endif()
    add_test(NAME ${_adp_godot_pure_target} COMMAND ${_adp_godot_pure_target})
    set_target_properties(${_adp_godot_pure_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endforeach()
