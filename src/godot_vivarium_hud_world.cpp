/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD WORLD
 *
 * @file        godot_vivarium_hud_world.cpp
 * @brief       VivariumView::fill_hud_world - the frames of the HUD's blocks that stand on the world under the HUD
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5: fill_hud (godot_vivarium_hud.cpp) lays the HUD's frame from the
 *              ports and hands the blocks placed in the world to this unit: the cut marks on the plant in focus, the
 *              ground kit with the brush, the hand's verb and readings, the light and the patch of the world drop,
 *              the tool place, the carried things, the cut under the trunk in the mode water and the drag's hit on
 *              the ground. A value no port carries yet is the bridge's (godot_vivarium_hud_hand.cpp,
 *              godot_vivarium_hud_carry.cpp, godot_vivarium_hud_bridge.cpp) and its register id is logged once on
 *              the edge (open_gap), never shown: the picture stands whole (design decision 2026-11-10).
 *
 *              THE CUT UNDER THE TRUNK (usecase:100367 water a patch). The soil now and the share a planned pour
 *              adds are the hand's readings; the roots are the segments of the focused body on
 *              flora.organ.root.v1 (N0463), laid under the stem base of the plant in focus, and the hand
 *              bridge's drawRootCut tree with the gap open on the edge while that port carries no record.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), N0454 (flora.patch.v1 preview), N0462 (flora.scene.v1 cuttable), N0463
 *              (root zone, flora.organ.root.v1 segments), N0465 (the things as asset rows), N0487 (physics.body.v1 bodies
 *              of the tool place and the ground things), carrier N0448
 * @created     2026-11-11
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_figure_hand.hpp>
#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/hud/gap/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

void VivariumView::settle_bed_choice() {
    // A NEW VERB OR A NEW MODE DROPS THE BED'S START. The mockup forgets bedA on terr:v (m.html 947), on equipping
    // a tool (608) and on every setMode (933): the start mark of a bed only stands while the bed verb stays chosen.
    // The change is read where it can first matter: in the frame, and before a ground verb acts (send_hud_verb). A
    // frame that carries the chip's tap AND the first ground tap (run 20261112-155949: one frame of 1.3 s) otherwise
    // laid the start under leito and dropped it in the same frame as if the verb had changed after it.
    if (hud_state_.hud_.verb() != hud_state_.hud_verb_seen_ || hud_state_.hud_.mode() != hud_state_.hud_mode_seen_) {
        if (scene_state_.bed_started_) {
            note(::godot::String(("bed start dropped: verb " + hud_state_.hud_.verb() + " mode " + hud_state_.hud_.mode()).c_str()));
        }
        scene_state_.bed_started_ = false;
        hud_state_.hud_verb_seen_ = hud_state_.hud_.verb();
        hud_state_.hud_mode_seen_ = hud_state_.hud_.mode();
    }
}

