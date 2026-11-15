/**
 * ASE GODOT ADAPTER - THE LEAF OF THE CARE VIEW IN THE COLOUR OF ITS SEASON
 *
 * @file        godot_plant_leaf_season.cpp
 * @brief       The season's colour and the dry olive the leaf of the care view's 3D scene is painted in
 * @description See godot_plant_leaf_season.hpp. One write per global reaches every leaf material at once, so the
 *              leaves need no list and no material is touched per frame.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_leaf_season.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ase::adp::godot {

void light_plant_leaf_season(const LeafSeasonShade& shade) {
    ::godot::RenderingServer* server = ::godot::RenderingServer::get_singleton();
    if (server == nullptr) {
        return;
    }
    // not shown, the defaults leave the leaf as its material paints it: no season's colour, no dry olive
    const LeafSeasonShade used = shade.shown ? shade : LeafSeasonShade{};
    // one line whenever what the leaf is painted with moves (register B0278): the season's colour and share and the
    // olive's, measured against the values the last line named, so the log holds each change once and never a line
    // per frame. They are kept as a meta of the server that owns the globals, since outside the editor Godot answers
    // global_shader_parameter_get with an error and an empty value
    ::godot::PackedFloat32Array now;
    now.push_back(used.season.r);
    now.push_back(used.season.g);
    now.push_back(used.season.b);
    now.push_back(used.season.a);
    now.push_back(used.dry.r);
    now.push_back(used.dry.g);
    now.push_back(used.dry.b);
    now.push_back(used.dry.a);
    const ::godot::PackedFloat32Array noted = server->get_meta(GODOT_LEAF_NOTE_META, ::godot::PackedFloat32Array());
    bool moved = noted.size() != now.size();
    for (int64_t i = 0; !moved && i < now.size(); ++i) {
        moved = ::godot::Math::abs(noted[i] - now[i]) > GODOT_LEAF_NOTE_STEP;
    }
    if (moved) {
        server->set_meta(GODOT_LEAF_NOTE_META, now);
        ::godot::UtilityFunctions::print(::godot::String("[") + GODOT_LOG_SOURCE + "] leaf season shown=" +
                                         (shade.shown ? "1" : "0") + " season=" + used.season.to_html(true) +
                                         " dry=" + used.dry.to_html(true));
    }
    server->global_shader_parameter_set(GODOT_LEAF_SEASON_COLOR, used.season);
    server->global_shader_parameter_set(GODOT_LEAF_DRY_COLOR, used.dry);
}

}  // namespace ase::adp::godot
