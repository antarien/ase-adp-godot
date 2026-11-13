#pragma once

/**
 * ASE GODOT ADAPTER - HUD DRAG LAYER
 *
 * @file        godot_hud_drag_layer.hpp
 * @brief       A carried thing under the finger: the ghost, the place it stands over, the verdict and the release
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, block DragLayer (the mockup's S.drag, ghost,
 *              dropTarget, freeSpot, dropItem and the pointer handlers).
 *
 *              THE TAKE. A touch on a thing (data-act item) takes it with the grip the finger holds on its
 *              drawn box and the scale it was drawn at. Nothing moves until the finger has gone further than
 *              hud_gesture_drag_slop: then the drag counts as moved, the selection falls (S.sel = null), and
 *              from then on every move aims.
 *
 *              THE PLACES, in the mockup's order. The linen pouch and the leather pack through BagWindow::over,
 *              with the spot the thing would lie at (the finger less the grip scaled to the container, less the
 *              content corner); the hand within hud_drop_hand_radius of hud_hand_x / hud_hand_y; a holster
 *              through BeltHolster::over; the bag icon through BagIcon::over (the pack, the spot found free at the
 *              release); above WorldDrop::ground_line and only where the finger meets the ground, the tool place
 *              through ToolRack::over, else the ground at the point the view projected. Every place carries its
 *              word where.* of texts.hud.
 *
 *              THE GHOST. While moved, the thing is drawn under the finger at hud_ghost_alpha, its silhouette
 *              in OVERLAY_SHADOW_LIFTED at hud_ghost_shadow_alpha hud_ghost_shadow_dy lower, widened by
 *              hud_ghost_shadow_blur. Its scale is the drawn box of the container it stands over, else of the
 *              container it came from, else of the pack, at most hud_ghost_max_box, over the drawing's longer
 *              side; its corner is the finger less the grip scaled from the taken scale to that one. Over a
 *              place, FitCheck draws the verdict above the ghost.
 *
 *              THE VERDICT. Whether a thing goes to a place is the module's word on flora.stow.v1: the caller
 *              asks for target() and hands the answer back as StowAnswer. An answer counts only for the thing
 *              and the place it names; a place entered this frame carries no verdict until the next one, so
 *              FitCheck draws nothing there and the drop is held. judge() takes the answer every frame, not only
 *              on a move: the mockup's ghost asks fits() on every render, also under a finger held still.
 *
 *              THE RELEASE. A drag that never moved is a tap on the thing (GODOT_HUD_DRAG_TAP). An allowed
 *              place sends the move (GODOT_HUD_DRAG_SEND) and toasts texts.hud.drop_at with the item's name and
 *              the place word in DOMAIN_GROUND, except at the hand, which equips without a toast (dropItem returns
 *              through equip()); at the bag icon the spot is the first free cell of a raster
 *              from hud_drop_spot_inset in steps of hud_drop_spot_step over the pack's things, else a random
 *              spot inside the inset, as the mockup lays it. A refused place toasts the module's word fit_<c> in
 *              DOMAIN_ALERT (GODOT_HUD_DRAG_REFUSED). No place or no verdict keeps the thing where it was
 *              (GODOT_HUD_DRAG_HELD).
 *
 *              FULL FROM THE FRAME. The verdict and the move are flora.stow.v1, the things and their places
 *              flora.carry.v1. DragLayer never asks whether a port delivered: until they do, the bridge judges by
 *              the mockup's fits() (VivariumView::bridge_stow, VivariumCarryBridge::answer) and lays the things
 *              (VivariumCarryBridge::fill), and VivariumView marks the gaps N0452 and N0451 in its own source.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0452 (flora.stow.v1), N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.01.00002
 */

#include <ase/adp/godot/godot_hud_bag_contents.hpp>
#include <ase/adp/godot/godot_hud_bag_icon.hpp>
#include <ase/adp/godot/godot_hud_bag_window.hpp>
#include <ase/adp/godot/godot_hud_belt_holster.hpp>
#include <ase/adp/godot/godot_hud_container_def.hpp>
#include <ase/adp/godot/godot_hud_fit_check.hpp>
#include <ase/adp/godot/godot_hud_toast.hpp>
#include <ase/adp/godot/godot_hud_tool_rack.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/godot_hud_world_drop.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

/** The thing a touch took. */
struct DragThing {
    std::string key;             // o.key: the carried thing
    std::string id;              // o.id: the item it is, the key of its words items.<id>.*
    std::string art;             // the item_art.json name of its drawing
    const char* from = nullptr;  // o.c: GODOT_HUD_CONTAINER_*, GODOT_HUD_PLACE_HAND or GODOT_HUD_PLACE_GROUND
    double length_cm = 0.0;      // D.len: its longest piece
    double level = 0.0;          // the held share 0..1 of a vessel: the ghost draws D.art(o) with it
};

/** The module's word on flora.stow.v1 for one thing at one place. */
struct StowAnswer {
    std::string key;                    // the thing it judged
    std::string place;                  // the place it judged (GODOT_HUD_CONTAINER_*, GODOT_HUD_PLACE_*)
    bool rack = false;                  // the ground place is the tool place
    std::string code;                   // the refusal code, empty when the thing goes there
    std::vector<HudPortValue> values;   // the values it sends with the code (cm, l, cap_l, kg)
};

/** The place a drag stands over (the mockup's tg). */
struct DragTarget {
    const char* place = nullptr;  // GODOT_HUD_CONTAINER_*, GODOT_HUD_PLACE_*; nullptr over no place
    const char* where = nullptr;  // GODOT_HUD_WHERE_*: its word
    ::godot::Vector2 spot;        // pack and pouch: where the thing would lie, from the content corner
    bool automatic = false;       // the bag icon: the spot is found free at the release
    bool rack = false;            // the ground place is the tool place
    ::godot::Vector3 ground;      // the ground: the point under the finger
};

