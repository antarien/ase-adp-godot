#pragma once

/**
 * ASE GODOT ADAPTER - ASSET BINDING
 *
 * @file        godot_asset_binding.hpp
 * @brief       Binds every name of the asset list to a file of the asset manifest, or reports it missing
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2: config/vivarium_assets.json names, per play
 *              stage, every material and organ template the view needs; the asset set of Phase 04
 *              arrives under godot/assets/ with a manifest of name, file and checksum. The view binds
 *              each name to the file the manifest names - only when the file's SHA-256 matches the
 *              manifest's. A name the manifest does not carry, one it carries twice, or one whose file
 *              does not match is MISSING: the boot line says `asset missing <name>` once per name and
 *              the view draws what it stood for in the error material. There is no silent stand-in.
 *
 *              NO GODOT TYPE: the caller reads the two JSON files and hands lists in; the checksum of
 *              a file comes through the caller's VivariumHashFn (the node: FileAccess::get_sha256), so
 *              the host tests bind against a test manifest without a Godot process. The colour a role
 *              is drawn with comes from asset_color, so the same test sees the error material.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100344 - verify assets in the package
 * @actor       Smartphone App Integration (SMARTAPP)
 * @created     2026-10-09
 * @modified    2026-10-18
 * @version     00.00.02.00002
 */

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/** One entry of the asset manifest (Phase 04, 04.4): the name, the file under the asset root, its SHA-256. */
struct VivariumAssetEntry {
    std::string name;
    std::string file;
    std::string sha256;   // 64 hex digits, either case
};

/**
 * The SHA-256 of a file the manifest names, as 64 hex digits; "" when the file cannot be read. The
 * caller resolves `file` against its asset root.
 */
using VivariumHashFn = std::string (*)(const std::string& file, void* user);

/** What the binding found: the file of every bound name, and the missing names in list order. */
struct VivariumAssetBinding {
    std::unordered_map<std::string, std::string> files;   // bound name → file
    std::vector<std::string>                     missing;  // each missing name once
};

/**
 * Binds `names` (the asset list of one play stage) against `manifest`. A name stands once in the
 * result however often the list names it.
 */
[[nodiscard]] VivariumAssetBinding bind_assets(const std::vector<std::string>& names,
                                               const std::vector<VivariumAssetEntry>& manifest, VivariumHashFn hash,
                                               void* user);

/** The boot line of one missing name: "asset missing <name>". */
[[nodiscard]] std::string asset_missing_line(const std::string& name);

/**
 * The colour the view draws a role with whose asset is `name`: `bound_color` when the binding holds
 * a file for it, else the error material GODOT_COLOR_ASSET_MISSING - one signal colour for every
 * missing name, an empty name included, never a colour of its own.
 */
[[nodiscard]] uint32_t asset_color(const VivariumAssetBinding& binding, const std::string& name, uint32_t bound_color);

}  // namespace ase::adp::godot
