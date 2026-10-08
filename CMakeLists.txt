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
@modified    2026-10-07
]]

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

# ── Host part: one KernelEmbeddedHost behind a resource manager, and the placement of the view's
#    layout on a surface of any aspect ratio - no Godot type in either, so both are tested without a
#    Godot process ──
add_library(ase-adp-godot-host STATIC
    src/godot_host_resource_manager.cpp
    src/godot_view_layout.cpp

    include/ase/adp/godot/godot_host_resource_manager.hpp
    include/ase/adp/godot/godot_view_layout.hpp
    include/ase/adp/godot/types.hpp
)
add_library(ase::adp::godot-host ALIAS ase-adp-godot-host)

target_include_directories(ase-adp-godot-host
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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
    src/godot_vivarium_node.cpp  # VivariumView: tick, input, drawing, lifecycle

    include/ase/adp/godot/godot_registration.hpp
    include/ase/adp/godot/godot_vivarium_node.hpp
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

if(ASE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
