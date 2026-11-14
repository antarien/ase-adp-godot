#pragma once

/**
 * ASE GODOT ADAPTER - HUD WORLD DROP
 *
 * @file        godot_hud_world_drop.hpp
 * @brief       The things lying on the ground: drawn at their world place, darkened with the hour, hit to pick up
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, block WorldDrop (the mockup's drawWorld for the things
 *              on the ground, and groundHits).
 *
 *              THE DRAWING. Every thing flora.carry.v1 reports on the ground stands at its world place over
 *              UiAnchor::place, the farthest first and among equals the last moved on top, turned by the angle
 *              the module gives, its drawing's foot in the middle of the place and its length in metres measured
 *              at that depth: one metre is the frame distance between the place and the place one metre along the
 *              world x axis, the axis the care view's camera looks across. Under every thing not at the tool
 *              place lies a flat shadow WORLD_THING_SHADOW (hud_world_thing_shadow_*). The whole layer darkens with
 *              the light of the hour, hud_world_light_floor plus hud_world_light_span times the light share: the
 *              pen's tint carries that brightness over the world group only (UiPen::set_tint), never the canvas.
 *
 *              THE HITS. A thing not at the tool place is hit inside its length (at least hud_ground_item_min)
 *              padded by hud_ground_item_pad, hud_ground_item_depth of the length deep and hud_ground_item_pad_low
 *              lower; from hud.perception.levels[1] on, and when it is not chosen, a dot of its domain tone
 *              (hud_ground_dot_*) sits under it. The tool place is hit over hud_ground_rack_hit_* metres at its
 *              place. A drop lands on the ground only above hud_drop_ground_max_y (DragLayer reads ground_line).
 *
 *              FULL FROM THE FRAME. The things on the ground, their places and the tool place are state of
 *              ase-inventory on flora.carry.v1. WorldDrop draws whatever the frame carries and never asks whether
 *              the port delivered: until it does, the bridge VivariumCarryBridge (godot_vivarium_hud_carry_frame.cpp)
 *              writes the mockup's initInv from SHA into the frame and VivariumView marks the gap N0451 in its
 *              own source (open_gap, godot_vivarium_hud.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-14
 * @version     00.00.01.00004
 */

#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

/** One thing lying on the ground as flora.carry.v1 reports it. */
struct GroundThing {
    std::string key;            // o.key: the item it is
    std::string art;            // the item_art.json name of its drawing
    std::string tone;           // D.dom: the domain tone of the item (water, life, ...)
    ::godot::Vector3 place;     // o.gu, o.gz on the ground: its world place
    double angle = 0.0;         // o.ang: the turn it lies at, radians
    double length_cm = 0.0;     // D.len: its longest piece
    int64_t order = 0;          // o.zo: the last moved has the highest
    bool at_rack = false;       // o.rack: it hangs at the tool place
    bool bodied = false;        // a body of its file stands for it in the 3D scene (lay_tools): the ground keeps its shadow and hit box, not its drawing
};

/** What the ground shows this frame. */
struct WorldDropFrame {
    const UiAnchor* anchor = nullptr;                // the projection of this frame
    uint32_t level = 0;                              // the depth of perception
    double light = 0.0;                              // the light share of the hour the sky port carries, 0..1
    const std::vector<GroundThing>* things = nullptr;// the things on the ground, nullptr while none
    ::godot::Vector3 rack;                           // the tool place
    ::godot::Vector3 patch;                          // the place of the patch, the carry bridge lays the ground by
    std::string selected;                            // S.sel: the chosen item, empty when none
};

/** Where one thing on the ground was drawn this frame (the mockup's POS entry). */
struct GroundThingPlace {
    std::string key;           // the item
    ::godot::Rect2 box;        // the drawing's box in frame units
    ::godot::Rect2 hit;        // the padded hit box
    double k = 0.0;            // the scale of its drawing
};

/** The things lying on the ground. */
class WorldDrop {
public:
    /** Binds measures, colours, the word and the levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the things of `now` under `frame` and keeps their hit boxes; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const WorldDropFrame& now,
              std::string& failed);

    /** The key of the topmost thing whose hit box holds `point`, empty when none. */
    std::string hit(const ::godot::Vector2& point) const;

    /** True when `point` lies on the tool place's hit box as last drawn (data-act rack). */
    bool hit_rack(const ::godot::Vector2& point) const;

    /** The tool place's hit box as last drawn in `box`; false while it is not drawn (the place line a test taps). */
    bool rack_place(::godot::Rect2& box) const {
        box = rack_hit_;
        return rack_drawn_;
    }

    /** hud_drop_ground_max_y: a drop above this line lands in the world. */
    double ground_line() const { return ground_max_y_; }

    /** The boxes of the things as last drawn, the topmost last. */
    const std::vector<GroundThingPlace>& places() const { return places_; }

private:
    /** The sprite of the drawing `art`, read once; nullptr with the reason in `failed`. */
    const UiItemSprite* sprite(const std::string& art, std::string& failed);

    /**
     * The frame point of `place` and the frame length of one metre of the mockup's world there: the scene's
     * metre at that place times hud_world_metre_scale. False when the place lies behind the camera.
     */
    bool metre(const UiAnchor& anchor, const ::godot::Vector3& place, ::godot::Vector2& at, double& length) const;

    const UiTheme* theme_ = nullptr;
    double metre_scale_ = 0.0;        // hud_world_metre_scale
    double light_floor_ = 0.0;        // hud_world_light_floor
    double light_span_ = 0.0;         // hud_world_light_span
    double shadow_dx_ = 0.0;          // hud_world_thing_shadow_dx
    double shadow_dy_ = 0.0;          // hud_world_thing_shadow_dy
    double shadow_share_ = 0.0;       // hud_world_thing_shadow_share
    double shadow_pad_ = 0.0;         // hud_world_thing_shadow_pad
    double shadow_ry_ = 0.0;          // hud_world_thing_shadow_ry
    double item_min_ = 0.0;           // hud_ground_item_min
    double item_depth_ = 0.0;         // hud_ground_item_depth
    double item_pad_ = 0.0;           // hud_ground_item_pad
    double item_pad_low_ = 0.0;       // hud_ground_item_pad_low
    double dot_radius_ = 0.0;         // hud_ground_dot_radius
    double dot_dy_ = 0.0;             // hud_ground_dot_dy
    double dot_alpha_ = 0.0;          // hud_ground_dot_alpha
    double rack_half_w_ = 0.0;        // hud_ground_rack_hit_half_w
    double rack_rise_ = 0.0;          // hud_ground_rack_hit_rise
    double rack_w_ = 0.0;             // hud_ground_rack_hit_w
    double rack_h_ = 0.0;             // hud_ground_rack_hit_h
    double ground_max_y_ = 0.0;       // hud_drop_ground_max_y
    uint32_t level_dot_ = 0;          // hud.perception.levels[1]
    ::godot::Color shadow_;
    std::vector<std::pair<std::string, UiItemSprite>> sprites_;  // one per drawing, read on first use
    std::vector<GroundThingPlace> places_;  // the things as last drawn
    ::godot::Rect2 rack_hit_;               // the tool place as last drawn

    bool rack_drawn_ = false;
};

}  // namespace ase::adp::godot
