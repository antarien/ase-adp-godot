/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW MODES
 *
 * @file        godot_vivarium_measure.cpp
 * @brief       VivariumView - the two explicit modes: the self-test before the game host and the measurement of a run
 * @description TWO EXPLICIT MODES (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3), off unless the launch names them:
 *              the self-test runs the table config/vivarium_selftest.toml on one test host of its own
 *              BEFORE the game host boots; the measurement records host and frame times after a warm-up
 *              and writes one block when the recording is complete, nothing per frame (acceptance A10).
 *              While it records, every new game day writes the line of the cycle that ended (03.6, the
 *              line the headless run writes too, godot_cycle_measure.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_016
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Component System (ECS)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/file_access.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

/** Nearest-rank percentile of `samples` (taken by value: the order of the recording stays). */
uint32_t percentile(std::vector<uint32_t> samples, uint32_t percent) {
    if (samples.empty()) {
        return 0u;
    }
    const std::size_t rank =
        (static_cast<std::size_t>(percent) * samples.size() + GODOT_MEASURE_PERCENT - 1u) / GODOT_MEASURE_PERCENT;
    const std::size_t index = rank == 0u ? 0u : rank - 1u;
    std::nth_element(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(index), samples.end());
    return samples[index];
}

/** "p50=.. p95=.. p99=.. max=.." of one recorded series, in microseconds. */
::godot::String series(const std::vector<uint32_t>& samples) {
    const uint32_t largest = samples.empty() ? 0u : *std::max_element(samples.begin(), samples.end());
    return "p50=" + ::godot::String::num_uint64(percentile(samples, GODOT_MEASURE_P50)) +
           " p95=" + ::godot::String::num_uint64(percentile(samples, GODOT_MEASURE_P95)) +
           " p99=" + ::godot::String::num_uint64(percentile(samples, GODOT_MEASURE_P99)) +
           " max=" + ::godot::String::num_uint64(largest);
}

/** A microsecond count as a sample: a frame longer than 71 minutes is a stall, held at the top. */
uint32_t sample_of(uint64_t usec) {
    return usec > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(usec);
}

}  // namespace

void VivariumView::run_self_test() {
    // The table is a staged file like the start configuration; a missing one is reported by the
    // manager as a self-test that could not run - never as one without errors.
    const ::godot::String table = ::godot::FileAccess::get_file_as_string(modes_.selftest_table);
    selftest_report_.clear();
    const VivariumSelfTestResult result = resources_.self_test(start_, utf8_of(table), selftest_report_);
    selftest_ran_ = true;
    selftest_errors_ = result.errors;
    drain_log();  // the test host's own lines come before the game host's
}

void VivariumView::measure_frame(uint64_t frame_start_usec, uint64_t host_usec) {
    VivariumMeasurement& measurement = measurement_;
    const uint64_t previous = measurement.last_usec;
    measurement.last_usec = frame_start_usec;
    // The first frame after a start has no interval of its own.
    if (previous == 0u || frame_start_usec <= previous) {
        return;
    }
    const uint64_t interval = frame_start_usec - previous;
    const double seconds = static_cast<double>(interval) / GODOT_USEC_PER_SECOND;
    if (!measurement.recording) {
        measurement.warmup_left -= seconds;
        if (measurement.warmup_left > 0.0) {
            return;
        }
        measurement.recording = true;
        const auto reserved = static_cast<std::size_t>(modes_.measure_record_seconds * GODOT_MEASURE_FRAMES_RESERVED);
        measurement.host_usec.reserve(reserved);
        measurement.frame_usec.reserve(reserved);
        measurement.pss_start_read = resources_.read_pss_kib(measurement.pss_start_kib);
        measurement.boots_at_start = host_boots_;
        note("measure begin warmup_s=" + ::godot::String::num(modes_.measure_warmup_seconds, GODOT_LOG_DECIMALS) +
             " record_s=" + ::godot::String::num(modes_.measure_record_seconds, GODOT_LOG_DECIMALS) + " " +
             measure(GODOT_NO_PLACE));
        begin_cycles();
        return;
    }
    measurement.host_usec.push_back(sample_of(host_usec));
    measurement.frame_usec.push_back(sample_of(interval));
    measure_cycle();
    measurement.record_left -= seconds;
    if (measurement.record_left <= 0.0) {
        report_measurement();
    }
}

