/**
 * ASE GODOT ADAPTER - ONE VALUE OF A HOST RECORD
 *
 * @file        godot_host_record.cpp
 * @brief       The value a port record carries under a key, and the test of a whole number in it
 * @description The two questions every reader of a port record asks: which value stands under a key, and
 *              whether it is a whole number the port uses for a place, a body or a code. The readers of the
 *              organ records, the terrain samples and the resource manager share this one answer, so no
 *              unit reads a record its own way.
 *
 *              NO GODOT TYPE: a record of the kernel in, a number out, so the host tests check it without
 *              a Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101344 - carry host records to app
 * @actor       Host Record Transport Control (HOSTXFER)
 * @created     2026-11-05
 * @modified    2026-11-05
 * @version     00.00.01.00000
 */

// Own header FIRST
#include <ase/adp/godot/godot_host_record.hpp>
#include <ase/math/math.hpp>
#include <ase/utils/strops.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

double record_value(const ase::kernel::KernelHostRecord& record, const char* key) {
    for (uint32_t i = 0u; i < record.value_count && i < ase::kernel::HostValuesMax; ++i) {
        if (ase::utils::str_equal(record.values[i].key, key, ase::kernel::HostKeyBytes)) {
            return record.values[i].value;
        }
    }
    return std::nan("");
}

bool whole(double value) {
    return std::isfinite(value) && value >= 0.0 &&
           ase::math::floor(static_cast<float>(value)) == static_cast<float>(value);
}

}  // namespace ase::adp::godot
