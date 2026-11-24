#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI ITEM SPRITE
 *
 * @file        godot_hud_ui_item_sprite.hpp
 * @brief       The drawing of one carried thing, fitted into a box as the mockup's artFit and artSvg do
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiItemSprite. Every place the mockup
 *              draws a thing - the hex chip of the hand, the belt holster, the bag, the tool rack, the drag
 *              ghost - draws it through this unit.
 *
 *              THE DRAWING COMES FROM SHA. item_art.json (sha-web-styles/src/item-art.json through
 *              generate-item-art.cjs) carries, per thing, its view box vw x vh, its paths in drawing order
 *              (SVG path data d, a stroke and a fill as colour names of colors.json, the width w) and the
 *              level of a vessel. UiTheme::item_art hands the entry over; set reads it once into outlines
 *              and refuses a colour, a path or a level it cannot draw, naming it in `failed`.
 *
 *              artFit, the fit into a box b centred at (x, y):
 *                long     tall when vh / vw > hud_item_long_ratio, wide when vw / vh > hud_item_long_ratio
 *                turn     a tall thing turns by +hud_item_long_turn degrees, a wide one by -hud_item_long_turn,
 *                         about the middle of its drawn box
 *                scale    k = b * (turned ? hud_item_long_fill : 1) / max(vw, vh)
 *              A container lays a thing unturned instead: draw_scaled puts the view box's corner at (x, y) and
 *              scales it by k (artSvg alone, the mockup's itemsIn).
 *              artSvg, the paths: fill first, then the stroke at w * hud_item_stroke_scale in the units of the
 *              frame whatever k is (vector-effect non-scaling-stroke), with round caps and joins.
 *
 *              THE LEVEL IS THE CALLER'S. A vessel's water or sap is its held amount over its capacity,
 *              state of the item the inventory carries; the caller passes the share 0..1 and the sprite
 *              draws it after the paths:
 *                line     the water line from (left, y) over (middle, y + sag) to (right, y),
 *                         y = bottom - share * span, stroked in its colour at w
 *                body     the sap between the walls from y = bottom - share * span down to the foot,
 *                         the wall x moving from top_left/top_right to foot_left/foot_right, filled
 *              A share of 0 draws no level.
 *
 *              THE SHADOW IS THE SAME DRAWING, BLURRED. The mockup lays drop-shadow(0 dy blur colour) under every
 *              thing in a container and under the drag ghost. The caller draws the thing once with `silhouette`
 *              set, moved by its dy, then once as itself on top. The silhouette is UiItemShadow
 *              (godot_hud_ui_item_shadow.hpp): the whole drawing as one mask, blurred by the Gaussian of the
 *              CSS radius `spread`, coloured `shade` at `opacity`, rastered once per transform and pixel scale
 *              and drawn as one quad whose pixels are the frame's. The path data is read through
 *              read_svg_path (godot_hud_ui_svg_path.hpp), the one flattening of every HUD curve.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_item_shadow.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** One path of a drawing, read from item_art.json; its outlines in view-box units. */
struct UiItemSpritePath {
    std::vector<UiSvgOutline> outlines;
    ::godot::Color stroke;
    ::godot::Color fill;
    bool stroked = false;   // the path names a stroke
    bool filled = false;    // the path names a fill
    double width = 0.0;     // w, in view-box units of the mockup's stroke before hud_item_stroke_scale
};

/** The level of a vessel, read from item_art.json (kind line or body). */
struct UiItemSpriteLevel {
    bool line = false;      // kind line: the water line
    bool body = false;      // kind body: the filled sap
    double bottom = 0.0;
    double span = 0.0;
    double left = 0.0;      // line
    double right = 0.0;     // line
    double sag = 0.0;       // line
    double width = 0.0;     // line: w
    double top = 0.0;       // body
    double top_left = 0.0;  // body
    double top_right = 0.0; // body
    double foot_left = 0.0; // body
    double foot_right = 0.0;// body
    ::godot::Color color;   // line: stroke, body: fill
};

/** Where and how one thing is drawn this frame. */
struct UiItemSpritePlace {
    ::godot::Vector2 centre;  // the middle of the box, in frame units
    double box = 0.0;         // b: the side of the box the thing is fitted into
    double level = 0.0;       // the held share 0..1 of a vessel; 0 draws no level
    double opacity = 1.0;     // the dragged place draws faded (hud_item_dragged_alpha)
    bool silhouette = false;  // drop-shadow: the drawing's mask blurred by `spread`, in `shade` at `opacity`
    ::godot::Color shade;     // the colour of the silhouette (OVERLAY_SHADOW)
    double spread = 0.0;      // the CSS blur radius of drop-shadow (hud_item_shadow_blur), in frame units
};

/** The drawing of one carried thing. */
class UiItemSprite {
public:
    /** Binds the fit and stroke tokens; false with the missing one in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Reads the drawing of the thing `name` of item_art.json; false with the reason in `failed`. */
    bool set(const std::string& name, std::string& failed);

    /** artFit: draws the thing under `frame` fitted into the box of `place`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place) const;

    /**
     * artSvg: draws the thing under `frame` with its view box's corner at `corner`, scaled by `k`, unturned,
     * as a container lays it; `place` gives level, opacity and silhouette, its centre and box are not read.
     */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place) const;

    double view_width() const { return vw_; }
    double view_height() const { return vh_; }

private:
    /** Draws the paths and the level moved by `local` from view-box units into frame units. */
    void draw_local(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Transform2D& local,
                    const UiItemSpritePlace& place) const;

    /**
     * Fills and strokes one run of view-box points moved by `local` into frame units. The stroke is ui_stroke's one
     * round-joined mesh under `frame` (P0692): stroke-linejoin and stroke-linecap round as the browser lays them,
     * never a polyline with a disc drawn on every corner.
     */
    void paint(UiPen& canvas, const ::godot::Transform2D& frame, const UiSvgOutline& outline,
               const ::godot::Transform2D& local, const ::godot::Color* fill, const ::godot::Color* stroke,
               double width, const UiItemSpritePlace& place) const;

    const UiTheme* theme_ = nullptr;
    double stroke_scale_ = 0.0;   // hud_item_stroke_scale
    double long_ratio_ = 0.0;     // hud_item_long_ratio
    double long_turn_ = 0.0;      // hud_item_long_turn
    double long_fill_ = 0.0;      // hud_item_long_fill
    double vw_ = 0.0;
    double vh_ = 0.0;
    std::vector<UiItemSpritePath> paths_;
    UiItemSpriteLevel level_;
    bool has_level_ = false;
    mutable UiItemShadow shadows_;  // the blurred silhouettes rastered so far (godot_hud_ui_item_shadow.hpp)
    mutable UiSvgOutline level_outline_;  // the level's outline at level_share_, laid again only when the level moves
    mutable double level_share_ = 0.0;    // the share level_outline_ was laid for, 0 before the first (P0692)
};

}  // namespace ase::adp::godot