void VivariumView::begin_cycles() {
    // THE LINE OF 03.6, the one the headless run writes too (godot_cycle_measure.hpp): every unit of
    // the bundle, every schedule ase-ecs names, every organ port of the start configuration.
    CycleMeasure& cycles = measurement_.cycles;
    cycles = CycleMeasure{};
    for (const VivariumBundleUnit& unit : resources_.get_bundle().units) {
        cycles.units.push_back(unit.unit);
    }
    for (uint32_t i = 0u; i < ase::kernel::KernelEmbeddedHost::schedule_count(); ++i) {
        cycles.schedules.emplace_back(ase::kernel::KernelEmbeddedHost::schedule_name(i));
    }
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        cycles.organ_words.emplace_back(GodotHostResourceManager::organ_word(c));
        cycles.organ_ports.push_back(start_.organ_ports[c]);
    }
    measurement_.cycles_ready = cycle_measure_valid(cycles) && resources_.measure_begin(cycles);
    const VivariumClockView& clock = resources_.get_clock();
    measurement_.cycle_day = clock.known ? clock.day : -1.0;
    if (!measurement_.cycles_ready) {
        note("measure cycles off - units, schedules or organ ports not readable");
    }
}

void VivariumView::measure_cycle() {
    if (!measurement_.cycles_ready) {
        return;
    }
    const VivariumClockView& clock = resources_.get_clock();
    if (!clock.known) {
        return;
    }
    if (measurement_.cycle_day < 0.0) {
        // The clock arrived after the recording began: this day is the first cycle's start.
        measurement_.cycle_day = clock.day;
        return;
    }
    if (clock.day == measurement_.cycle_day) {
        return;
    }
    measurement_.cycle_day = clock.day;
    std::string line;
    resources_.measure_cycle(measurement_.cycles, line);
    note("measure " + text_of(line));
}

void VivariumView::report_measurement() {
    VivariumMeasurement& measurement = measurement_;
    uint64_t pss_end_kib = 0u;
    const bool pss_end_read = resources_.read_pss_kib(pss_end_kib);
    uint64_t recorded_usec = 0u;
    for (const uint32_t interval : measurement.frame_usec) {
        recorded_usec += interval;
    }
    const double recorded_seconds = static_cast<double>(recorded_usec) / GODOT_USEC_PER_SECOND;
    const double fps = recorded_seconds > 0.0 ? static_cast<double>(measurement.frame_usec.size()) / recorded_seconds : 0.0;
    // ONE BLOCK, written when the recording is complete: nothing was logged per frame.
    note("measure host_usec frames=" + ::godot::String::num_uint64(measurement.host_usec.size()) + " " +
         series(measurement.host_usec));
    note("measure frame_usec frames=" + ::godot::String::num_uint64(measurement.frame_usec.size()) + " " +
         series(measurement.frame_usec) + " fps=" + ::godot::String::num(fps, GODOT_LOG_DECIMALS) +
         " recorded_s=" + ::godot::String::num(recorded_seconds, GODOT_LOG_DECIMALS));
    note("measure end pss_kib_start=" +
         (measurement.pss_start_read ? ::godot::String::num_uint64(measurement.pss_start_kib) : ::godot::String("unknown")) +
         " pss_kib_end=" + (pss_end_read ? ::godot::String::num_uint64(pss_end_kib) : ::godot::String("unknown")) +
         " restarts=" + ::godot::String::num_uint64(host_boots_ - measurement.boots_at_start) +
         " catch_ups=" + ::godot::String::num_uint64(measurement.catch_ups) + " " + measure(GODOT_NO_PLACE));
    measurement = VivariumMeasurement{};
}

}  // namespace ase::adp::godot
