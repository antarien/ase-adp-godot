/**
 * ASE ADAPTER TESTS - GODOT HOST RESOURCE MANAGER
 *
 * @file        test_godot_host_resource_manager.cpp
 * @design      DSGN_021
 * @brief       Pins GodotHostResourceManager against the real bundle: boot, port values,
 *              irrigation, the dt cap, restarts, a missing plugin and the teardown
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG, the parts of V1, V2, V3, V7 and V8 that do not
 *              need a window. The manager boots the SAME bundle Godot loads: the plugin library,
 *              its staged manifest and the index, side by side in the build's bundle directory
 *              (clients/avt-client-android/cmake/AvtRuntime.cmake). No Godot in this process -
 *              that is why the host part carries no Godot type.
 *
 *              THE START VALUES ARE THE PLAN'S (Festgelegte Darstellung und Spielaktionen):
 *              four patches 1-4 with moisture 100/70/40/10, biomass 0, age 0, irrigate 25,
 *              dt at most 0.125 s. The expected numbers are those of Phase 00 (avt_headless.cpp):
 *              one Regulation step is four ticks of 0.125 s, drain 0.75 and growth 2.0 per step.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @created     2026-10-05
 * @modified    2026-10-05
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

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

namespace {

VivariumStartConfig vivarium_config() {
    VivariumStartConfig config;
    config.bundle_dir = ASE_ADP_GODOT_TEST_BUNDLE;
    config.port = "vegetation.patch.v1";
    config.op_create = "create_patch";
    config.op_irrigate = "irrigate";
    config.start_biomass = 0.0;
    config.start_age_seconds = 0.0;
    config.irrigate_amount = 25.0;
    config.tick_max_seconds = 0.125f;
    config.patches = {{1u, 100.0}, {2u, 70.0}, {3u, 40.0}, {4u, 10.0}};
    return config;
}

bool near(double actual, double expected) {
    return std::fabs(actual - expected) <= 0.0001;
}

// One Regulation step: four frames of exactly the cap.
void advance_steps(GodotHostResourceManager& resources, uint32_t steps) {
    for (uint32_t step = 0u; step < steps; ++step) {
        for (uint32_t frame = 0u; frame < 4u; ++frame) {
            REQUIRE(resources.advance(0.125f) == ase::kernel::HostStatusOk);
        }
    }
}

double moisture_of(const GodotHostResourceManager& resources, uint32_t object_id) {
    const VivariumPatchView* patch = resources.get_patch(object_id);
    REQUIRE(patch != nullptr);
    return patch->moisture;
}

// A host line without its colour sequences. ase-log colours words of a certain shape inside the
// message body (log_display.cpp, colorize_log_line), so a phrase can stand split by them. A CSI
// sequence runs from ESC '[' to the first byte in '@'..'~', as ansi_len reads it there.
std::string without_colour(std::string_view line) {
    std::string plain;
    plain.reserve(line.size());
    for (std::string_view::size_type i = 0; i < line.size(); ++i) {
        if (line[i] == '\x1b' && i + 1 < line.size() && line[i + 1] == '[') {
            std::string_view::size_type end = i + 2;
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
// short form ("failed" becomes "fail", "load_plugin" "load_plgn") and drops fillers - but keeps a
// filler at the very start of a line. So the text is walked behind a bracket group, which the walk
// copies unchanged, and cut off again. The function is the runtime's own, exported by
// libase_kernel_embedded.so; this test links no second copy of the log.
std::string displayed(const std::string& text) {
    const std::string shown = ase::log::shorten_display_line("[] " + text);
    return shown.substr(shown.compare(0, 3, "[] ") == 0 ? 3u : 2u);
}

// Whether one of the host's lines carries `text`, compared in the form the lines arrive in.
bool any_line_carries(const std::vector<std::string>& lines, const std::string& text) {
    const std::string marker = displayed(text);
    for (const std::string& line : lines) {
        if (without_colour(line).find(marker) != std::string::npos) {
            return true;
        }
    }
    return false;
}

}  // anonymous namespace

TEST_CASE("V1: one boot - one host, the plugin's two systems, four patches at their start values") {
    GodotHostResourceManager resources;
    REQUIRE(resources.boot(vivarium_config()) == ase::kernel::HostStatusOk);
    CHECK(resources.running());
    CHECK(resources.system_count() == 2u);
    CHECK(resources.patch_count() == 4u);
    CHECK(std::string(resources.failure_step()).empty());
    CHECK(resources.failure_status() == ase::kernel::HostStatusOk);

    const double moistures[4] = {100.0, 70.0, 40.0, 10.0};
    for (uint32_t id = 1u; id <= 4u; ++id) {
        const VivariumPatchView* patch = resources.get_patch(id);
        REQUIRE(patch != nullptr);
        CHECK(patch->object_id == id);
        CHECK(near(patch->moisture, moistures[id - 1u]));
        CHECK(near(patch->biomass, 0.0));
        CHECK(near(patch->age_seconds, 0.0));
        CHECK(patch->seed);
        CHECK(!patch->sprout);
        CHECK(!patch->mature);
        CHECK(!patch->dead);
    }
    CHECK(!resources.has_patch(5u));
    CHECK(resources.get_patch(5u) == nullptr);

    // The bundle index named the plugin it carries.
    CHECK(resources.get_bundle().plugin == "ase-pl-vegetation");
    CHECK(!resources.get_bundle().version.empty());
    CHECK(!resources.get_bundle().api_version.empty());
}

TEST_CASE("V2: watering patch 4 raises only its moisture, and only after the next Regulation step") {
    GodotHostResourceManager resources;
    REQUIRE(resources.boot(vivarium_config()) == ase::kernel::HostStatusOk);
    CHECK(resources.irrigate(4u) == ase::kernel::HostStatusOk);
    // Nothing optimistic: the view still shows what the port reported.
    CHECK(near(moisture_of(resources, 4u), 10.0));

    advance_steps(resources, 1u);
    CHECK(near(moisture_of(resources, 4u), 34.25));  // 10 + 25 - 0.75
    CHECK(near(moisture_of(resources, 1u), 99.25));  // the others only drain
    CHECK(near(moisture_of(resources, 2u), 69.25));
    CHECK(near(moisture_of(resources, 3u), 39.25));

    // Consumed once: the next step only drains.
    advance_steps(resources, 1u);
    CHECK(near(moisture_of(resources, 4u), 33.5));

    // An id the plugin does not know is refused at the port and changes nothing.
    CHECK(resources.irrigate(9u) == ase::kernel::HostStatusNotFound);
}

TEST_CASE("V3: patch 4 dies after seven seconds, patches 1 to 3 reach full cover at twenty") {
    GodotHostResourceManager resources;
    REQUIRE(resources.boot(vivarium_config()) == ase::kernel::HostStatusOk);

    advance_steps(resources, 14u);  // 7 s: 10 - 14 x 0.75 reaches 0
    const VivariumPatchView* fourth = resources.get_patch(4u);
    REQUIRE(fourth != nullptr);
    CHECK(fourth->dead);
    const VivariumPatchView* first = resources.get_patch(1u);
    REQUIRE(first != nullptr);
    CHECK(first->sprout);  // biomass 28, still growing
    CHECK(near(first->biomass, 28.0));

    advance_steps(resources, 26u);  // 20 s in all
    const double moistures[3] = {70.0, 40.0, 10.0};
    for (uint32_t id = 1u; id <= 3u; ++id) {
        const VivariumPatchView* patch = resources.get_patch(id);
        REQUIRE(patch != nullptr);
        CHECK(patch->mature);
        CHECK(near(patch->biomass, 80.0));
        CHECK(near(patch->moisture, moistures[id - 1u]));
    }
    CHECK(resources.get_patch(4u)->dead);
    CHECK(near(resources.simulation_seconds(), 20.0));
    CHECK(resources.tick_count() == 160u);

    // A dead patch refuses water at the port (Phase 00, I2).
    CHECK(resources.irrigate(4u) == ase::kernel::HostStatusInvalidState);
}

TEST_CASE("dt: a long frame is capped, a broken frame is refused without a tick") {
    GodotHostResourceManager resources;
    REQUIRE(resources.boot(vivarium_config()) == ase::kernel::HostStatusOk);

    // A one-second stall advances the App by the cap only - no catch-up.
    CHECK(resources.advance(1.0f) == ase::kernel::HostStatusOk);
    CHECK(resources.tick_count() == 1u);
    CHECK(near(resources.simulation_seconds(), 0.125));

    CHECK(resources.advance(std::numeric_limits<float>::quiet_NaN()) ==
          ase::kernel::HostStatusInvalidArgument);
    CHECK(resources.advance(std::numeric_limits<float>::infinity()) ==
          ase::kernel::HostStatusInvalidArgument);
    CHECK(resources.advance(0.0f) == ase::kernel::HostStatusInvalidArgument);
    CHECK(resources.advance(-0.125f) == ase::kernel::HostStatusInvalidArgument);

    // Refused frames are dropped: no tick counted, no time added, the host keeps running.
    CHECK(resources.tick_count() == 1u);
    CHECK(near(resources.simulation_seconds(), 0.125));
    CHECK(resources.running());
    CHECK(std::string(resources.failure_step()).empty());
}

TEST_CASE("V7: twenty restarts - always four patches, two systems, the start values, no old input") {
    GodotHostResourceManager resources;
    for (uint32_t cycle = 0u; cycle < 20u; ++cycle) {
        REQUIRE(resources.boot(vivarium_config()) == ase::kernel::HostStatusOk);
        CHECK(resources.system_count() == 2u);
        CHECK(resources.patch_count() == 4u);
        CHECK(resources.tick_count() == 0u);
        CHECK(near(resources.simulation_seconds(), 0.0));
        CHECK(near(moisture_of(resources, 1u), 100.0));

        // Water queued in the PREVIOUS cycle must not arrive here: only the drain of one step.
        advance_steps(resources, 1u);
        CHECK(near(moisture_of(resources, 1u), 99.25));

        // Left pending on purpose - the next boot tears it down with its host.
        CHECK(resources.irrigate(1u) == ase::kernel::HostStatusOk);
    }
    resources.clear_all();
    CHECK(!resources.has_host());
}

TEST_CASE("V8: a bundle without its plugin library fails at load_plugin, loudly and without a simulation") {
    // A copy of the real bundle WITHOUT the library: index and manifest name a file that is gone.
    std::string manifest_name;
    {
        GodotHostResourceManager reference;
        REQUIRE(reference.boot(vivarium_config()) == ase::kernel::HostStatusOk);
        manifest_name = reference.get_bundle().manifest;
    }
    const std::string bundle = ASE_ADP_GODOT_TEST_BUNDLE;
    const std::string broken = bundle + "-without-library";
    REQUIRE(ase::fileio::create_directories(broken));
    REQUIRE(ase::fileio::write_text(ase::fileio::path_join(broken, GODOT_BUNDLE_INDEX),
                                    ase::fileio::read_text(ase::fileio::path_join(bundle, GODOT_BUNDLE_INDEX))));
    REQUIRE(ase::fileio::write_text(ase::fileio::path_join(broken, manifest_name),
                                    ase::fileio::read_text(ase::fileio::path_join(bundle, manifest_name))));

    VivariumStartConfig config = vivarium_config();
    config.bundle_dir = broken;
    GodotHostResourceManager resources;
    CHECK(resources.boot(config) == ase::kernel::HostStatusNotFound);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_LOAD);
    CHECK(resources.failure_status() == ase::kernel::HostStatusNotFound);

    // No stand-in simulation: no patch, no system, no tick, no water.
    CHECK(!resources.running());
    CHECK(resources.patch_count() == 0u);
    CHECK(resources.system_count() == 0u);
    CHECK(resources.advance(0.125f) == ase::kernel::HostStatusInvalidState);
    CHECK(resources.irrigate(1u) == ase::kernel::HostStatusInvalidState);

    // The stopped host is kept, so its log carries the cause.
    CHECK(resources.has_host());
    std::vector<std::string> lines;
    CHECK(resources.remove_log_lines(lines) > 0u);
    CHECK(any_line_carries(lines, std::string("failed at ") + GODOT_STEP_LOAD));
}

TEST_CASE("a missing bundle and an incomplete configuration are named by their step") {
    VivariumStartConfig nowhere = vivarium_config();
    nowhere.bundle_dir = "/nonexistent/avt-bundle";
    GodotHostResourceManager resources;
    CHECK(resources.boot(nowhere) == ase::kernel::HostStatusNotFound);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_BUNDLE);
    CHECK(!resources.running());

    VivariumStartConfig no_cap = vivarium_config();
    no_cap.tick_max_seconds = 0.0f;
    CHECK(resources.boot(no_cap) == ase::kernel::HostStatusInvalidArgument);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_CONFIG);
    // Refused before a host exists.
    CHECK(!resources.has_host());

    VivariumStartConfig wrong_port = vivarium_config();
    wrong_port.port = "vegetation.patch.v9";
    CHECK(resources.boot(wrong_port) == ase::kernel::HostStatusNotFound);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_PORT);

    // POSITIVE CONTROL: the same manager boots the right configuration afterwards.
    CHECK(resources.boot(vivarium_config()) == ase::kernel::HostStatusOk);
    CHECK(std::string(resources.failure_step()).empty());
}

TEST_CASE("the host's lines and the view's own line reach the queue and the log file") {
    const std::string path = ASE_ADP_GODOT_TEST_LOG;
    REQUIRE(path.front() == '/');
    (void)ase::utils::fs::remove(path);

    VivariumStartConfig config = vivarium_config();
    config.log_file = path;
    {
        GodotHostResourceManager resources;
        REQUIRE(resources.boot(config) == ase::kernel::HostStatusOk);
        CHECK(resources.note("boot line probe") == ase::kernel::HostStatusOk);

        std::vector<std::string> lines;
        CHECK(resources.remove_log_lines(lines) > 0u);
        CHECK(any_line_carries(lines, "[KernelEmbeddedHost] started"));
        CHECK(any_line_carries(lines, "[AVT] boot line probe"));
        for (const std::string& line : lines) {
            CHECK((line.empty() || line.back() != '\n'));
        }
        // Drained means drained.
        std::vector<std::string> again;
        CHECK(resources.remove_log_lines(again) == 0u);

        resources.clear_all();
        CHECK(resources.note("after the host") == ase::kernel::HostStatusInvalidState);
    }
    CHECK(ase::utils::fs::is_regular_file(path));
}

TEST_CASE("clear_all is idempotent and leaves no host behind") {
    GodotHostResourceManager resources;
    REQUIRE(resources.boot(vivarium_config()) == ase::kernel::HostStatusOk);
    resources.clear_all();
    resources.clear_all();
    CHECK(!resources.has_host());
    CHECK(!resources.running());
    CHECK(resources.system_count() == 0u);
    CHECK(resources.patch_count() == 0u);
    CHECK(resources.advance(0.125f) == ase::kernel::HostStatusInvalidState);
}

}  // namespace ase::adp::godot
