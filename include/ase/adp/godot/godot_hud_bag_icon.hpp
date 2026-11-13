#pragma once

/**
 * ASE GODOT ADAPTER - HUD BAG ICON
 *
 * @file        godot_hud_bag_icon.hpp
 * @brief       The leather pack closed at the hip: a disc with its drawing, tapped to open, a drop place
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, block BagIcon (the mockup's BAGI in hand(), the "Tasche"
 *              of layer 13 with HandSlot and BeltHolster; the opened pack is BagWindow on layer 14).
 *
 *              THE DISC. A circle of hud_bag_icon_radius at (hud_bag_icon_x, hud_bag_icon_y), filled
 *              OVERLAY_BAG_DISC at hud_bag_icon_fill_alpha, edged OVERLAY_INK_FAINT at hud_bag_icon_line; while the
 *              pack is open its edge is DOMAIN_GROUND, while a moved drag stands over the pack DOMAIN_LIFE, or
 *              DOMAIN_ALERT when the FitCheck refuses the thing, both at hud_bag_icon_open_line. On it the drawing of
 *              the pack (item art ranzen) fitted into hud_bag_icon_art, raised by hud_bag_icon_art_dy.
 *
 *              THE LOAD. From perception levels[1] on, texts.hud.load_hip with the weight of the pack's things in
 *              the mockup's f1 (hud_load_decimals) stands hud_bag_icon_load_dy below, size hud_bag_icon_load_size,
 *              class d, OVERLAY_INK_FAINT.
 *
 *              THE GESTURES. hit names GODOT_HUD_ACT_PACK_CLOSE within hud_bag_icon_hit_radius (the caller opens or
 *              closes BagWindow); over names GODOT_HUD_CONTAINER_PACK for a drag within hud_drop_bag_icon_radius.
 *
 *              BRIDGE GAP N0451. The pack's things and their weight are state of ase-inventory on flora.carry.v1.
 *              The icon draws disc, pack and load in full from BagIconFrame and never asks whether the port
 *              delivered: while it delivers nothing VivariumCarryBridge fills the frame with the mockup's things,
 *              and VivariumView logs the gap once on its edge (operator decision 2026-11-10).
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

#include <ase/adp/godot/godot_hud_bag_contents.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the closed bag shows this frame. */
struct BagIconFrame {
    uint32_t level = 0;                           // the depth of perception
    const std::vector<BagThing>* things = nullptr;// the things in the pack, nullptr while none
    bool open = false;                            // BagWindow::pack_open
    const char* hot = nullptr;                    // GODOT_HUD_CONTAINER_* a moved drag stands over
    bool refused = false;                         // the FitCheck refuses the dragged thing there
};

/** The leather pack closed at the hip. */
class BagIcon {
public:
    /** Binds measures, colours, the words and the drawing of the pack; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the disc, the pack and the load under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const BagIconFrame& now,
              std::string& failed);

    /** GODOT_HUD_ACT_PACK_CLOSE when `point` lies on the disc as last drawn, nullptr otherwise. */
    const char* hit(const ::godot::Vector2& point) const;

    /** The square round the disc a tap reaches hit() through (in the mockup's units), its centre the disc's, in
     *  `out`; false while the disc was not drawn. */
    bool place(::godot::Rect2& out) const;

    /** GODOT_HUD_CONTAINER_PACK within hud_drop_bag_icon_radius of a drag at `point`, nullptr otherwise. */
    const char* over(const ::godot::Vector2& point) const;

private:
    /** The weight as the mockup's f1. */
    std::string number(double value) const;

    ::godot::Vector2 at_;        // hud_bag_icon_x, hud_bag_icon_y
    double radius_ = 0.0;        // hud_bag_icon_radius
    double art_ = 0.0;           // hud_bag_icon_art
    double art_dy_ = 0.0;        // hud_bag_icon_art_dy
    double hit_radius_ = 0.0;    // hud_bag_icon_hit_radius
    double drop_radius_ = 0.0;   // hud_drop_bag_icon_radius
    double load_dy_ = 0.0;       // hud_bag_icon_load_dy
    double load_size_ = 0.0;     // hud_bag_icon_load_size
    double line_ = 0.0;          // hud_bag_icon_line
    double open_line_ = 0.0;     // hud_bag_icon_open_line
    double decimals_ = 0.0;      // hud_load_decimals
    uint32_t level_load_ = 0;    // hud.perception.levels[1]
    ::godot::Color disc_, faint_, ground_, life_, alert_;
    std::vector<HudRun> load_;  // texts.hud.load_hip
    UiRing ring_;                              // the disc and its edge
    UiTextBlock data_;                         // class d
    UiItemSprite pack_;                        // item art ranzen

    bool drawn_ = false;
};

}  // namespace ase::adp::godot