void VivariumView::fill_hud_world(const VivariumPatchView* patch, const PlantOrigin* origin,
                                  const VivariumPatchSpec& spec, double opened) {
    // The cut marks on the internodes of the plant in focus: the bridge's TREE.cand while flora.scene.v1 marks none.
    PruneMarksFrame& prune = hud_state_.hud_now_.prune;
    prune.anchor = &hud_state_.hud_anchor_;
    prune.segments = &hud_state_.hud_segments_;
    prune.origin = origin != nullptr ? *origin : prune.origin;
    prune.elapsed = opened;
    open_gap(GODOT_HUD_GAP_PRUNE, "flora.scene.v1.cuttable");
    hud_state_.hud_hand_.cut(hud_state_.hud_now_.hand, prune.cuttable, patch != nullptr ? patch->object_id : GODOT_NO_OBJECT,
                  resources_.get_organs(GODOT_ORGAN_SEGMENT), hud_state_.hud_segments_);
    std::string cut_counts;
    if (hud_state_.hud_hand_.cut_changed(cut_counts)) {
        note(::godot::String(cut_counts.c_str()));
    }
    // Prune is TOOLS.prune.ok's TREE.cand.some(c=>!S.cuts.has(c)): the crown's own candidates still uncut, as cut()
    // just counted them on the organ port, never the bridge value alone - with it the fan offered PODÃO over a crown
    // of three shoots on the stem that marks none (run 20261114-c01-motion5: on_stem=3 chosen=0 left=0). The mark is
    // laid HERE and not in bridge_signs: fill_hud lays hud_state_.hud_now_ anew before that bridge, so hand.prune_left read 0
    // there in every frame and the fan never offered PODÃO (run 20261117-c01-motion9: left=1 at 21:01:21.668, the
    // fan of 21:01:22.668 with water and ground only). The resolver reads the marks after fill_hud (draw_hud).
    if (hud_state_.hud_bridge_branch_uncut_ && hud_state_.hud_now_.hand.prune_left > 0u) {
        hud_state_.hud_now_.game.marks.emplace_back("branch_uncut");
    }

    // The ground kit: the ground, its brooks, the brush under the finger and the patch the level verb levels.
    TerrainKitFrame& terrain = hud_state_.hud_now_.terrain;
    terrain.anchor = &hud_state_.hud_anchor_;
    terrain.ground = &hud_state_.hud_ground_;
    terrain.streams = &resources_.get_streams();
    terrain.elapsed = opened;
    if (origin != nullptr) {
        terrain.origin = *origin;
    }
    GroundReticleFrame& reticle = terrain.reticle;
    reticle.anchor = &hud_state_.hud_anchor_;
    reticle.ground = &hud_state_.hud_ground_;
    if (hud_state_.hud_.brushing()) {
        float x = 0.0f;
        float z = 0.0f;
        float nearest_px = -1.0f;
        reticle.brush_on = pick_ground(hud_state_.hud_frame_.xform(hud_state_.hud_.brush()), x, z, nearest_px);
        reticle.brush_x = static_cast<double>(x);
        reticle.brush_z = static_cast<double>(z);
    }
    settle_bed_choice();
    reticle.bed_on = scene_state_.bed_started_;
    reticle.bed_x = static_cast<double>(scene_state_.bed_x_);
    reticle.bed_z = static_cast<double>(scene_state_.bed_z_);
    reticle.patch_x = spec.x;
    reticle.patch_z = spec.z;
    reticle.patch_m = spec.half_m;

    // The crumbs of loosened soil in every mode (row N08): the cells of the owner's looseness raster (terrain.loose.v1,
    // N0456) once it stands, the loose bridge's points before, in the light of the hour, the mockup's light() under
    // the sky as SkyOvercast keeps it (land_light, the one source of it).
    GroundCrumbsFrame& crumbs = terrain.crumbs;
    crumbs.anchor = &hud_state_.hud_anchor_;
    crumbs.ground = &hud_state_.hud_ground_;
    const VivariumLooseView& owner_loose = resources_.get_loose();
    if (owner_loose.known) {
        crumbs.loose = &owner_loose.loose;
        crumbs.columns = owner_loose.columns;
        crumbs.rows = owner_loose.rows;
        crumbs.west_x = owner_loose.west_x;
        crumbs.south_z = owner_loose.south_z;
        crumbs.step_x_m = owner_loose.cell_m;
        crumbs.step_z_m = owner_loose.cell_m;
    } else {
        hud_state_.hud_loose_.frame(crumbs);
    }
    SkyOvercastShade overcast;
    hud_state_.hud_.overcast().shade(hud_state_.hud_now_.sky, overcast);
    crumbs.light = hud_state_.hud_.overcast().land_light(resources_.get_sky().sun_y, overcast.light);

    // The hand: the verb's brush and the area a level act covers; its readings follow the carried things below.
    // The area is its number alone: every word that takes {place} (verb_reading.level, rack_sub) sets ÁREA itself.
    HandSlotFrame& hand = hud_state_.hud_now_.hand;
    hand.bed_started = scene_state_.bed_started_;
    hand.area = ::godot::String::num_uint64(spec.place + 1u).utf8().get_data();
    const auto verb = hud_state_.hud_verbs_.find(hud_state_.hud_.verb());
    if (verb != hud_state_.hud_verbs_.end()) {
        hand.brush_m = verb->second.brush_m;
    }
    hand.elapsed = opened;

    // The world: the light of the hour and the place of the patch, where the carry gap is named.
    const VivariumSkyView& sky = resources_.get_sky();
    WorldDropFrame& world = hud_state_.hud_now_.world;
    world.anchor = &hud_state_.hud_anchor_;
    if (sky.known) {
        world.light = sky.sun_y;  // the sine of the sun's elevation: its share above the horizon
    }
    if (patch != nullptr) {
        world.patch = ::godot::Vector3(static_cast<float>(patch->x), static_cast<float>(patch->y),
                                       static_cast<float>(patch->z));
    }
    hud_state_.hud_now_.rack_world.anchor = &hud_state_.hud_anchor_;
    hud_state_.hud_now_.rack.anchor = &hud_state_.hud_anchor_;
    hud_state_.hud_now_.rack.area = hand.area;
    hud_state_.hud_now_.rack.elapsed = opened;

    // The carried things: flora.carry.v1 files no record yet (N0451), the bridge's inventory stands in its place.
    // The tool place is set beside the patch along the camera's right and view on the ground plane.
    open_gap(GODOT_HUD_GAP_CARRY, "flora.carry.v1.things");
    // The containers likewise: VivariumCarryBridge::fill writes the mockup's CAPS from SHA into the pack and the
    // pouch record of every frame (N0451, the fields flora.carry.v1 will carry).
    open_gap(GODOT_HUD_GAP_CARRY, "flora.carry.v1.containers(opening_cm,room_l,carry_kg,asset)");
    // The drawings of the things: UiItemSprite draws the sixteen things from SHA's item_art.json, the paths of the
    // mockup, while the asset pipeline carries none of them as a row (package 06.10: PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE
    // and config/vivarium_assets.json, form values after DSGN_024). Its id stands here, beside the carry it draws.
    constexpr const char* GAP_ASSET = "N0465";
    open_gap(GAP_ASSET, "vivarium_assets.json.things(16)");
    ::godot::Vector3 right;
    ::godot::Vector3 ahead;
    const ::godot::Viewport* view_port = node_->get_viewport();
    const ::godot::Camera3D* camera = view_port != nullptr ? view_port->get_camera_3d() : nullptr;
    if (camera != nullptr) {
        const ::godot::Basis basis = camera->get_global_transform().basis;
        right = basis.get_column(0);
        ahead = -basis.get_column(2);
        right.y = 0.0f;
        ahead.y = 0.0f;
        right = right.normalized();
        ahead = ahead.normalized();
    }
    // The rack body (usecase:101625): the view asks the host once to lay ase-pl-toolrack's rack at the tool place
    // off the patch (toolrack.scene.v1 lay), ase-physics lets it fall, and toolrack.rack.v1 hands its place back in
    // the patch frame; the tool place is then that body.
    bool body_known = false;
    ::godot::Vector3 body;
    if (patch != nullptr && resources_.rack_offered()) {
        if (!hud_state_.hud_carry_.rack_asked() && resources_.running()) {
            hud_state_.hud_carry_.ask_rack();
            const ::godot::Vector3 offset = hud_state_.hud_carry_.rack_offset(right, ahead);
            const ase::kernel::HostStatus laid = resources_.lay_rack(patch->object_id, offset.x, offset.z);
            note("rack lay at patch " + ::godot::String::num_uint64(patch->object_id) + " x=" +
                 ::godot::String::num(offset.x, 2) + " z=" + ::godot::String::num(offset.z, 2) +
                 " result=" + GodotHostResourceManager::status_name(laid));
        }
        for (const VivariumRackView& rack : resources_.get_racks()) {
            if (rack.object_id == patch->object_id) {
                body_known = true;
                body = world.patch + ::godot::Vector3(static_cast<float>(rack.x), static_cast<float>(rack.y),
                                                      static_cast<float>(rack.z));
            }
        }
    } else {
        open_gap(VivariumCarryBridge::GAP_BODY, "toolrack.rack.v1");  // the host offers no rack port (N0487)
    }
    // The can's level of ase-equipment (07.14, usecase:100367); a risen level is the fill at the brook, of the player's
    // tap on the fill port or of the AI, and names ENCHEU NO RIACHO with the litres the can holds now (m.html 970).
    double rose_to = -1.0;
    hud_state_.hud_carry_.engine_fill(resources_.get_figure_gear(), rose_to);
    if (rose_to >= 0.0) {
        std::string failed;
        // The values stand named before the test (-Wdangling-pointer, as in godot_vivarium_water.cpp).
        const std::vector<HudPortValue> held{HudPortValue{"l", hud_decimals(rose_to, 0)}};
        if (!hud_state_.hud_.toast(VivariumCarryBridge::TOAST_FILLED_BROOK, held, VivariumHud::COLOUR_WATER, failed)) {
            fail_hud(failed);
        }
    }
    hud_state_.hud_carry_.fill(hud_state_.hud_now_, world.patch, right, ahead, hud_state_.hud_ground_, body_known, body);
    open_gap(VivariumCarryBridge::GAP_BODY, "physics.body.v1.body_y");  // the loose ground things (N0487)
    // 06 8.3: the tool place in 3D - the rack body and every tool on the ground as instances of their files,
    // from the same places the HUD hit-tests by; what stands in 3D the 2D ground draws as shadow and hit box only
    hud_state_.hud_now_.rack_world.bodied = lay_tools(scene_state_.scene_, hud_state_.hud_now_.world.rack, hud_state_.hud_carry_.rack_along(), figure_state_.figure_look_.rack,
                                           hud_state_.hud_carry_.ground_things(), figure_state_.figure_look_.tools);
    open_gap(GODOT_HUD_GAP_PATCH, "flora.patch.v1.after_rel");  // the preview after the pour (N0454)
    hud_state_.hud_hand_.frame(hand, patch, reticle, hud_state_.hud_bridge_sap_l_, hud_state_.hud_.pouring());  // the hold as the HUD keeps it, before compose
    terrain.root_m = hud_state_.hud_hand_.zone_m();  // the ring of the ground mode: the patch record's root_m (RDX_SPREAD, N0463)

    // The cut under the trunk in the mode water: the soil now, the share the planned pour adds, and the roots
    // ase-radix grows for the patch's body, as ase-pl-flora files them on flora.organ.root.v1 (N0463). While the
    // port carries no root record at all, the hand bridge lays drawRootCut's tree in their place and the gap
    // stands on the edge; the first record closes it.
    RootCutFrame& cut = hud_state_.hud_now_.root_cut;
    cut.anchor = &hud_state_.hud_anchor_;
    cut.origin = origin != nullptr ? *origin : cut.origin;
    cut.soil_rel = hand.soil_rel;
    cut.pour_rel = hand.poured_l > 0.0 ? hand.after_rel - hand.soil_rel : 0.0;
    const std::vector<VivariumOrganView>& grown = resources_.get_organs(GODOT_ORGAN_ROOT);
    if (grown.empty()) {
        open_gap(GODOT_HUD_GAP_ROOT, "flora.organ.root.v1");
    } else {
        close_gap(GODOT_HUD_GAP_ROOT, "flora.organ.root.v1");
    }
    cut.roots = &hud_state_.hud_hand_.roots(patch, grown, hud_state_.hud_anchor_, cut.origin);

    // The tap hole in the birch: the owner's tap of flora.sap.v1 once its record stands (N0457), the bridge's tap from
    // hud_bridge_patch_sap_* before; the bark vessel hangs at the trunk while the tap runs.
    bool dead = false;
    for (const std::string& sign : hud_state_.hud_now_.game.signs) {
        dead = dead || sign == "dead";
    }
    // VivariumView::bridge_patch took the owner's tap from flora.sap.v1 above once its record stands (N0457) and
    // logged the gap's edge there; the bridge's members carry whichever state stands.
    TapHoleFrame& tap = hud_state_.hud_now_.tap;
    tap.running = hud_state_.hud_bridge_sap_running_ && !dead;
    tap.placed = hud_state_.hud_now_.tree_placed;
    tap.trunk = hud_state_.hud_now_.trunk;
    tap.tree_height = hud_state_.hud_now_.tree_height;
    tap.vessel = tap.running;
    tap.carry.held_l = hud_state_.hud_bridge_sap_l_;
    tap.carry.capacity_l = hud_state_.hud_bridge_sap_cap_l_;
    tap.clock = hud_state_.hud_now_.clock;

    // The hole the dibber made: ase-flora answers no sowing on flora.seed.v1 yet (N0451); the carry bridge, which
    // accepted the sowing, wrote its place and age into the frame above (VivariumCarryBridge::fill).
    open_gap(GODOT_HUD_GAP_CARRY, "flora.seed.v1(place,at)");
    hud_state_.hud_now_.sow.anchor = &hud_state_.hud_anchor_;
    // The mound or the seed under the birch follow the planting steps of the carry bridge (TreeShade, Q01).
    hud_state_.hud_now_.shade.seeded = hud_state_.hud_now_.sow.seeded;
    hud_state_.hud_now_.shade.covered = hud_state_.hud_now_.sow.covered;

    // The drag: where the finger meets the ground.
    DragLayerFrame& drag = hud_state_.hud_now_.drag;
    if (hud_state_.hud_down_) {
        drag_ground(drag);
    }
    bridge_stow(drag);
}

void VivariumView::drag_ground(DragLayerFrame& drag) const {
    // THE GROUND UNDER THE FINGER, read where the finger stands now (B0275). The mockup's drop reads the ground at
    // the release point itself (toUZ(p.x, p.y), m.html 821); a move and a release that arrive between two frames
    // aim at this place, never at the one the last frame read, so a thing let go over the ground lands there.
    drag.ground_hit = false;
    float x = 0.0f;
    float z = 0.0f;
    float nearest_px = -1.0f;
    double height = 0.0;
    if (pick_ground(hud_state_.hud_finger_, x, z, nearest_px) &&
        hud_state_.hud_ground_.height(static_cast<double>(x), static_cast<double>(z), height)) {
        drag.ground_hit = true;
        drag.ground = ::godot::Vector3(x, static_cast<float>(height), z);
    }
    // 8.6: the ray of the camera through the finger onto the rack body, read where the finger stands now
    drag.rack_world.struck =
        drag.rack_world.bodied && rack_struck(scene_state_.scene_, node_->get_viewport()->get_camera_3d(), hud_state_.hud_finger_);
}

}  // namespace ase::adp::godot
