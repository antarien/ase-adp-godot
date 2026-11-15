#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI ITEM SHADOW
 *
 * @file        godot_hud_ui_item_shadow.hpp
 * @brief       The drop-shadow of a carried thing's drawing: its mask, blurred as CSS blurs, kept as a texture
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiItemShadow, the silhouette UiItemSprite
 *              draws under a thing when `silhouette` is set.
 *
 *              THE MOCKUP'S SHADOW IS A FILTER. drop-shadow(0 dy blur colour) lies under every thing in a
 *              container (blur 0.8, hud_item_shadow_blur, m.html 756) and under the drag ghost (blur 3,
 *              hud_ghost_shadow_blur, m.html 788). CSS takes the alpha of the whole drawing as ONE mask,
 *              blurs it by a Gaussian whose standard deviation is half the blur radius, and colours it.
 *              This unit does the same:
 *                mask     every path's fill by the nonzero rule (each run closed from its last point to its
 *                         first) and its stroke at w * hud_item_stroke_scale in frame units with round caps
 *                         and joins, rastered at GODOT_ITEM_SHADOW_SAMPLES samples per pixel along each axis;
 *                         overlapping paths are counted once
 *                blur     a Gaussian of sigma = blur * GODOT_ITEM_SHADOW_SIGMA_PER_BLUR pixels of the frame,
 *                         along the rows and then the columns, cut and margined at GODOT_ITEM_SHADOW_SIGMA_REACH
 *                         sigmas
 *                colour   every pixel the shade's colour, its alpha the shade's alpha times the opacity times
 *                         the blurred coverage
 *              A vessel's level lies inside its walls and adds nothing to the mask.
 *
 *              KEPT, NOT RASTERED PER FRAME. A shadow is rastered once per transform (the axes of the
 *              view box in frame units), pixel scale, blur, shade and opacity, and kept; the oldest of more
 *              than GODOT_ITEM_SHADOW_KEPT goes first. A container draws its things at a few scales, the ghost
 *              at the scale of the container under the finger, so a handful are kept per thing.
 *
 *              Before 2026-11-12 the silhouette was every fill and stroke drawn in the shade, the strokes
 *              widened by twice the blur: hundreds of round-join discs at alpha 0.6 stacked into a hard black
 *              block under the ghost (run 20261112-161703-act-body, ground_act_body_dragged.png).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>

#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

constexpr double GODOT_ITEM_SHADOW_SIGMA_PER_BLUR = 0.5;  // CSS blur radius over the Gaussian's standard deviation: sigma = blur / 2
constexpr double GODOT_ITEM_SHADOW_SIGMA_REACH = 3.0;     // sigmas the blur kernel and the mask's margin reach out; past 3 sigma a Gaussian holds 0.3 %
constexpr int64_t GODOT_ITEM_SHADOW_SAMPLES = 2;          // samples of the mask per pixel along each axis, the edge's coverage in quarters
constexpr int64_t GODOT_ITEM_SHADOW_KEPT = 8;             // shadow textures a thing keeps; the oldest goes first
constexpr double GODOT_ITEM_SHADOW_SAME = 1.0e-4;         // largest difference of two keys' numbers that still names the same shadow
constexpr int64_t GODOT_ITEM_SHADOW_PIXEL_MAX = 1024;     // longest side of a shadow texture in pixels; a larger mask is not rastered
constexpr double GODOT_ITEM_SHADOW_BYTE = 255.0;          // the largest value of a texture's 8-bit channel (Image::FORMAT_RGBA8)
constexpr int64_t GODOT_ITEM_SHADOW_CHANNELS = 4;         // bytes of one RGBA8 pixel

/** One path of a drawing as UiItemSprite reads it from item_art.json (godot_hud_ui_item_sprite.hpp). */
struct UiItemSpritePath;

/** One blurred silhouette, and the key it was rastered for. */
struct UiItemSpriteShadow {
    ::godot::Vector2 axis_x;         // the first column of the local transform, view-box units into frame units
    ::godot::Vector2 axis_y;         // its second column
    double scale = 0.0;              // pixels per frame unit
    double spread = 0.0;             // the blur radius, frame units
    ::godot::Color shade;            // the colour, its alpha already times the opacity
    ::godot::Vector2 corner;         // the mask's top left from the transform's origin, frame units
    ::godot::Vector2 size;           // the mask's size, frame units
    int64_t width = 0;               // the texture's pixels across
    int64_t height = 0;              // the texture's pixels down
    ::godot::Ref<::godot::ImageTexture> texture;  // null when the mask came out empty or too large
};

/** The drop-shadows of one drawing, rastered on first use and kept. */
class UiItemShadow {
public:
    /**
     * The shadow of `paths` (strokes at w * `stroke_scale` frame units) under the axes of `local`, at `scale`
     * pixels per frame unit, blurred by the CSS radius `spread` and coloured `shade`; rastered now when not kept.
     */
    const UiItemSpriteShadow& of(const std::vector<UiItemSpritePath>& paths, double stroke_scale,
                                 const ::godot::Transform2D& local, double scale, double spread,
                                 const ::godot::Color& shade);
    /** Forgets every kept shadow: the drawing they were rastered from changed. */
    void clear();

private:
    /** Rasters the mask of `paths` under `cast`'s axes, blurs it and makes `cast`'s texture. */
    void raster(const std::vector<UiItemSpritePath>& paths, double stroke_scale, UiItemSpriteShadow& cast) const;

    std::vector<UiItemSpriteShadow> kept_;  // oldest first
};

}  // namespace ase::adp::godot
