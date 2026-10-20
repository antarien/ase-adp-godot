cmake_minimum_required(VERSION 3.20)

#[[
ASE-ADP-GODOT TESTS (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.2/01.3)

ONE TARGET, ONE SOURCE: the doctest entry point stands as a compile definition on the TARGET
(MACRO_DEFINITION_FORBIDDEN in the file head), so two units in one target would give two main.
The same form as plugins/ase-pl-vegetation/tests.

  ase-adp-godot-host-test     GodotHostResourceManager against the REAL bundle of the client, with its
                              one game host: boot with four patches, start values, the log, pouring,
                              the organ and instance ports, the cycle line, catching up, refused dt, the
                              self-test beside the game host and the idempotent teardown. No Godot in
                              the process - that is what the host part carries no Godot type for.
  ase-adp-godot-refusal-test  The same manager without a game host: the bundle index, a missing bundle,
                              every incomplete configuration and every port the set does not offer, the
                              words of the start configuration, a missing unit (L8), and the self-test
                              (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3) with the client's table and its
                              positive control.
  ase-adp-godot-scene-test    The scene ports of a game host of its own (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS
                              03.11): the organ templates of ase-render-data, the sky, the ground and
                              the stem bases.
  ase-adp-godot-layout-test   place_view, band_offset and in_one_band on the surfaces of the Android
                              market (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3): logical size, a 9:20
                              handset, camera and gesture bar insets, a 3:4 tablet, a safe area too
                              small, a surface without room, the band edges. Pure computation, needs
                              neither bundle nor Godot.
  ase-adp-godot-plant-test    rows_by_body, build_template, ground and stem base, the order budget by
                              frame time and bind_assets against a test manifest
                              (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1, 03.2, 03.11). Pure computation.
  ase-adp-godot-keeper-test   The care flow of the Plant Keeper: the return to the garden, the focused
                              patch and the seed of every sowing (03.1, 03.6). Pure computation.
  ase-adp-godot-showcase-test The asset showcase (PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7). Pure
                              computation against a test binding.

THE BUNDLE AND THE SELF-TEST TABLE COME FROM THE EMBEDDING CMAKE (clients/avt-client-android): the
directory and the target of the bundle, the path of the table. Without them there is nothing these
tests could run against - a test against an invented bundle or a table of its own would pass every
case without touching the path Godot takes.

@usecase     usecase:100051 - bind a build edge
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

# The three targets against the real bundle. A file is named after its target, never after the
# manager it drives: a .cpp whose name carries resource_manager is a resource manager implementation
# (file_types.json, resource_mgr_impl). Each target writes its own log file - ABSOLUTE, like the
# user:// path the node hands over - so no run rotates the file of another.
foreach(_adp_godot_bundle_test host refusal scene)
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
foreach(_adp_godot_pure_test layout plant keeper showcase)
    if(_adp_godot_pure_test STREQUAL "layout")
        set(_adp_godot_pure_source test_godot_view_layout.cpp)
    elseif(_adp_godot_pure_test STREQUAL "plant")
        set(_adp_godot_pure_source test_godot_plant_view.cpp)
    elseif(_adp_godot_pure_test STREQUAL "keeper")
        set(_adp_godot_pure_source test_godot_keeper_flow.cpp)
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
    add_test(NAME ${_adp_godot_pure_target} COMMAND ${_adp_godot_pure_target})
    set_target_properties(${_adp_godot_pure_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endforeach()
