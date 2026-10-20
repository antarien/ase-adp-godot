#pragma once

/**
 * ASE GODOT ADAPTER - THE LINE OF ONE MEASURED CYCLE
 *
 * @file        godot_cycle_measure.hpp
 * @brief       One line per cycle: organs per class and branching order, compute time per unit and schedule, PSS
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6 point 1: a run lets the birch grow and writes one
 *              line per cycle - the organ entities per class and per branching order, the compute time
 *              of every unit per schedule, the memory of the process. The run goes headless on Linux
 *              (avt-headless) and inside the app under --avt-measure on the emulator; both build the line
 *              here, so the two runs cannot measure two different things.
 *
 *              EVERYTHING COMES THROUGH THE FACADE: the organs from the snapshots of the organ ports
 *              (every record carries its branching order), the compute time from
 *              KernelEmbeddedHost::unit_time as running totals this measure differences against its
 *              own cursor, the memory from /proc/self/smaps_rollup. No EnTT, no unit header, no Godot
 *              type - the host tests drive it against the real bundle.
 *
 *              WHAT THE PORTS DO NOT SAY: the organ ports of ase-pl-flora publish internodes and axes
 *              (segment), leaves, male and female catkins, infructescences and root segments; a bud and
 *              a root tip are no port of their own. The line counts the classes the ports carry and
 *              names each by the word of its port, so a class the ports add later shows up without a
 *              change here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    process/validation/check
 * @usecase     usecase:6540 - generate plant organs
 * @actor       Procedural Phyllotaxis Generation (PPG)
 * @created     2026-10-10
 * @modified    2026-10-10
 * @version     00.00.01.00001
 */

#include <ase/kernel/kernel_embedded_host.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What one measurement run reads, and the cursors it differences against. */
struct CycleMeasure {
    std::vector<std::string> units;        // unit names as the units name themselves (the bundle index)
    std::vector<std::string> schedules;    // schedule names as ase-ecs prints them
    std::vector<std::string> organ_words;  // the word of each organ port (segment, leaf, ...)
    std::vector<std::string> organ_ports;  // the port of each word, same order
    std::vector<uint64_t>    seen_us;      // cursor per (unit, schedule): row by unit, units x schedules
    uint64_t                 cycle = 0;    // lines written so far
};

/**
 * true when the measure can run: at least one unit, schedule and organ port, a port for every word,
 * and no name empty.
 */
[[nodiscard]] bool cycle_measure_valid(const CycleMeasure& measure);

/**
 * Set every cursor to the total the host holds now, so the first line counts its own cycle only.
 * Answers false when the host refused a read (an unknown unit or schedule, a stopped host); the
 * cursors are then all 0.
 */
bool cycle_measure_begin(CycleMeasure& measure, const ase::kernel::KernelEmbeddedHost& host);

/**
 * The line of the cycle that just ended, written into `line`, and the cursors moved on:
 *
 *   cycle=<n> organs <word>=<count>... orders <order>=<count>... time_us <unit>@<schedule>=<us>...
 *   time_us_cycle=<us> pss_kib=<kib|unknown>
 *
 * Only a (unit, schedule) pair that took time in this cycle is named; time_us_cycle sums them all.
 * Answers false when the host refused a snapshot or a time read - the line then ends with
 * "refused=<what>" and the cursors stand where they stood.
 */
bool cycle_measure_line(CycleMeasure& measure, ase::kernel::KernelEmbeddedHost& host, std::string& line);

/**
 * The proportional set size of this process in KiB (/proc/self/smaps_rollup, its "Pss:" line).
 * Answers false where the file cannot be read or carries no such line; out is then 0.
 */
[[nodiscard]] bool read_pss_kib(uint64_t& out);

}  // namespace ase::adp::godot
