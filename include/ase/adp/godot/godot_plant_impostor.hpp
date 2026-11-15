#pragma once

/**
 * ASE GODOT ADAPTER - IMPOSTOR OF A FAR PLANT
 *
 * @file        godot_plant_impostor.hpp
 * @brief       From which distance a body is drawn as an impostor card, and how large that card and its texture are
 * @description Register B0295 point 4: far bodies become an impostor. The distance follows the same rule by which the
 *              bark shader closes a ring onto its axis (plant_bark.gdshader): a ring thinner than one pixel on the
 *              screen is not drawn. Beyond the distance at which the widest ring of the tube is thinner than one pixel,
 *              no ring of the body stands any more, so the body is drawn from there on as one card facing the camera.
 *              For a vertical field of view fov and a render height of H pixels a ring of radius r spans
 *              2 r H / (2 d tan(fov / 2)) pixels at the distance d, so the widest ring w reaches one pixel at
 *              d = w H / tan(fov / 2). A length L spans L / (2 w) pixels there, so the card texture is as many
 *              pixels high and wide as the body spans at the switch: the card shows the body at the size it has on
 *              the screen where the card takes over, and nothing is chosen by hand.
 *
 *              The extent of the body is the box around the vertices of its tube and the origins of its instance
 *              records (leaves, catkins, fruit stands), plant-local; the card looks along -z at it, so its width is
 *              the extent in x and its height the extent in y.
 *
 *              NO GODOT TYPE: plain numbers in, plain numbers out, so the host tests check it without a Godot
 *              process. The Godot side is fill_impostor (godot_plant_scene_impostor.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_tubes.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

constexpr const char* GODOT_PLANT_IMPOSTOR = "PlantImpostor";            // the card that stands for a far plant (B0295 point 4)
constexpr const char* GODOT_PLANT_IMPOSTOR_VIEW = "PlantImpostorView";  // the viewport that draws the card texture of a plant once

/**
 * The impostor of one body. A reach of 0 means the body has no ring to measure: it gets no card and its nodes no
 * end of their visibility range.
 */
struct PlantImpostorFrame {
    float reach = 0.0f;           // metres from which the widest ring is thinner than one pixel, and the card takes over
    float width = 0.0f;           // the extent of the body in x, metres
    float height = 0.0f;          // the extent of the body in y, metres
    float depth = 0.0f;           // the extent of the body in z, metres
    float centre_x = 0.0f;        // the centre of the box around the body, plant-local
    float centre_y = 0.0f;
    float centre_z = 0.0f;
    uint32_t texture_width = 0;   // pixels the body spans in x at the reach
    uint32_t texture_height = 0;  // pixels the body spans in y at the reach
};

/**
 * The impostor of the body of `tube` and its instance records `rows` (each a buffer of twelve values per record as
 * MultiMesh takes it, the origin at 3, 7 and 11), for a render height of `view_height` pixels and the tangent of
 * half the vertical field of view `tan_half_fov`. The widest ring is the largest ring radius of custom1. No ring,
 * no height in pixels or no field of view gives the frame with reach 0.
 */
PlantImpostorFrame impostor_of(const PlantBodyTube& tube, const std::vector<const std::vector<float>*>& rows,
                               float view_height, float tan_half_fov);

}  // namespace ase::adp::godot
