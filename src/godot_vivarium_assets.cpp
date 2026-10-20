/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW ASSET SET
 *
 * @file        godot_vivarium_assets.cpp
 * @brief       VivariumView - the asset list of the play stage, its roles and the manifest of the delivered set
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2 (res://config/vivarium_assets.json): load_assets reads
 *              the list of the play stage and the manifest, bind_assets checks every file by its SHA-256
 *              (godot_asset_binding.hpp). The asset list is configuration: without it, without its play
 *              stage or without a role the view draws with, the view does not start. The manifest is the
 *              delivery of Phase 04: without it every name is missing - reported name by name at every
 *              boot - and the view runs.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100344 - verify assets in the package
 * @actor       Smartphone App Integration (SMARTAPP)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/array.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

/**
 * VivariumHashFn of the view (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2): the SHA-256 of one file of
 * the asset set, through FileAccess - so an asset inside an exported package is checked like one on
 * disk. user is the view's asset root (res://assets); "" when the file cannot be read.
 */
std::string asset_checksum(const std::string& file, void* user) {
    const ::godot::String path = static_cast<const ::godot::String*>(user)->path_join(text_of(file));
    if (!::godot::FileAccess::file_exists(path)) {
        return std::string();
    }
    return utf8_of(::godot::FileAccess::get_sha256(path));
}

}  // namespace

