cmake_minimum_required(VERSION 3.20)

#[[
ASE-ADP-GODOT TESTS (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.2/01.3)

EIN ZIEL, EINE QUELLE: der doctest-Einstiegspunkt steht als Uebersetzungsdefinition am ZIEL
(MACRO_DEFINITION_FORBIDDEN im Dateikopf). Dieselbe Form wie plugins/ase-pl-vegetation/tests.

  ase-adp-godot-host-test  GodotHostResourceManager gegen das ECHTE Bundle des Clients: Boot mit
                           vier Flaechen, Startwerte, Giessen, dt-Deckel, verweigertes dt,
                           20-mal Reiniciar, fehlendes Plugin, idempotenter Abbau, und der
                           Selbsttest (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3) mit der Tabelle
                           des Clients samt Positivkontrolle. Kein Godot im Prozess - genau
                           dafuer traegt der Host-Teil keinen Godot-Typ.
  ase-adp-godot-layout-test
                           place_view, band_offset und in_one_band auf den Flaechen des
                           Android-Markts (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3): logische
                           Groesse, 9:20-Handy, Einzuege von Kamera und Gestenleiste, 3:4-Tablet,
                           zu kleine sichere Flaeche, Flaeche ohne Raum, die Bandgrenzen. Reine
                           Rechnung, braucht weder Bundle noch Godot.

DAS BUNDLE UND DIE SELBSTTEST-TABELLE KOMMEN VON DER EINBETTENDEN CMAKE
(clients/avt-client-android): Verzeichnis und Ziel des Bundles, Pfad der Tabelle. Ohne sie gibt es
nichts, wogegen dieser Test laufen koennte - ein Test gegen ein erfundenes Bundle oder eine eigene
Tabelle bestuende jeden Fall, ohne den Pfad zu beruehren, den Godot nimmt.
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

add_executable(ase-adp-godot-host-test test_godot_host_resource_manager.cpp)
target_compile_features(ase-adp-godot-host-test PRIVATE cxx_std_20)
target_compile_definitions(ase-adp-godot-host-test
    PRIVATE
        DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
        ASE_ADP_GODOT_TEST_BUNDLE="${ASE_ADP_GODOT_TEST_BUNDLE_DIR}"
        ASE_ADP_GODOT_TEST_SELFTEST="${ASE_ADP_GODOT_TEST_SELFTEST_TABLE}"
        # ABSOLUTE, like the user:// path the node hands over.
        ASE_ADP_GODOT_TEST_LOG="${CMAKE_CURRENT_BINARY_DIR}/test-logs/ase-adp-godot-test.log"
)
target_link_libraries(ase-adp-godot-host-test
    PRIVATE
        ase::adp::godot-host
        ase::fileio
        ase::utils
        doctest::doctest
)
# The plugin is loaded from the bundle, not linked - without this edge the test would run
# against a bundle that is not written yet.
add_dependencies(ase-adp-godot-host-test ${ASE_ADP_GODOT_TEST_BUNDLE_TARGET})
add_test(NAME ase-adp-godot-host-test COMMAND ase-adp-godot-host-test)
set_target_properties(ase-adp-godot-host-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Ein eigenes Ziel, keine zweite Quelle im Ziel oben: der Einstiegspunkt steht am ZIEL und
# erzeugte in zwei Einheiten zwei main.
add_executable(ase-adp-godot-layout-test test_godot_view_layout.cpp)
target_compile_features(ase-adp-godot-layout-test PRIVATE cxx_std_20)
target_compile_definitions(ase-adp-godot-layout-test PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
target_link_libraries(ase-adp-godot-layout-test
    PRIVATE
        ase::adp::godot-host
        doctest::doctest
)
add_test(NAME ase-adp-godot-layout-test COMMAND ase-adp-godot-layout-test)
set_target_properties(ase-adp-godot-layout-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
