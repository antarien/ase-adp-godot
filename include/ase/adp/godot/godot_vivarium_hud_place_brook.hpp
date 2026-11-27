#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD PLACES: THE BROOK
 *
 * @file        godot_vivarium_hud_place_brook.hpp
 * @brief       The point of the first brook's bed a tap in the mode water lands in, for the place lines
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.11: VivariumView::note_hud_places (godot_vivarium_hud_place.cpp)
 *              names every place the window acceptance taps; in the mode water one of them is the brook (`brook`),
 *              the point a tap on the ground asks brookTap at (ACT_GROUND sends ASK_BROOK there). Finding that
 *              point is a search of its own over the terrain samples through the camera of the frame, so it stands
 *              here, beside the lines that name it. The view passes what it reads (the ground, the anchor, the
 *              frame, the eye, the drawn line, the first stream, the terrain, the pick radius); the function reads
 *              nothing else and writes only its outputs.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.39.00039 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_stream_view.hpp>
#include <ase/adp/godot/godot_host_terrain_view.hpp>
#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_ground.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The place a tap fills the can at in the brook `stream` drawn as `line`: true with the tapped pixel in `tap`
 *  (mockup units, a rect of no size) and " pick=x,z run=n/m half=h" in `detail`; false where the brook's ends do not
 *  stand on the ground (both outputs empty) or where its column lands no pixel in the bed (the column in `unplaced`).
 *  `eye` is the camera's global transform of this frame, `pick_px` the logical pixels a tap reaches a sample in. */
bool brook_place(const UiGround& ground, const UiAnchor& anchor, const ::godot::Transform2D& frame,
                 const ::godot::Transform3D& eye, const ::godot::PackedVector2Array& line,
                 const VivariumStreamView& stream, const std::vector<VivariumTerrainView>& terrain, float pick_px,
                 ::godot::Rect2& tap, std::string& detail, std::string& unplaced);

}  // namespace ase::adp::godot
