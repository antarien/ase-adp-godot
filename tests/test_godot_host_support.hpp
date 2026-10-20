#pragma once

/**
 * ASE ADAPTER TESTS - GODOT HOST SUPPORT
 *
 * @file        test_godot_host_support.hpp
 * @design      DSGN_021
 * @brief       The start configuration, the stage reader, the game host and the log matching the host test executables use
 * @description The manager is tested against the SAME bundle Godot loads (clients/avt-client-android,
 *              AvtRuntime.cmake) in three executables: test_godot_host_refusal.cpp starts no game host - the
 *              refusals and the self-test on its own test host - while test_godot_host.cpp (boot, pour,
 *              organs, catch-up, teardown) and test_godot_host_scene.cpp (templates, sky, ground, stem
 *              base) each start the one game host of their run. All read the bundle through the same
 *              stage reader, configure the same ports and patches as the client's start configuration,
 *              and match the host's lines in the form ase-log displays them; these functions are that one
 *              shared part. Each executable takes
 *              its doctest entry point from its own target (tests/CMakeLists.txt), never from this header.
 *
 *              THE PATCHES COME FROM THE START CONFIGURATION (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS):
 *              with a host present ase-pl-flora lays none of its own, and the manager asks for each
 *              configured patch with a lay request. The configuration names four, places 0 to 3, each
 *              with 100 mm field capacity and 20 mm start water (the plugin's types.hpp), a seed of a
 *              healthy stand. The tests find them by place, never by a guessed object id.
 *
 *              THE STAGE READER IS THE TEST'S OWN (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.1): the node
 *              reads index and manifests through Godot's FileAccess, these tests through ase-fileio from
 *              the bundle directory - the manager sees only VivariumStageReadFn.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Component System (ECS)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/fileio/path.hpp>
#include <ase/fileio/text_reader.hpp>
#include <ase/fileio/text_writer.hpp>
#include <ase/log/log_display.hpp>
#include <ase/utils/fs.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot::test {

// Where the four test patches lie: centres in world metres (east, north) inside the measured square
// of 16 +- 12 m, and the half edge of each - the radius of the disc ase-terrain levels.
inline constexpr double kPatchCentres[4][2] = {{10.0, 10.0}, {22.0, 10.0}, {10.0, 22.0}, {22.0, 22.0}};
inline constexpr double kPatchHalfM = 2.0;

// The tests' VivariumStageReadFn: one file of the bundle directory `user` names.
inline bool read_bundle_file(const char* name, std::string& out, void* user) {
    const std::string path = ase::fileio::path_join(*static_cast<const std::string*>(user), name);
    if (!ase::fileio::file_exists(path)) {
        return false;
    }
    out = ase::fileio::read_text(path);
    return true;
}

// The client's start configuration over the bundle directory `bundle`, which must outlive it: the
// libraries are opened by path there and the data root lies below it, as on the desktop.
inline VivariumStartConfig vivarium_config(std::string& bundle) {
    VivariumStartConfig config;
    config.read_stage = &read_bundle_file;
    config.read_stage_user = &bundle;
    config.library_dir = bundle;
    config.files_dir = bundle;
    config.patch_port = "flora.patch.v1";
    config.clock_port = "flora.clock.v1";
    config.scene_port = "flora.scene.v1";
    // The ground of the scene (03.8) and the sky of the scene (03.11 step 3): boot() asks the set for
    // these ports like for the others, so the tests name them as the client's start configuration does.
    config.ground_port = "flora.ground.v1";
    config.brook_port = "flora.brook.v1";
    config.terrain_port = "flora.terrain.v1";
    config.sky_port = "sky.scene.v1";
    // The seed bodies of ase-physics (03.11 step 4): boot() asks the set for this port as well.
    config.seed_port = "flora.seed.v1";
    // The bark tiling of the internodes (03.11 step 2): boot() asks the set for this port as well.
    config.tile_port = "flora.tile.v1";
    // The organ templates of ase-render-data (usecase:101011 lay out organ templates): both ports as well.
    config.template_corner_port = "flora.template.corner.v1";
    config.template_triangle_port = "flora.template.triangle.v1";
    config.organ_ports[GODOT_ORGAN_SEGMENT] = "flora.organ.segment.v1";
    config.organ_ports[GODOT_ORGAN_LEAF] = "flora.organ.leaf.v1";
    config.organ_ports[GODOT_ORGAN_MALE] = "flora.organ.male.v1";
    config.organ_ports[GODOT_ORGAN_FEMALE] = "flora.organ.female.v1";
    config.organ_ports[GODOT_ORGAN_FRUIT] = "flora.organ.fruit.v1";
    config.organ_ports[GODOT_ORGAN_ROOT] = "flora.organ.root.v1";
    // The ground cells beside the terrain samples (03.11 step 2): boot() asks the set for the port.
    config.ground_cell_port = "flora.ground.cell.v1";
    // The instance ports of the drawn classes (03.11 step 2): boot() asks the set for them as well.
    config.instance_ports[GODOT_ORGAN_SEGMENT] = "flora.instance.segment.v1";
    config.instance_ports[GODOT_ORGAN_LEAF] = "flora.instance.leaf.v1";
    config.instance_ports[GODOT_ORGAN_MALE] = "flora.instance.male.v1";
    config.instance_ports[GODOT_ORGAN_FEMALE] = "flora.instance.female.v1";
    config.instance_ports[GODOT_ORGAN_FRUIT] = "flora.instance.fruit.v1";
    config.op_irrigate = "irrigate";
    config.op_prune = "prune";
    config.op_lay = "lay";
    config.irrigate_amount_mm = 25.0;
    config.catch_up_steps = 8u;
    // The measured square of the client's start configuration (config/vivarium_start.json, "ground"):
    // boot() lays it on the ground port, so a square of zero edge would ask ase-terrain for nothing.
    config.ground.x = 16.0;
    config.ground.z = 16.0;
    config.ground.half_m = 12.0;
    config.ground.step_m = 0.5;
    // Four patches on the places 0 to 3, the set every case finds by place; place 3 on the forest floor,
    // so the ground travels through the port both ways. Each lies in the world - a lay request without a
    // centre and a positive half edge is refused by ase-pl-flora and places nothing
    // (FloraHostPlcmSystem) - on a corner of the measured square, 12 m apart, so no two levelled discs
    // touch.
    for (uint32_t place = 0u; place < 4u; ++place) {
        VivariumPatchSpec patch;
        patch.place = place;
        patch.species = "arosaruebe";
        patch.seed = 1u + place;
        patch.soil = place == 3u ? GODOT_SOIL_FOREST : GODOT_SOIL_MEADOW;
        patch.x = kPatchCentres[place][0];
        patch.z = kPatchCentres[place][1];
        patch.half_m = kPatchHalfM;
        config.patches.push_back(patch);
    }
    return config;
}

// A host line without its colour sequences. ase-log colours words of a certain shape inside the
// message body (log_display.cpp, colorize_log_line), so a phrase can stand split by them. A CSI
// sequence runs from ESC '[' to the first byte in '@'..'~', as ansi_len reads it there.
inline std::string without_colour(const std::string& line) {
    std::string plain;
    plain.reserve(line.size());
    for (std::string::size_type i = 0; i < line.size(); ++i) {
        if (line[i] == '\x1b' && i + 1 < line.size() && line[i + 1] == '[') {
            std::string::size_type end = i + 2;
            while (end < line.size() && !(line[end] >= '@' && line[end] <= '~')) {
                ++end;
            }
            i = end;  // the loop steps over the final byte
            continue;
        }
        plain.push_back(line[i]);
    }
    return plain;
}

// `text` as ase-log displays it INSIDE a line: shorten_display_line puts every known word in its
// short form and drops fillers - but keeps a filler at the very start of a line. So the text is
// walked behind a bracket group, which the walk passes unchanged, and cut off again. The function
// is the runtime's own, exported by libase_kernel_embedded.so; these tests link it from there only.
inline std::string displayed(const std::string& text) {
    const std::string shown = ase::log::shorten_display_line("[] " + text);
    return shown.substr(shown.compare(0, 3, "[] ") == 0 ? 3u : 2u);
}

// Whether one of the host's lines carries `text`, compared in the form the lines arrive in.
inline bool any_line_carries(const std::vector<std::string>& lines, const std::string& text) {
    const std::string marker = displayed(text);
    for (const std::string& line : lines) {
        if (without_colour(line).find(marker) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// THE GAME HOST OF A RUN: one manager, booted on the first call of environment() with the log file of
// the executable (ASE_ADP_GODOT_TEST_LOG, one per target), torn down by the last case about it or with
// the process. Its boot lines are taken out of the queue right after the boot, so a case about the log
// does not depend on what the cases before it drained. What the boot returned is kept and reported by
// every case - a failed boot is never retried, because a second boot would be a second test environment.
struct AdapterEnvironment {
    bool attempted = false;
    std::string bundle;
    GodotHostResourceManager resources;
    ase::kernel::HostStatus booted = ase::kernel::HostStatusInvalidState;
    std::vector<std::string> boot_lines;
};

// The one boot of the run.
inline AdapterEnvironment& environment() {
    static AdapterEnvironment env;
    if (env.attempted) {
        return env;
    }
    env.attempted = true;
    env.bundle = ASE_ADP_GODOT_TEST_BUNDLE;
    const std::string path = ASE_ADP_GODOT_TEST_LOG;
    (void)ase::utils::fs::remove(path);
    VivariumStartConfig config = vivarium_config(env.bundle);
    config.log_file = path;
    env.booted = env.resources.boot(config);
    (void)env.resources.remove_log_lines(env.boot_lines);
    return env;
}

// n frames of a quarter second each - one tick each, the scheduler's frame ceiling.
inline void advance_frames(GodotHostResourceManager& resources, uint32_t frames) {
    for (uint32_t frame = 0u; frame < frames; ++frame) {
        REQUIRE(resources.advance(0.25) == ase::kernel::HostStatusOk);
    }
}

inline const VivariumPatchView& patch_at(const GodotHostResourceManager& resources, uint32_t place) {
    const VivariumPatchView* patch = resources.get_patch_at(place);
    REQUIRE(patch != nullptr);
    return *patch;
}

}  // namespace ase::adp::godot::test