/** What the layer reads this frame. */
struct DragLayerFrame {
    const BagWindow* window = nullptr;                   // REQUIRED: the windows and both containers
    const std::vector<BagThing>* pack_things = nullptr;  // flora.carry.v1: the pack's things, nullptr while none
    const BeltHolster* belt = nullptr;                   // REQUIRED
    const BagIcon* bag_icon = nullptr;                   // REQUIRED
    const ToolRack* rack = nullptr;                      // REQUIRED
    ToolRackWorld rack_world;                            // the tool place in the world this frame
    const WorldDrop* world = nullptr;                    // REQUIRED: the ground line
    bool ground_hit = false;                             // the finger meets the ground (toUZ)
    ::godot::Vector3 ground;                             // the point it meets
    const StowAnswer* answer = nullptr;                  // flora.stow.v1: the last verdict, nullptr while none
    double hand_band = 0.0;                              // the hand zone's offset (HudCover::hand), set by the HUD
};

/** What a release did. */
struct DragRelease {
    const char* kind = nullptr;   // GODOT_HUD_DRAG_*
    std::string key;              // the thing
    DragTarget target;            // GODOT_HUD_DRAG_SEND: where it goes
};

/** The thing under the finger. */
class DragLayer {
public:
    /** Binds measures, colours and the FitCheck; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Takes `thing` at `point`; `box` and `k` are where and how large it was last drawn. */
    void take(const DragThing& thing, const ::godot::Vector2& point, const ::godot::Rect2& box, double k);

    /** Follows the finger; true on the move that first goes past the slop (the selection falls then). */
    bool move(const ::godot::Vector2& point);

    /** Finds the place under the finger and takes the verdict for it; false with the reason in `failed`. */
    bool aim(const DragLayerFrame& now, std::string& failed);

    /**
     * Takes the stow bridge's answer in `now` for the place aim() found, every frame while moved: the bridge
     * answers the place a frame after the finger entered it, and a finger that then stands still moves no more.
     */
    void judge(const DragLayerFrame& now);

    /** Draws the ghost and the verdict while moved; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, std::string& failed);

    /**
     * Lets the thing go: tap, send, refusal or hold in `out`, the toast posted at `clock`; false with the
     * reason in `failed`. The layer is empty afterwards.
     */
    bool release(const DragLayerFrame& now, Toast& toast, double clock, DragRelease& out, std::string& failed);

    bool holding() const { return holding_; }
    bool moved() const { return holding_ && moved_; }

    /** The thing a moved drag holds, empty when none. */
    std::string dragged() const { return moved() ? thing_.key : std::string(); }

    /** The square about the hand within which a drag is carried to the hand (hud_drop_hand_radius). */
    ::godot::Rect2 hand_drop() const {
        const ::godot::Vector2 half(static_cast<float>(hand_radius_), static_cast<float>(hand_radius_));
        return ::godot::Rect2(::godot::Vector2(static_cast<float>(hand_x_), static_cast<float>(hand_y_)) - half,
                              half * 2.0f);
    }

    /** The place a moved drag stands over, nullptr when none (the hot edges). */
    const char* hot() const { return moved() ? target_.place : nullptr; }

    /** The module refused the thing at the hot place. */
    bool refused() const { return moved() && FitCheck::refused(verdict_); }

    /** The thing and the place the verdict is asked for; target().place is nullptr when none. */
    const DragTarget& target() const { return target_; }
    const DragThing& thing() const { return thing_; }

private:
    /** The drawing `art`, read once; nullptr with the reason in `failed`. */
    const UiItemSprite* sprite(const std::string& art, std::string& failed);

    /** ik: the scale of `drawing` for a thing of `length_cm` in `container`. */
    static double scale_in(const ContainerDef& container, const UiItemSprite& drawing, double length_cm);

    /** The container named `place` of `window`, nullptr when `place` is no container. */
    static const ContainerDef* container_of(const BagWindow& window, const char* place);

    /** freeSpot: the first free cell of the raster in the pack, else a random spot inside the inset. */
    bool free_spot(const DragLayerFrame& now, ::godot::Vector2& spot, std::string& failed);

    const UiTheme* theme_ = nullptr;
    double slop_ = 0.0;           // hud_gesture_drag_slop
    double ghost_alpha_ = 0.0;    // hud_ghost_alpha
    double max_box_ = 0.0;        // hud_ghost_max_box
    double shadow_dy_ = 0.0;      // hud_ghost_shadow_dy
    double shadow_blur_ = 0.0;    // hud_ghost_shadow_blur
    double hand_x_ = 0.0;         // hud_hand_x
    double hand_y_ = 0.0;         // hud_hand_y
    double hand_radius_ = 0.0;    // hud_drop_hand_radius
    double spot_inset_ = 0.0;     // hud_drop_spot_inset
    double spot_step_ = 0.0;      // hud_drop_spot_step
    ::godot::Color shade_;        // OVERLAY_SHADOW_LIFTED at hud_ghost_shadow_alpha
    FitCheck fit_;
    std::vector<std::pair<std::string, UiItemSprite>> sprites_;

    bool holding_ = false;        // S.drag
    bool moved_ = false;          // d.moved
    DragThing thing_;
    ::godot::Vector2 start_;      // down.x, down.y
    ::godot::Vector2 finger_;     // d.x, d.y
    ::godot::Vector2 grip_;       // d.ox, d.oy
    double taken_k_ = 0.0;        // d.k
    double ghost_k_ = 0.0;        // the ghost's scale as last aimed
    DragTarget target_;           // d.tg
    FitCheckFrame verdict_;       // the verdict for d.tg
};

}  // namespace ase::adp::godot
