#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI HEX CHIP
 *
 * @file        godot_hud_ui_hex_chip.hpp
 * @brief       The hexagonal chip of the HUD: a tone, a symbol or a thing, a label per perception level
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiHexChip. The mockup draws two forms:
 *
 *                HX   the glyph chip - a hexagon of the radius r with a corner at 0 degrees, filled with the
 *                     tone at hud_chip_on_fill_alpha when it is on and with OVERLAY_CHIP_OFF when it is not,
 *                     stroked in the tone at hud_line_chip_on or hud_line_chip; the ICON_* centred at the
 *                     size r * hud_chip_glyph_scale, or r * hud_chip_glyph_label_scale lifted by
 *                     hud_chip_glyph_label_lift when a label stands under it at r * hud_chip_label_drop in
 *                     class t, hud_chip_label_size, in the tone
 *                HXI  the thing chip - the same hexagon with hud_chip_thing_on_fill_alpha and
 *                     OVERLAY_ITEM_CHIP_OFF; the thing's picture fitted into r * hud_chip_thing_scale, or
 *                     r * hud_chip_thing_label_scale lifted by hud_chip_thing_label_lift above a label at
 *                     r * hud_chip_thing_label_drop, hud_chip_thing_label_size
 *
 *              The whole chip fades in (.fade, hud_anim_fade) after its delay; its hexagon is the tap target
 *              (data-act), which hit answers.
 *
 *              THE PICTURE IS NOT THIS UNIT'S. A thing is drawn by UiItemSprite from its preview, from the
 *              same source in the bag, the hand, the belt and the world (plan, Inventar). The thing chip draws
 *              its body and its label and hands the picture's box through picture(); the caller draws the
 *              thing there.
 *
 *              The label belongs to the perception level: the group passes it where the mockup shows one
 *              (S.perc === 1) and an empty one where it does not.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>

namespace ase::adp::godot {

/** hexPts(x, y, r): the six corners, the first at 0 degrees, a sixth of a turn apart; every hexagon of the HUD. */
::godot::PackedVector2Array hud_hex_points(const ::godot::Vector2& centre, double radius);

/** True when `point` lies inside the hexagon of hud_hex_points(centre, radius), its edges included. */
bool hud_hex_contains(const ::godot::Vector2& centre, double radius, const ::godot::Vector2& point);

/** Where and in which state one chip is drawn this frame. */
struct UiHexChipPlace {
    ::godot::Vector2 centre;   // x, y: the centre of the hexagon, in the mockup's units
    double radius = 0.0;       // r: the distance from the centre to a corner
    ::godot::Color tone;       // col: the domain tone of the chip
    bool on = false;           // o.on: the chip is active
    bool fade = false;         // o.fade: the chip fades in with hud_anim_fade
    double delay = 0.0;        // o.d: the seconds before it fades in
    double elapsed = 0.0;      // the seconds since the chip appeared
    bool still = false;        // .still and reduced motion: every animation at its end
};

/** The box a thing chip leaves for its picture: artFit(id, it, x, y, b). */
struct UiHexChipPicture {
    ::godot::Vector2 centre;   // the centre the picture is fitted on
    double box = 0.0;          // b: the longer side the picture is fitted into
    double opacity = 0.0;      // the chip's opacity this frame, which the picture shares
};

/** One hexagonal chip: a glyph chip (HX) or a thing chip (HXI). */
class UiHexChip {
public:
    /** Binds the tokens and colours of both forms; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Makes this a glyph chip showing `icon` (an ICON_* name) over `label` (empty: no label). */
    bool set_glyph(const std::string& icon, const std::string& label, std::string& failed);

    /** Makes this a thing chip over `label` (empty: no label); the picture is drawn by the caller. */
    bool set_thing(const std::string& label, std::string& failed);

    /** Draws the hexagon, the glyph of a glyph chip and the label under `frame`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiHexChipPlace& place);

    /** The box the picture of a thing chip is fitted into at `place`. */
    UiHexChipPicture picture(const UiHexChipPlace& place) const;

    /** Whether `point` (in the mockup's units) lies inside the hexagon at `place`: the chip's tap target. */
    bool hit(const UiHexChipPlace& place, const ::godot::Vector2& point) const;

private:
    double shown(const UiHexChipPlace& place) const;

    const UiTheme* theme_ = nullptr;     // the theme the icon and the label bind against
    bool thing_ = false;                 // the chip is a thing chip (HXI)
    bool labelled_ = false;              // a label stands under the content
    UiIcon icon_;                        // the glyph of a glyph chip
    UiTextBlock label_;                  // class t
    UiMotion fade_;                      // hud_anim_fade
    ::godot::Color off_;                 // OVERLAY_CHIP_OFF
    ::godot::Color thing_off_;           // OVERLAY_ITEM_CHIP_OFF
    double line_ = 0.0;                  // hud_line_chip
    double line_on_ = 0.0;               // hud_line_chip_on
    double on_alpha_ = 0.0;              // hud_chip_on_fill_alpha
    double glyph_scale_ = 0.0;           // hud_chip_glyph_scale
    double glyph_label_scale_ = 0.0;     // hud_chip_glyph_label_scale
    double glyph_label_lift_ = 0.0;      // hud_chip_glyph_label_lift
    double label_drop_ = 0.0;            // hud_chip_label_drop
    double label_size_ = 0.0;            // hud_chip_label_size
    double thing_on_alpha_ = 0.0;        // hud_chip_thing_on_fill_alpha
    double thing_scale_ = 0.0;           // hud_chip_thing_scale
    double thing_label_scale_ = 0.0;     // hud_chip_thing_label_scale
    double thing_label_lift_ = 0.0;      // hud_chip_thing_label_lift
    double thing_label_drop_ = 0.0;      // hud_chip_thing_label_drop
    double thing_label_size_ = 0.0;      // hud_chip_thing_label_size
};

}  // namespace ase::adp::godot
