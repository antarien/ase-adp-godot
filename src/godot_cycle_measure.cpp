/**
 * ASE GODOT ADAPTER - THE LINE OF ONE MEASURED CYCLE
 *
 * @file        godot_cycle_measure.cpp
 * @brief       One line per cycle: organs per class and branching order, compute time per unit and schedule, PSS
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6 point 1, see godot_cycle_measure.hpp. Reads only
 *              the kernel facade and /proc/self/smaps_rollup.
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
 *
 * WHY THE TIME IS A DIFFERENCE: KernelEmbeddedHost::unit_time answers running totals that only grow
 * (EcsAppStaModTimComponent). A measure that read them as they are would write the time since the
 * start into every line; the cursor per (unit, schedule) turns them into the time of one cycle, and
 * the totals themselves are never reset - another reader of the same host keeps its own cursor.
 */

#include <ase/adp/godot/godot_cycle_measure.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/fileio/text_reader.hpp>
#include <ase/utils/strops.hpp>

#include <cmath>

namespace ase::adp::godot {

namespace {

// Every record a port holds now; false when the host refused the snapshot.
bool port_records(ase::kernel::KernelEmbeddedHost& host, const std::string& port,
                  std::vector<ase::kernel::KernelHostRecord>& records) {
    records.assign(ase::kernel::HostRecordsMax, ase::kernel::KernelHostRecord{});
    uint32_t written = 0u;
    uint32_t required = 0u;
    ase::kernel::HostStatus status = host.snapshot(port.c_str(), records.data(),
                                                   static_cast<uint32_t>(records.size()), &written, &required);
    if (status == ase::kernel::HostStatusCapacity) {
        records.assign(required, ase::kernel::KernelHostRecord{});
        status = host.snapshot(port.c_str(), records.data(), static_cast<uint32_t>(records.size()),
                               &written, &required);
    }
    if (status != ase::kernel::HostStatusOk) {
        records.clear();
        return false;
    }
    records.resize(written);
    return true;
}

// The branching order a record carries, or GODOT_MEASURE_ORDERS when it carries none.
uint32_t order_of(const ase::kernel::KernelHostRecord& record) {
    for (uint32_t i = 0u; i < record.value_count; ++i) {
        if (ase::utils::str_equal(record.values[i].key, GODOT_KEY_ORDER, ase::kernel::HostKeyBytes)) {
            const double order = record.values[i].value;
            if (!std::isfinite(order) || order < 0.0) {
                return GODOT_MEASURE_ORDERS;
            }
            const double last = static_cast<double>(GODOT_MEASURE_ORDERS - 1u);
            return static_cast<uint32_t>(order < last ? order : last);
        }
    }
    return GODOT_MEASURE_ORDERS;
}

}  // namespace

bool cycle_measure_valid(const CycleMeasure& measure) {
    if (measure.units.empty() || measure.schedules.empty() || measure.organ_words.empty() ||
        measure.organ_words.size() != measure.organ_ports.size()) {
        return false;
    }
    for (const std::vector<std::string>* names : {&measure.units, &measure.schedules, &measure.organ_words,
                                                  &measure.organ_ports}) {
        for (const std::string& name : *names) {
            if (name.empty()) {
                return false;
            }
        }
    }
    return true;
}

bool cycle_measure_begin(CycleMeasure& measure, const ase::kernel::KernelEmbeddedHost& host) {
    measure.seen_us.assign(measure.units.size() * measure.schedules.size(), 0u);
    measure.cycle = 0u;
    for (std::size_t u = 0u; u < measure.units.size(); ++u) {
        for (std::size_t s = 0u; s < measure.schedules.size(); ++s) {
            uint64_t total = 0u;
            if (host.unit_time(measure.units[u].c_str(), measure.schedules[s].c_str(), &total, nullptr) !=
                ase::kernel::HostStatusOk) {
                measure.seen_us.assign(measure.units.size() * measure.schedules.size(), 0u);
                return false;
            }
            measure.seen_us[u * measure.schedules.size() + s] = total;
        }
    }
    return true;
}

bool cycle_measure_line(CycleMeasure& measure, ase::kernel::KernelEmbeddedHost& host, std::string& line) {
    line = "cycle=" + std::to_string(measure.cycle + 1u);

    // ORGANS: the records of every organ port, per class and per branching order.
    std::vector<uint64_t> per_order(GODOT_MEASURE_ORDERS + 1u, 0u);
    std::vector<ase::kernel::KernelHostRecord> records;
    line += " organs";
    for (std::size_t p = 0u; p < measure.organ_ports.size(); ++p) {
        if (!port_records(host, measure.organ_ports[p], records)) {
            line += " refused=snapshot:" + measure.organ_ports[p];
            return false;
        }
        line += " " + measure.organ_words[p] + "=" + std::to_string(records.size());
        for (const ase::kernel::KernelHostRecord& record : records) {
            per_order[order_of(record)] += 1u;
        }
    }
    line += " orders";
    for (uint32_t order = 0u; order < GODOT_MEASURE_ORDERS; ++order) {
        if (per_order[order] == 0u) {
            continue;
        }
        line += " " + std::to_string(order) + (order + 1u == GODOT_MEASURE_ORDERS ? "+" : "") + "=" +
                std::to_string(per_order[order]);
    }
    if (per_order[GODOT_MEASURE_ORDERS] > 0u) {
        line += " none=" + std::to_string(per_order[GODOT_MEASURE_ORDERS]);
    }

    // COMPUTE TIME: the difference of every running total against its cursor. The cursors move
    // only once every read succeeded, so a refused read leaves the next line its whole cycle.
    if (measure.seen_us.size() != measure.units.size() * measure.schedules.size()) {
        line += " refused=begin";
        return false;
    }
    std::vector<uint64_t> now(measure.seen_us.size(), 0u);
    std::string times;
    uint64_t cycle_us = 0u;
    for (std::size_t u = 0u; u < measure.units.size(); ++u) {
        for (std::size_t s = 0u; s < measure.schedules.size(); ++s) {
            const std::size_t at = u * measure.schedules.size() + s;
            if (host.unit_time(measure.units[u].c_str(), measure.schedules[s].c_str(), &now[at], nullptr) !=
                ase::kernel::HostStatusOk) {
                line += " refused=time:" + measure.units[u] + "@" + measure.schedules[s];
                return false;
            }
            const uint64_t spent = now[at] >= measure.seen_us[at] ? now[at] - measure.seen_us[at] : 0u;
            if (spent > 0u) {
                times += " " + measure.units[u] + "@" + measure.schedules[s] + "=" + std::to_string(spent);
                cycle_us += spent;
            }
        }
    }
    measure.seen_us = now;
    measure.cycle += 1u;
    line += " time_us" + times + " time_us_cycle=" + std::to_string(cycle_us);

    uint64_t pss_kib = 0u;
    line += " pss_kib=" + (read_pss_kib(pss_kib) ? std::to_string(pss_kib) : std::string("unknown"));
    return true;
}

bool read_pss_kib(uint64_t& out) {
    out = 0u;
    // smaps_rollup: a header line, then "Key:   value kB" lines. The key is matched at a line
    // start - "SwapPss:" further down ends in the same four characters.
    const std::string text = ase::fileio::read_text(GODOT_PSS_SOURCE);
    const std::size_t at = text.find(std::string("\n") + GODOT_PSS_KEY);
    if (at == std::string::npos) {
        return false;
    }
    std::size_t digit = text.find_first_of("0123456789", at);
    const std::size_t line_end = text.find('\n', at + 1u);
    if (digit == std::string::npos || (line_end != std::string::npos && digit > line_end)) {
        return false;
    }
    uint64_t value = 0u;
    while (digit < text.size() && text[digit] >= '0' && text[digit] <= '9') {
        value = value * 10u + static_cast<uint64_t>(text[digit] - '0');
        digit += 1u;
    }
    out = value;
    return true;
}

}  // namespace ase::adp::godot