bool VivariumView::load_assets(const ::godot::Dictionary& root) {
    // The asset list is configuration: without it, without its play stage or without a role the
    // view draws with, the view does not start. The manifest is the delivery of Phase 04: without
    // it every name is missing - reported name by name at every boot - and the view runs.
    double stage = 0.0;
    if (!read_number(root, "play_stage", stage) || !(stage >= 1.0) ||
        ase::math::floor(static_cast<float>(stage)) != static_cast<float>(stage)) {
        return fail_config("play_stage");
    }
    play_stage_ = static_cast<uint32_t>(stage);
    const ::godot::Variant parsed =
        ::godot::JSON::parse_string(::godot::FileAccess::get_file_as_string(GODOT_ASSETS_PATH));
    if (parsed.get_type() != ::godot::Variant::DICTIONARY) {
        return fail_config(GODOT_ASSETS_PATH);
    }
    const ::godot::Dictionary list = parsed;
    ::godot::String manifest_path;
    if (!read_text(list, GODOT_ASSETS_KEY_MANIFEST, manifest_path) ||
        !read_text(list, GODOT_ASSETS_KEY_ROOT, asset_root_)) {
        return fail_config(GODOT_ASSETS_KEY_MANIFEST);
    }
    const ::godot::Variant stages = list.get(GODOT_ASSETS_KEY_STAGES, ::godot::Variant());
    if (stages.get_type() != ::godot::Variant::ARRAY) {
        return fail_config(GODOT_ASSETS_KEY_STAGES);
    }
    // Exactly one entry lists the play stage: none leaves the view without names, two leave it a
    // choice the list does not make.
    const ::godot::Array entries = stages;
    ::godot::Dictionary chosen;
    uint32_t found = 0u;
    for (int64_t i = 0; i < entries.size(); ++i) {
        ::godot::Dictionary entry;
        double listed = 0.0;
        if (entries[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config(GODOT_ASSETS_KEY_STAGES);
        }
        entry = entries[i];
        if (!read_number(entry, GODOT_ASSETS_KEY_STAGE, listed)) {
            return fail_config(GODOT_ASSETS_KEY_STAGE);
        }
        if (listed == stage) {
            chosen = entry;
            found += 1u;
        }
    }
    if (found != 1u) {
        return fail_config(GODOT_ASSETS_KEY_STAGE);
    }
    asset_roles_.clear();
    asset_names_.clear();
    asset_materials_.clear();
    if (!load_asset_roles(chosen, GODOT_ASSETS_KEY_MATERIALS) || !load_asset_roles(chosen, GODOT_ASSETS_KEY_TEMPLATES)) {
        return false;
    }
    // Every role the view draws with stands in the list: a role without a name has nothing to bind
    // and nothing to report, so it could only be drawn with a colour of its own.
    if (asset_roles_.count(GODOT_ROLE_GROUND_MEADOW) == 0u || asset_roles_.count(GODOT_ROLE_GROUND_FOREST) == 0u) {
        return fail_config(GODOT_ASSETS_KEY_MATERIALS);
    }
    std::vector<VivariumAssetEntry> manifest;
    load_asset_manifest(manifest_path, manifest);
    assets_ = bind_assets(asset_names_, manifest, &asset_checksum, &asset_root_);
    return true;
}

bool VivariumView::load_asset_roles(const ::godot::Dictionary& stage, const char* section) {
    // role → asset name; a key starting with GODOT_ASSETS_COMMENT is the list's own comment. A role
    // stands once in the stage, whichever section names it.
    ::godot::Dictionary roles;
    if (!read_section(stage, section, roles)) {
        return fail_config(section);
    }
    const ::godot::Array keys = roles.keys();
    for (int64_t i = 0; i < keys.size(); ++i) {
        const ::godot::String role = keys[i];
        if (role.begins_with(GODOT_ASSETS_COMMENT)) {
            continue;
        }
        const ::godot::Variant value = roles.get(role, ::godot::Variant());
        if (value.get_type() != ::godot::Variant::STRING || static_cast<::godot::String>(value).is_empty() ||
            asset_roles_.count(utf8_of(role)) != 0u) {
            return fail_config(section);
        }
        const std::string asset = utf8_of(static_cast<::godot::String>(value));
        asset_roles_[utf8_of(role)] = asset;
        asset_names_.push_back(asset);
        // The showcase (04.7) shows a material on a sphere and a plane, a template as its model.
        if (std::string(section) == GODOT_ASSETS_KEY_MATERIALS) {
            asset_materials_.push_back(asset);
        }
    }
    return true;
}

void VivariumView::load_asset_manifest(const ::godot::String& path, std::vector<VivariumAssetEntry>& out) {
    // The delivery of Phase 04 (04.4, 04.5). An entry that is no object binds nothing; the name it
    // would have carried is then missing like every name the manifest does not carry.
    out.clear();
    if (!::godot::FileAccess::file_exists(path)) {
        asset_manifest_ = "absent";
        return;
    }
    const ::godot::Variant parsed = ::godot::JSON::parse_string(::godot::FileAccess::get_file_as_string(path));
    const ::godot::Variant entries = parsed.get_type() == ::godot::Variant::DICTIONARY
                                         ? static_cast<::godot::Dictionary>(parsed).get(GODOT_ASSETS_KEY_ENTRIES,
                                                                                        ::godot::Variant())
                                         : ::godot::Variant();
    if (entries.get_type() != ::godot::Variant::ARRAY) {
        asset_manifest_ = "broken";
        return;
    }
    asset_manifest_ = "read";
    const ::godot::Array list = entries;
    for (int64_t i = 0; i < list.size(); ++i) {
        if (list[i].get_type() != ::godot::Variant::DICTIONARY) {
            continue;
        }
        const ::godot::Dictionary entry = list[i];
        ::godot::String asset;
        ::godot::String file;
        ::godot::String checksum;
        // A field that is absent stays empty, and an empty field never binds (bind_assets).
        (void)read_text(entry, GODOT_ASSETS_KEY_ASSET, asset);
        (void)read_text(entry, GODOT_ASSETS_KEY_FILE, file);
        (void)read_text(entry, GODOT_ASSETS_KEY_SHA256, checksum);
        out.push_back(VivariumAssetEntry{utf8_of(asset), utf8_of(file), utf8_of(checksum)});
    }
}

}  // namespace ase::adp::godot
