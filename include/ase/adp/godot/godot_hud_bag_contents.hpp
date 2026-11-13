#pragma once

/**
 * ASE GODOT ADAPTER - HUD BAG CONTENTS
 *
 * @file        godot_hud_bag_contents.hpp
 * @brief       The things lying in one open container, drawn where they were let go, the last moved on top
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the contents of BagWindow (the mockup's itemsIn).
 *              The leather pack and the linen pouch draw their things through one BagContents each.
 *
 *              THE ORDER. Things are drawn by their order (o.zo) rising, so the last moved lies on top and is
 *              the first a tap finds. Each thing stands at its place from the content corner of the window,
 *              in a box of ContainerDef::drawn_box(length) - its true length at the container's scale, never
 *              below the readable minimum - and is drawn through UiItemSprite.
 *
 *              ONE THING, FOUR LAYERS, as the mockup lays them:
 *                selection  the chosen thing: a rect padded by hud_item_select_pad, rx hud_item_select_radius,
 *                           RENDER_WHITE at hud_item_select_fill_alpha, stroked in the container's ink at
 *                           hud_item_select_line, dashed hud_item_select_dash / hud_item_select_gap
 *                shadow     drop-shadow(0 hud_item_shadow_dy hud_item_shadow_blur OVERLAY_SHADOW at
 *                           hud_item_shadow_alpha): the sprite's silhouette, then the sprite itself
 *                count      a stack of more than one: texts.hud.stack_count with {n} at the box's lower right
 *                           (+hud_item_count_dx, +hud_item_count_dy), class d at hud_item_count_size, anchored
 *                           at its end, in the container's ink
 *                hit        the box padded by hud_item_hit_pad; the thing being dragged draws its place at
 *                           hud_item_dragged_alpha
 *
 *              THE PLACES ARE THE OUTPUT (the mockup's POS). Every drawn thing leaves its box and its scale k
 *              (the box over the drawing's longer side), so a drag starts at the grip it was taken by and the
 *              FitCheck scales the ghost to the target container.
 *
 *              The things themselves are state of ase-inventory on flora.carry.v1 (register N0451): the caller
 *              hands over the frame's things and this unit draws them in full; while the port delivers nothing
 *              VivariumCarryBridge fills the frame with the mockup's things and VivariumView logs the gap once.
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
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_container_def.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

/** One thing in a container as flora.carry.v1 delivers it. */
struct BagThing {
    std::string key;          // o.key: the item it is
    std::string art;          // the item_art.json name of its drawing
    ::godot::Vector2 at;      // o.x, o.y: where it was let go, from the content corner
    double length_cm = 0.0;   // D.len: its longest piece
    double kg = 0.0;          // wt(o): its weight with what it holds
    double litres = 0.0;      // vol(o): its room, a stack's piece times the count
    int64_t order = 0;        // o.zo: the last moved has the highest
    int64_t count = 0;        // o.n: the pieces of a stack
    bool stack = false;       // D.stack: the thing comes in pieces
    double level = 0.0;       // the held share 0..1 of a vessel
};

/** Where one thing was drawn this frame (the mockup's POS entry). */
struct BagThingPlace {
    std::string key;          // the item
    ::godot::Rect2 box;       // x, y, w, h in the mockup's units
    double k = 0.0;           // the scale of its drawing
};

/** How the contents are drawn this frame. */
struct BagContentsView {
    ::godot::Vector2 corner;  // ox, oy: the content corner of the window
    ::godot::Color ink;       // the ink of the container (LEATHER_BAG_TITLE, LINEN_POUCH_INK)
    std::string selected;     // S.sel: the chosen item, empty when none
    std::string dragged;      // the item a moved drag holds, empty when none
};

/** The things lying in one open container. */
class BagContents {
public:
    /** Binds the tokens, colours and the count word; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /**
     * Draws `things` of `container` under `frame` and writes their boxes into `places`; false with the
     * reason in `failed` when a drawing or a word cannot be read.
     */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const ContainerDef& container,
              const std::vector<BagThing>& things, const BagContentsView& view, std::vector<BagThingPlace>& places,
              std::string& failed);

    /** The item whose padded box holds `point`, the topmost first; empty when none. */
    std::string hit(const std::vector<BagThingPlace>& places, const ::godot::Vector2& point) const;

private:
    /** The sprite of the drawing `art`, read once; nullptr with the reason in `failed`. */
    const UiItemSprite* sprite(const std::string& art, std::string& failed);

    const UiTheme* theme_ = nullptr;
    double select_pad_ = 0.0;          // hud_item_select_pad
    double select_radius_ = 0.0;       // hud_item_select_radius
    double select_line_ = 0.0;         // hud_item_select_line
    double select_dash_ = 0.0;         // hud_item_select_dash
    double select_gap_ = 0.0;          // hud_item_select_gap
    double shadow_dy_ = 0.0;           // hud_item_shadow_dy
    double shadow_blur_ = 0.0;         // hud_item_shadow_blur
    double count_dx_ = 0.0;            // hud_item_count_dx
    double count_dy_ = 0.0;            // hud_item_count_dy
    double count_size_ = 0.0;          // hud_item_count_size
    double hit_pad_ = 0.0;             // hud_item_hit_pad
    double dragged_alpha_ = 0.0;       // hud_item_dragged_alpha
    ::godot::Color select_fill_;       // RENDER_WHITE at hud_item_select_fill_alpha
    ::godot::Color shadow_;            // OVERLAY_SHADOW at hud_item_shadow_alpha
    std::vector<HudRun> count_word_;   // texts.hud.stack_count
    UiTextBlock count_;                // class d
    std::vector<std::pair<std::string, UiItemSprite>> sprites_;  // one per drawing, read on first use
};

}  // namespace ase::adp::godot
