/**
 * ASE GODOT ADAPTER - HUD CONTAINER DEF
 *
 * @file        godot_hud_container_def.cpp
 * @brief       One container of the care view: drawing side from SHA, physical side from the carry bridge
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_container_def.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_container_def.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ase::adp::godot {

namespace {

/** Reads hud_<family>_<field> into `out`; false with the token name in `failed`. */
bool read_family_token(const UiTheme& theme, const std::string& family, const char* field, double& out,
                       std::string& failed) {
    const std::string token = "hud_" + family + "_" + field;
    if (!theme.measure(token, out) || !(out > 0.0)) {
        failed = token;
        return false;
    }
    return true;
}

}  // namespace

bool ContainerDef::bind(const UiTheme& theme, const std::string& family, std::string& failed) {
    if (family.empty()) {
        failed = "hud.container_def.family";
        return false;
    }
    if (!read_family_token(theme, family, "content_w", content_w_, failed) ||
        !read_family_token(theme, family, "content_h", content_h_, failed) ||
        !read_family_token(theme, family, "per_cm", per_cm_, failed) ||
        !read_family_token(theme, family, "item_min", item_min_, failed)) {
        return false;
    }
    family_ = family;
    measured_ = false;
    logged_ = false;
    return true;
}

void ContainerDef::carry(const ContainerDefCarry* record) {
    if (record != nullptr) {
        physical_ = *record;
        measured_ = true;
        logged_ = false;
        return;
    }
    measured_ = false;  // the last record stands: the picture stays whole
    if (logged_) {
        return;
    }
    logged_ = true;
    ::godot::UtilityFunctions::push_warning(::godot::String("ContainerDef ") + family_.c_str() + ": no record " +
                                            gap() + " - the carry bridge handed this frame no container record");
}

const char* ContainerDef::gap() const {
    return GODOT_HUD_GAP_CARRY;
}

double ContainerDef::drawn_box(double length_cm) const {
    return ase::math::max(item_min_, length_cm * per_cm_);
}

}  // namespace ase::adp::godot
