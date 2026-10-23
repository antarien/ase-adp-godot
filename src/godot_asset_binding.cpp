/**
 * ASE GODOT ADAPTER - ASSET BINDING IMPLEMENTATION
 *
 * @file        godot_asset_binding.cpp
 * @brief       Name → file of the asset manifest, checked by SHA-256, or missing
 * @description See godot_asset_binding.hpp. A checksum is compared without regard to the case of its
 *              hex digits - the manifest may write either, the hash function of the caller the other.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100344 - verify assets in the package
 * @actor       Smartphone App Integration (SMARTAPP)
 * @created     2026-10-09
 * @modified    2026-10-23
 * @version     00.00.03.00003
 */

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

namespace {

/** Two checksums as the same 64 hex digits, whatever the case; an empty one matches nothing. */
bool same_checksum(const std::string& a, const std::string& b) {
    if (a.size() != GODOT_SHA256_HEX_DIGITS || b.size() != GODOT_SHA256_HEX_DIGITS) {
        return false;
    }
    for (std::string::size_type i = 0u; i < a.size(); ++i) {
        const char x = a[i] >= 'A' && a[i] <= 'F' ? static_cast<char>(a[i] - 'A' + 'a') : a[i];
        const char y = b[i] >= 'A' && b[i] <= 'F' ? static_cast<char>(b[i] - 'A' + 'a') : b[i];
        if (x != y) {
            return false;
        }
    }
    return true;
}

}  // namespace

VivariumAssetBinding bind_assets(const std::vector<std::string>& names,
                                 const std::vector<VivariumAssetEntry>& manifest,
                                 const std::unordered_map<std::string, uint32_t>& stand_ins, VivariumHashFn hash,
                                 void* user) {
    VivariumAssetBinding binding;
    std::unordered_map<std::string, bool> seen;
    for (const std::string& name : names) {
        if (seen.count(name) != 0u) {
            continue;
        }
        seen[name] = true;
        const auto colour = stand_ins.find(name);
        if (colour != stand_ins.end()) {
            binding.stand_ins[name] = colour->second;
        }
        const VivariumAssetEntry* entry = nullptr;
        bool twice = false;
        for (const VivariumAssetEntry& candidate : manifest) {
            if (candidate.name == name) {
                twice = twice || entry != nullptr;
                entry = &candidate;
            }
        }
        // missing: not in the manifest, named there twice, or a file that does not match its checksum
        if (entry == nullptr || twice || entry->file.empty() || hash == nullptr ||
            !same_checksum(hash(entry->file, user), entry->sha256)) {
            binding.missing.push_back(name);
            continue;
        }
        binding.files[name] = entry->file;
    }
    return binding;
}

std::string asset_missing_line(const std::string& name) {
    return std::string(GODOT_ASSET_MISSING) + name;
}

uint32_t asset_color(const VivariumAssetBinding& binding, const std::string& name, uint32_t bound_color) {
    if (binding.files.count(name) != 0u) {
        return bound_color;
    }
    const auto colour = binding.stand_ins.find(name);
    return colour == binding.stand_ins.end() ? GODOT_COLOR_ASSET_MISSING : colour->second;
}

}  // namespace ase::adp::godot
