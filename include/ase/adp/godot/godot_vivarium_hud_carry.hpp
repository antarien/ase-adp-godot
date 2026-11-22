#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM HUD CARRY BRIDGE
 *
 * @file        godot_vivarium_hud_carry.hpp
 * @brief       The carried things the bridge writes into the HUD's frame while flora.carry.v1 files no record
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.7 to 06.9, design decision 2026-11-10: the HUD draws every
 *              element from its frame and never asks whether a port delivered. While ase-inventory files no record
 *              on flora.carry.v1 (N0451) and judges no move on flora.stow.v1 (N0452), this bridge holds the
 *              mockup's initInv and follows its rules of equip(), stow(), placeIn(), layAt(), fits(), wt() and
 *              vol(), every number a hud_bridge_* token of SHA (theme.css --ase-hud-bridge-item-*, -container-*,
 *              -carry-*, -rack-*, -belt-*, -fill-*, -sap-*, -lay-*, -stow-*) and every drawing word of hud.items.
 *
 *              THE FRAME. Each frame it hands the HUD every carried thing (things, possession), the pack and the
 *              pouch with their records and contents, the two holsters, the hand, the tool place with the tools
 *              leaning at it and the things lying on the ground.
 *
 *              THE TOOL PLACE stands beside the patch as the camera of the first frame sees it: across along the
 *              camera's right and ahead along its view on the ground (rack_offset). The view lays the rack body of
 *              ase-pl-toolrack there (usecase:101625, toolrack.scene.v1 lay); once ase-physics has let it fall
 *              and the body stands on toolrack.rack.v1, fill() takes the place of the body, its height the rest
 *              height physics found. A tool leans at its slot along the place, slot_first plus slot_step for each
 *              tool before it, at the height of the place.
 *
 *              THE ACTS. take and equip lift a thing into the hand and stow what it held; stow puts the hand's
 *              thing back where it came from, else into the pack, else to the tool place; send moves a thing to
 *              the place a drop names when fits() lets it; sow takes one seed of the sow tool's item. A refused
 *              act leaves every thing where it lies and is named in the note the view logs.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), N0452 (flora.stow.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-20
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_bark_vessel.hpp>
#include <ase/adp/godot/godot_hud_compose.hpp>
#include <ase/adp/godot/godot_hud_ui_ground.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_watering_pot.hpp>

#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

struct VivariumFigureGearView;  // godot_host_figure_view.hpp: one piece of the gear port

/** The litres or kilograms a load may stand over its room or carry and still fit (the mockup's 1e-6 in fits()). */
constexpr double GODOT_HUD_FIT_EPSILON = 1.0e-6;

/** The place of the trough hung under the tap (the mockup's o.c = 'trunk'): no container, no ground, no hand. */
constexpr const char* GODOT_HUD_PLACE_TRUNK = "trunk";

/** The mockup's inventory while flora.carry.v1 is silent. */
class VivariumCarryBridge {
public:
    /** Reads the items, containers, carried things, tool place and rules of SHA; false with the missing name. */
    bool load(const UiTheme& theme, std::string& failed);

    /**
     * The tool place's offset from the patch (usecase:101625): fixed on the first call from the camera's `right`
     * and `ahead` on the ground plane, right times hud_bridge_rack_across plus ahead times hud_bridge_rack_ahead.
     * Its x and z are what the view hands to toolrack.scene.v1 lay; every later call returns the same offset.
     */
    ::godot::Vector3 rack_offset(const ::godot::Vector3& right, const ::godot::Vector3& ahead);

    /** True once the view asked the host to lay the rack body; the view asks once (ask_rack). */
    bool rack_asked() const;
    /** The way the slots of the tool place run: the camera's right of the frame the place was laid by. */
    ::godot::Vector3 rack_along() const;
    /** The things on the ground of the last fill, which the frame's world points at; lay_tools marks those it stands in 3D. */
    std::vector<GroundThing>& ground_things();

    /** Marks that the view asked the host to lay the rack body. */
    void ask_rack();

    /**
     * Fills the carry groups of `now`. `patch` is the patch in focus, `right` and `ahead` the camera's right and
     * view on the ground plane (unit length), `ground` the terrain the things lie on. `body_known` says the rack
     * body of ase-physics stands at `body` (world, from toolrack.rack.v1); the tool place then is that body.
     */
    void fill(VivariumHudFrame& now, const ::godot::Vector3& patch, const ::godot::Vector3& right,
              const ::godot::Vector3& ahead, const UiGround& ground, bool body_known,
              const ::godot::Vector3& body);

    /** Applies take, equip, stow or send. False when the act was refused, `note` names what was done or why not. */
    bool apply(const VivariumHudRequest& request, std::string& note);

    /**
     * sowTap (usecase:100388 sow a new seed): `picked` says the tap found the ground at `point`; a point farther than
     * hud_bridge_hand_sow_reach_m from the stem place is FORA DA ÁREA, a living birch (`dead` false) A BÉTULA AINDA
     * VIVE, no seed of the sow tool's item SEM SEMENTES NA BOLSA; else one seed goes in and the dibber's hole opens.
     * `toast` is the key of the refusal or of SEMENTE NA TERRA, `note` what was done. False when refused.
     */
    bool sow(bool picked, const ::godot::Vector3& point, bool dead, const char*& toast, std::string& note);

    /**
     * The spade's cavar at the stem place (operator 2026-11-13, usecase:100388, Q02): `picked` says the tap found the
     * ground at `point`; within hud_bridge_hand_sow_reach_m of the stem place and over no closed seed it opens the
     * planting hole there, which stands until cover() closes it. sow() lays a seed into an open hole without the
     * dibber's closing (Q03). `hole` is the ground the spade digs: the stem place where the hole stands, so the
     * ground act and the drawn hole meet (B0297). False with the reason in `note` when the tap opens no hole.
     */
    bool dig(bool picked, const ::godot::Vector3& point, ::godot::Vector3& hole, std::string& note);

    /**
     * The spade's amontoar over the planting hole (Q04): within the reach of the stem place an open hole with its seed
     * is filled, the seed lies covered. `hole` is the ground the fill goes on, the same stem place dig() dug, never
     * the tap beside it (B0297). False with the reason in `note` when there is no seeded hole to close.
     */
    bool cover(bool picked, const ::godot::Vector3& point, ::godot::Vector3& hole, std::string& note);

    /** The register task of the planting hole's port flora.seed.v1 (place, hole_open, seeded, covered, at). */
    static constexpr const char* GAP_PLANT = "N0485";

    /**
     * The register task of the bodies of the things on the ground (N0487, carrier N0448): they are to be
     * ase-physics bodies (HUB_PHYS_REG_BODY_ADD with mass and fall), falling by PhysicsGravSystem onto the dug
     * ground and handed out on a port as HUB_PHYS_BODY_X/Y/Z. The tool place is such a body already
     * (ase-pl-toolrack, toolrack.rack.v1); a thing lying loose on the ground is not, and fill() sets its height to
     * the ground under it each frame, a bridge default and no physics. The same default holds the tool place while
     * the host offers no rack port or its body has not been read yet.
     */
    static constexpr const char* GAP_BODY = "N0487";

    /**
     * brookTap (usecase:100399 carry brook water): `picked` says the tap found the ground at `point`, `laid` that a
     * brook runs from `from` to `to`. Only the can of the hand (hud_bridge_hand_can_item) within
     * hud_bridge_hand_brook_reach_m of that line acts, else `toast` stays nullptr and nothing happens; a full can
     * names O REGADOR JÁ ESTÁ CHEIO. True when the tap asks a fill: the caller sends it to the fill port, ase-equipment
     * fills the can, and engine_fill names ENCHEU NO RIACHO when the level comes back risen. The bridge writes no level.
     */
    bool brook(bool picked, const ::godot::Vector3& point, bool laid, const ::godot::Vector3& from,
               const ::godot::Vector3& to, const char*& toast, std::string& note);

    // The words of brookTap's toasts (texts.hud), both in the mockup's 'water'.
    static constexpr const char* TOAST_POT_FULL = "toast.pot_full";          // O REGADOR JÁ ESTÁ CHEIO
    static constexpr const char* TOAST_FILLED_BROOK = "toast.filled_brook";  // {l}: ENCHEU NO RIACHO · 3 L

    // The words of sowTap's toasts (texts.hud), in the mockup's 'alert' or 'life'.
    static constexpr const char* TOAST_OFF_PATCH = "toast.off_patch";  // FORA DA ÁREA
    static constexpr const char* TOAST_ALIVE = "sow_alive";            // A BÉTULA AINDA VIVE
    static constexpr const char* TOAST_NO_SEED = "sow_none";           // SEM SEMENTES NA BOLSA
    static constexpr const char* TOAST_SOWN = "toast.sown";            // SEMENTE NA TERRA
    static constexpr const char* COLOUR_LIFE = "DOMAIN_LIFE";          // the domain colour of the seed

    /**
     * The engine's level of the can (07.14, usecase:100367 water a patch): the first vessel of the gear port whose
     * kind is the can item (name_id of hud_bridge_hand_can_item) gives its water_l to the carried can, each time that
     * level changes; it is the only writer of the can's fill. A fill and a pour of the player (fill and drain ports)
     * or of the AI all show here. `rose_to` is the new level when it rose against the last snapshot, else below 0.
     * False while no such vessel stands.
     */
    bool engine_fill(const std::vector<VivariumFigureGearView>& gear, double& rose_to);

    /**
     * trunkTap: one spout goes into the bore, the trough hangs under it (GODOT_HUD_PLACE_TRUNK, sap 0, no home) and
     * the hand's thing is stowed. False when the trough or a spout is missing; `toast` is the key of the
     * refusal or of the bore, `note` what was done.
     */
    bool tap(const char*& toast, std::string& note);

    /**
     * plugTap while a tap `running`: one peg of the hand closes the bore, the trough takes `held_l` and is laid beside
     * the stem, the hand's thing is stowed. False when no tap runs or the hand holds no peg.
     */
    bool plug(bool running, double held_l, std::string& note);

    /**
     * The tick of a running tap on a living birch: `seconds` real at the pace `factor` are game hours (pace-hour-s),
     * each fills `held_l` by sap-per-hour up to `cap_l`. True on the frame the trough becomes full.
     */
    bool flow(double seconds, double factor, double cap_l, double& held_l) const;

    /** The verdict of fits() on moving `key` to `place`; false when no such thing is carried. */
    bool answer(const std::string& key, const char* place, bool rack, StowAnswer& out) const;

private:
    /** One item as SHA states it: hud.items for its drawing, hud_bridge_item_* for its facts. */
    struct Facts {
        std::string id;                  // the key of hud.items
        std::string tone;                // D.dom
        std::string asset;               // D.asset
        std::vector<std::string> verbs;  // D.verbs
        double view_w = 0.0;             // D.vw
        double view_h = 0.0;             // D.vh
        double length_cm = 0.0;          // D.len
        double kg = 0.0;                 // D.kg
        double litres = 0.0;             // D.l, a stack's piece
        double cap_l = 0.0;              // the litres a vessel takes, 0 for every other
        bool stack = false;              // D.stack
    };

    /** Where a thing lay before it went into the hand (o.home). */
    struct Home {
        bool set = false;
        const char* place = nullptr;
        ::godot::Vector2 at;
        ::godot::Vector3 ground;
        double along = 0.0;
        double angle = 0.0;
        bool rack = false;
    };

    /** One carried thing (an INV entry). */
    struct Carried {
        std::string key;              // o.key
        uint32_t facts = 0;           // index into facts_
        const char* place = nullptr;  // o.c: GODOT_HUD_CONTAINER_*, GODOT_HUD_PLACE_HAND or GODOT_HUD_PLACE_GROUND
        ::godot::Vector2 at;          // o.x, o.y in a container
        ::godot::Vector3 ground;      // o.gu, o.gz off the tool place
        double along = 0.0;           // its slot along the tool place, world
        double angle = 0.0;           // o.ang
        bool rack = false;            // o.rack
        int64_t order = 0;            // o.zo
        int64_t count = 0;            // o.n
        double fill_l = 0.0;          // o.fill
        double mm_per_l = 0.0;        // the column one litre lays in its bearer's hand, mm: the gear port's mm_per_l (N0517)
        double sap_l = 0.0;           // o.sap
        Home home;                    // o.home
    };

    /** wt(o): the weight of `thing` with what it holds. */
    double weight(const Carried& thing) const;

    /** vol(o): the room `thing` takes. */
    double room(const Carried& thing) const;

    /** fits(it, c): the refusal code of moving `thing` to `place`, empty when it goes; the values the word names. */
    std::string fits(const Carried& thing, const char* place, std::vector<HudPortValue>& values) const;

    /** layAt(): `thing` onto the ground at `point`, or to the next slot of the tool place. */
    void lay(Carried& thing, const ::godot::Vector3& point, bool rack);

    /** stow(it): the hand's thing back home, else into the pack, else to the tool place. */
    void stow(Carried& thing);

    /** equip(key): `thing` into the hand, the hand's thing stowed first. */
    void equip(Carried& thing);

    /** placeIn(it, c, x, y): `thing` into the container `place` at `at`, held inside its content less the drawn box. */
    void place_in(Carried& thing, const char* place, const ::godot::Vector2& at);

    /** The thing of `key`, else the first carried piece of the item `key` names (findIt); nullptr when none. */
    Carried* find(const std::string& key);
    const Carried* find(const std::string& key) const;

    /** Two places are the same when both name the same word. */
    static bool same_place(const char* a, const char* b);

    std::vector<Facts> facts_;
    std::vector<Carried> carried_;
    ContainerDefCarry pack_;        // hud_bridge_container_ranzen_*
    ContainerDefCarry pouch_;       // hud_bridge_container_bolsa_*
    ContainerDef pack_room_;        // hud_bag_*: the pack's content (C.w, C.h) and item scale (ik) for placeIn
    ContainerDef pouch_room_;       // hud_pouch_*: the same for the pouch
    std::string seed_item_;         // hud_bridge_hand_seed_item: the item sowTap takes one of (findIt('sementes'))
    double sow_reach_m_ = 0.0;      // hud_bridge_hand_sow_reach_m: the farthest a sowing tap lies from the stem place
    std::string can_item_;          // hud_bridge_hand_can_item: the can brookTap fills (handItem().id === 'regador')
    double engine_water_l_ = -1.0;  // the can's water_l of the last gear snapshot engine_fill applied; below 0 none yet
    double brook_reach_m_ = 0.0;    // hud_bridge_hand_brook_reach_m: the farthest a filling tap lies from the brook
    std::string spout_item_;        // hud_bridge_hand_spout_item: the spouts the sap mode counts (findIt('bica'))
    std::string vessel_item_;       // hud_bridge_hand_vessel_item: the trough the sap mode needs (have('cocho'))
    std::string plug_item_;         // hud_bridge_hand_plug_item: the peg plugTap takes from the hand (cavilha)
    double plug_across_ = 0.0;      // hud_bridge_plug_across: the trough laid right of the stem, world
    double plug_ahead_ = 0.0;       // hud_bridge_plug_ahead: and this far ahead of it, world
    double sap_per_hour_ = 0.0;     // hud_bridge_patch_sap_per_hour: litres a game hour fills the trough
    BarkVessel sap_rule_;           // the level of held sap: drawn above hud_item_sap_from (o.sap > 0.02)
    double hour_s_ = 0.0;          // hud_bridge_pace_hour_s: real seconds of a game hour at factor 1
    ::godot::Vector3 stem_place_;   // the patch's stem place of the last frame (U0, Z0 on the ground)
    bool sow_pending_ = false;      // sowTap accepted a seed since the last frame: fill() stamps its clock and place
    bool sown_ = false;             // S.hole was set
    double sown_at_ = 0.0;          // the view's clock of the sowing, seconds
    ::godot::Vector3 sown_place_;   // the stem place the seed went in (U0, Z0 on the ground)
    bool dug_ = false;              // the spade opened the planting hole at the stem place (Q02); none at the start
    bool seeded_ = false;         // a seed lies in the open planting hole (Q03); none at the start, nobody sowed
    bool covered_ = false;          // the planting hole is closed over its seed (Q04); none at the start
    double belt_len_cm_ = 0.0;      // hud_bridge_belt_len_cm
    double fill_kg_per_l_ = 0.0;    // hud_bridge_fill_kg_per_l
    double sap_kg_per_l_ = 0.0;     // hud_bridge_sap_kg_per_l
    double lay_tall_ = 0.0;         // hud_bridge_lay_tall
    double lay_angle_ = 0.0;        // hud_bridge_lay_angle
    double stow_x_from_ = 0.0;      // hud_bridge_stow_x_from
    double stow_x_span_ = 0.0;      // hud_bridge_stow_x_span
    double stow_y_from_ = 0.0;      // hud_bridge_stow_y_from
    double stow_y_span_ = 0.0;      // hud_bridge_stow_y_span
    double rack_across_ = 0.0;      // hud_bridge_rack_across
    double rack_ahead_ = 0.0;       // hud_bridge_rack_ahead
    double slot_first_ = 0.0;       // hud_bridge_rack_slot_first
    double slot_step_ = 0.0;        // hud_bridge_rack_slot_step
    double slot_ahead_ = 0.0;       // hud_bridge_rack_slot_ahead
    double slot_angle_ = 0.0;       // hud_bridge_rack_slot_angle
    bool rack_placed_ = false;      // the tool place was set from the first frame's camera
    bool rack_asked_ = false;       // the view asked the host to lay the rack body (toolrack.scene.v1 lay)
    ::godot::Vector3 rack_offset_;  // the tool place off the patch, from the first frame's camera
    ::godot::Vector3 rack_place_;   // the tool place: the rack body of ase-physics, else beside the patch
    ::godot::Vector3 rack_right_;   // the camera's right of that frame, along which the slots run
    ::godot::Vector3 rack_view_;    // the camera's view of that frame, ahead of the place
    int64_t next_order_ = 0;        // ++ZO
    std::vector<BagThing> pack_things_;
    std::vector<BagThing> pouch_things_;
    BagThing belt_first_;
    BagThing belt_second_;
    HandThing hand_;
    std::vector<RackTool> rack_tools_;
    std::vector<GroundThing> ground_things_;
};

}  // namespace ase::adp::godot
