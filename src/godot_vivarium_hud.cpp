/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD
 *
 * @file        godot_vivarium_hud.cpp
 * @brief       VivariumView - the HUD of the care view: its frame from the ports (its drawing in
 *              godot_vivarium_hud_draw.cpp, its configuration and the still mode in godot_vivarium_hud_config.cpp)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5: once the world stands initialised and no error panel stands,
 *              the canvas over the scene is VivariumHud (godot_hud_compose.hpp). Every frame the view lays the
 *              HUD's frame on the placed layout (place_hud), fills every block's frame from the ports it reads
 *              (fill_hud, which first lays the surface and the outer bands' offsets, HudCover, in the frame's
 *              units), and draw_hud (godot_vivarium_hud_draw.cpp) lets the HUD resolve its mode and draws it. The
 *              frames of the blocks placed in the world (cut marks, ground kit, hand, world drop, tool place, carried
 *              things, the cut under the trunk, the drag) are filled in godot_vivarium_hud_world.cpp
 *              (fill_hud_world). A value no port carries yet is the mockup's, written by the bridge from
 *              hud_bridge_* of SHA (godot_vivarium_hud_bridge.cpp), and its register id is logged once on the edge
 *              of the bridge (open_gap), never shown: the picture stands whole (design decision 2026-11-10). The
 *              touches go through godot_vivarium_hud_input.cpp.
 *
 *              BACKEND GAPS OF THE FRAME (register carrier N0448): N0449 weather, N0450 trail, N0451 carry,
 *              N0453 skill (the level stands at hud.perception.start), N0454 patch preview and growth,
 *              N0456 loosened soil, N0457 sap, N0459 pace, N0460 clock shares, N0461 sign states,
 *              N0462 marked internodes, N0463 roots, N0464 the measure request, N0530 the body of the figure,
 *              B0657 the camp fire. Each id is logged with its port field where its bridge default is written
 *              (bridge_trail, bridge_skill, bridge_patch, fill_hud_world, send_hud_verb, bridge_clock,
 *              bridge_weather, bridge_signs, bridge_pace, bridge_body, bridge_fire).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {
// usecase:101330 watch the growing plant body (m.html 1033): NOVO ESTÁGIO · {stage} in DOMAIN_LIFE once the patch
// reaches a higher stage; {stage} is the word texts.hud.stages.<n> of the stage reached.
constexpr const char* HUD_TOAST_NEW_STAGE = "toast.new_stage";
constexpr const char* HUD_WORD_STAGE = "stages.";
// B0783: the share of fuel the line `hud survival` names in whole tenths, so the line moves on a tenth, never per tick
constexpr double HUD_FUEL_TENTHS = 10.0;

/**
 * The states the survival layer of `frame` draws by, as the line `hud survival ...` (U0245): the rest card (U05),
 * the four signs of the rail SINAIS (U02), the air mark (U06: the owner's grade of the air, and the soot in the air of the
 * seat's cell, which draws its smoke glyph without the word), the food of the bag and whether its spoil stands above
 * 0, the boundary the ring ESTRAGO draws a bow from (U07); the turbid drop of U08 follows poisoned; and the camp fire
 * of the card FOGO (U04, B0783): whether a fire stands by the figure, its stage (0 ignite to 4 ash) and whether it
 * burns at a rate, and the share of fuel left in whole tenths (HUB_FIRE_CELL_FUEL over HUB_FIRE_CELL_FUEL_LAID of
 * ingle.fire.v1, rounded down). Every field is a flag, a stage or a tenth, so the line changes only on an edge, never
 * per tick: the fuel falls every tick, its tenth about ten times over a whole burn.
 */
std::string survival_line(const VivariumHudFrame& frame) {
    const auto bit = [](bool on) { return on ? "1" : "0"; };
    const SurvivalBodyFrame& body = frame.body;
    return std::string("hud survival resting=") + bit(frame.rest.shown) + " sleeping=" + bit(frame.rest.sleeping) +
           " poisoned=" + bit(body.poisoned) + " bleeding=" + bit(body.bleeding) + " emaciated=" +
           bit(body.emaciated) + " well_rested=" + bit(body.well_rested) + " toxic=" + bit(frame.air.toxic) +
           // the smoke glyph of the air mark also stands on the soot of the seat's cell (SurvivalAirMark::set, D2500)
           " smoke=" + bit(frame.air.soot > 0.0 || frame.air.smell > 0.0) + " food=" + bit(frame.food.shown) + " spoiling=" + bit(frame.food.shown && frame.food.spoil > 0.0) +
           " fire=" + bit(frame.fire.shown) + " fire_stage=" + std::to_string(frame.fire.stage) + " burning=" +
           bit(frame.fire.shown && frame.fire.burning) + " fuel_tenths=" +
           // the share lies in 0 to 1, so the cast toward zero is the rounding down
           std::to_string(frame.fire.shown
                              ? static_cast<int32_t>(ase::math::clamp(frame.fire.fuel, 0.0, 1.0) * HUD_FUEL_TENTHS)
                              : 0);
}
}  // namespace

bool VivariumView::hud_live() const {
    return configured_ && initialised_ && failure_text_.is_empty() && placement_.scale > 0.0f;
}

void VivariumView::place_hud() {
    // The mockup's frame on the middle band of the placed layout: one mockup unit is hud_unit_ logical pixels.
    const float k = static_cast<float>(static_cast<double>(placement_.scale) * hud_unit_);
    hud_frame_ = ::godot::Transform2D(0.0f, ::godot::Vector2(k, k), 0.0f,
                                      ::godot::Vector2(placement_.offset_x, placement_.middle_y));
    ::godot::Viewport* viewport = node_->get_viewport();
    hud_anchor_.follow(viewport != nullptr ? viewport->get_camera_3d() : nullptr, hud_frame_);
    hud_ground_.follow(resources_.get_terrain(), start_.ground, resources_.terrain_generation());
}

bool VivariumView::fill_hud(std::string& failed) {
    hud_now_ = VivariumHudFrame{};
    // The surface and the outer bands in the units of hud_frame_ (the zones on a tall surface): the band zone and the
    // hand zone stand on the band their y belongs to (band_offset), their offset against the middle band over the
    // frame's scale; the surface is the drawn surface through the inverse of the frame.
    ::godot::Rect2 head_zone;
    ::godot::Rect2 hand_zone;
    if (!hud_.theme().zone("band", head_zone) || !hud_.theme().zone("hand", hand_zone)) {
        failed = "hud.zones: band and hand";
        return false;
    }
    const double k = static_cast<double>(hud_frame_.get_scale().y);
    if (!(k > 0.0)) {
        failed = "hud frame: no scale";
        return false;
    }
    hud_now_.cover.head =
        static_cast<double>(band_offset(layout_.bands, placement_, head_zone.position.y) - placement_.middle_y) / k;
    hud_now_.cover.hand =
        static_cast<double>(band_offset(layout_.bands, placement_, hand_zone.position.y) - placement_.middle_y) / k;
    const ::godot::Transform2D to_frame = hud_frame_.affine_inverse();
    const ::godot::Vector2 from = to_frame.xform(::godot::Vector2(0.0f, 0.0f));
    const ::godot::Vector2 to = to_frame.xform(::godot::Vector2(drawn_surface_.width, drawn_surface_.height));
    hud_now_.cover.surface = ::godot::Rect2(from, to - from);
    if (hud_.mode() != hud_mode_) {
        // A mode the HUD opened since the last drawn frame: draw_hud writes its edge line once the frame of the new
        // mode stands drawn and its places are logged.
        const uint32_t focus = start_.patches.empty() ? GODOT_NO_PLACE
                                                      : patch_focus_place(selected_place_, start_.patches.front().place);
        // The birch in focus is the patch the fan serves (hud.focus.fan: one tool shows while a place is selected):
        // the focus selects the patch in focus where none is, as Regar and Nivelar do.
        if (hud_.mode() == VivariumHud::MODE_FOCUS && selected_place_ == GODOT_NO_PLACE && focus != GODOT_NO_PLACE) {
            select(focus);
        }
        hud_refresh_.force();  // setMode renders at once in the mockup (T10)
    }
    hud_now_.clock = hud_clock_;

    // THE PATCH IN FOCUS: the selected one, without a selection the first - the place the camera looks at.
    if (start_.patches.empty()) {
        failed = "start.patches";
        return false;
    }
    const uint32_t focus = patch_focus_place(selected_place_, start_.patches.front().place);
    const VivariumPatchSpec* spec = nullptr;
    for (const VivariumPatchSpec& each : start_.patches) {
        if (each.place == focus) {
            spec = &each;
            break;
        }
    }
    if (spec == nullptr) {
        failed = "start.patches: the place in focus";
        return false;
    }
    // THE READINGS ARE SAMPLED AT THE MOCKUP'S CADENCE (T10, m.html 1037): render(false) writes the simulation into
    // the HUD every hud_refresh_ms and in every held frame (UiRefresh::advance in tick_hud); between two samples the
    // HUD shows the readings of the last one, while geometry, scene and animations run on in every frame. An event
    // renders at once: another place in focus, its record appearing or gone, a new stage or condition (the stage
    // toast and the death, m.html 1033), besides the press, the release and the mode the view forces itself.
    const VivariumPatchView* live = resources_.get_patch_at(focus);
    if (focus != hud_shown_.place || (live != nullptr) != hud_shown_.patch_known ||
        (live != nullptr && (live->stage != hud_shown_.patch.stage || live->cond_known != hud_shown_.patch.cond_known ||
                             live->wilted != hud_shown_.patch.wilted || live->dead != hud_shown_.patch.dead))) {
        hud_refresh_.force();
    }
    if (hud_refresh_.take()) {
        hud_shown_.place = focus;
        hud_shown_.patch_known = live != nullptr;
        if (live != nullptr) {
            hud_shown_.patch = *live;
        }
        hud_shown_.clock = resources_.get_clock();
        // Night is the sun under the horizon of ase-pl-sky's sun, the same test the scene's sun disc stands by.
        const VivariumSkyView& sampled_sky = resources_.get_sky();
        hud_shown_.night = sampled_sky.known && !(sampled_sky.sun_y > 0.0);
        // the debug drawer's run numbers are readings of the same render (debugLayer, m.html 903)
        hud_shown_.simulation_s = resources_.simulation_seconds();
        hud_shown_.steps = resources_.tick_count();
    }
    const VivariumPatchView* patch = hud_shown_.patch_known ? &hud_shown_.patch : nullptr;
    const PlantOrigin* origin = nullptr;
    hud_segments_.clear();
    if (patch != nullptr) {
        const auto found = scene_origins_.find(patch->object_id);
        if (found != scene_origins_.end()) {
            origin = &found->second;
        }
        plant_segments(patch->object_id, hud_segments_);
    }

    // The game state: the marks the patch record decides, the level the skill port will carry. flora.skill.v1
    // files no level yet: the bridge writes the start level of SHA in its place (N0453, bridge_skill).
    HudGameState& game = hud_now_.game;
    game.place_selected = selected_place_ != GODOT_NO_PLACE;
    bridge_skill(game);
    if (patch != nullptr) {
        if (patch->dead) {
            game.marks.push_back(VivariumHud::MARK_DEAD);
        } else if (patch->cond_known) {
            game.marks.push_back(VivariumHud::MARK_ALIVE);
        }
        if (patch->grow) {
            game.marks.push_back(VivariumHud::MARK_STAGE_GROW);
        } else if (patch->mature) {
            game.marks.push_back(VivariumHud::MARK_STAGE_MATURE);
        }
    }
    // The mockup's lastKey (m.html 903): the fades and drawings of every frame below play again from this clock when
    // the mode, the open sign, the level, the debug view, its drawer or tab, the stage or the death changed (T05).
    // The seconds count on the wall clock as the mockup's CSS animations do, never on the capped hud_clock_.
    const double opened = hud_.replay(game.level, patch != nullptr ? patch->stage : GODOT_STAGE_NONE,
                                      patch != nullptr && patch->dead);

    // The birch on the screen: the foot of its trunk and the height up to its highest internode.
    if (origin != nullptr) {
        hud_now_.tree_placed = hud_anchor_.trunk(*origin, hud_now_.trunk);
        ::godot::Vector2 crown;
        if (hud_now_.tree_placed && hud_anchor_.crown(*origin, hud_segments_, crown)) {
            hud_now_.tree_height = static_cast<double>(hud_now_.trunk.y - crown.y);
        }
    }
    // The shade under the birch, or the seed mound: the stem place and the stage of the stand (N19). The shade is
    // drawn onto the ground as the terrain lies now, never at the stem base the patch record carried.
    // BRIDGE GAP (register D2314, carrier N0487): for the seed mound the view sets a thing onto the terrain height
    // itself, so a planting hole lowers it and a filled one raises it again. The height of the sown seed belongs to
    // its owner, reported through its port as the body ase-physics holds (the seed port does so for flying seeds,
    // D2312). Until the patch port carries it, the terrain at the stem stands in; a bridge, never evidence
    hud_now_.shade.placed = origin != nullptr && hud_ground_.surface(hud_anchor_, origin->x, origin->z, 0.0,
                                                                      hud_now_.shade.trunk);
    hud_now_.shade.stage = patch != nullptr ? patch->stage : GODOT_STAGE_NONE;
    hud_now_.shade.seed = patch != nullptr && patch->seed;
    hud_now_.shade.sprout = patch != nullptr && patch->sprout;
    hud_now_.shade.grow = patch != nullptr && patch->grow;
    if (hud_now_.shade.placed) {
        open_gap(VivariumCarryBridge::GAP_BODY, "flora.patch.v1.mound_y");  // seed mound on the terrain (D2314)
    }

    // The clock record: the SURA date, the hour and the phase of the band and of the calendar sheet.
    // The port's record of the last sample first; every group it leaves empty is the mockup's, written by the bridge
    // (N0460).
    VivariumClockView clock = hud_shown_.clock;
    bridge_clock(clock);
    SkyBandFrame& band = hud_now_.band;
    band.vec = static_cast<uint32_t>(clock.year);
    band.lun = static_cast<uint32_t>(clock.month);
    band.tum = static_cast<uint32_t>(clock.month_day);
    band.hour = clock.hour;
    band.period = static_cast<int32_t>(clock.period);
    // The day record (N0460, flora.day.v1) with sunrise, sunset and the season, and the phase records
    // (flora.phase.v1) with the hour each phase begins, in the order of GODOT_PERIOD_*; the last phase ends where
    // ase-time's day ends.
    band.phase_starts = clock.phase_start_h;
    band.hours_per_day = clock.hours_per_day;
    band.sunrise = clock.sunrise_h;
    band.sunset = clock.sunset_h;
    band.season = static_cast<int32_t>(clock.season);
    // Night as the last sample read it from ase-pl-sky's sun (hud_shown_.night).
    band.night = hud_shown_.night;
    band.elapsed = hud_clock_;
    CalendarSheetFrame& sheet = hud_now_.sheet;
    sheet.year.lun = band.lun;
    sheet.year.months_per_year = static_cast<uint32_t>(clock.months_per_year);
    // The season records (N0460, flora.season.v1) and today's share of the year from the day record: ase-calendar
    // places the day, the ring only turns the share into an angle; the bridge filled what the ports left empty.
    // The first LUN of a season is fractional (CAL_CONST_SEASON_*_START over CAL_CONST_DAYS_PER_MONTH plus 1): the arc
    // starts on the day the season begins, not at the start of the LUN that holds it.
    sheet.year.season_starts = clock.season_start_lun;
    sheet.year.season_light = clock.season_light_h;
    sheet.year.year_share = clock.year_share;
    // B0280: the share of a whole LUN from the day record places the LUN marks and the season arcs on the scale of
    // today's share, never the LUN count of a year (the last LUN is shorter).
    sheet.year.lun_share = clock.lun_share;
    sheet.year.elapsed = opened;
    // The past events of the trail on arrival and the ring's dots: flora.trail.v1 files none yet, so the bridge
    // writes the mockup's S.log (N0450, bridge_trail).
    bridge_trail(hud_now_.trail, sheet.year);
    // The body of the figure for the rail CORPO from survival.body.v1 of the seat's figure; without its record the
    // bridge writes the mockup's body under its gap (N0530, bridge_body).
    bridge_body(hud_now_.body);
    // The camp fire for the card FOGUEIRA from ingle.fire.v1; without a record the bridge writes the mockup's camp
    // fire under its gap (B0657, bridge_fire).
    bridge_fire(hud_now_.fire);
    // The figure's rest for the rest card: survival.rest.v1 lays is_resting and sleeping of the seat's figure; while
    // no record carries them the bridge writes the mockup's resting figure (N0530, N0555, bridge_rest).
    bridge_rest(hud_now_.rest);
    // The owner's grade of the air at the figure's cell for the mark AR TÓXICO from the breath of survival.body.v1;
    // without it the bridge writes the mockup's air under its gap (N0530, bridge_air).
    bridge_air(hud_now_.air);
    // The spoil of the bag's food for the ring ESTRAGO from survival.food.v1; without the port the bridge writes the
    // mockup's food of the bag under its gap (N0530, bridge_food).
    bridge_food(hud_now_.food);
    // The states of the frames just written, logged on their edges for the acceptance (survival_line, U0245).
    const std::string survival = survival_line(hud_now_);
    if (survival != survival_watch_.line) {
        survival_watch_.line = survival;
        note(::godot::String(survival.c_str()));
    }
    sheet.day.vec = band.vec;
    sheet.day.lun = band.lun;
    sheet.day.tum = band.tum;
    sheet.day.hour = band.hour;
    sheet.day.period = band.period;
    // The shares of the day ase-time places the clock, each phase start, the sunrise and the sunset at (N0460,
    // flora.day.v1 and flora.phase.v1); the night is the band's.
    sheet.day.phase_shares = clock.phase_start_share;
    sheet.day.day_share = clock.day_share;
    sheet.day.sunrise_share = clock.sunrise_share;
    sheet.day.sunset_share = clock.sunset_share;
    sheet.day.night = band.night;
    sheet.day.elapsed = opened;
    sheet.forecast.elapsed = opened;
    bridge_pace(sheet.glass);
    sheet.glass.hour = clock.hour;  // the hourglass turns once per game hour, as fast as the clock runs
    sheet.glass.elapsed = opened;
    sheet.elapsed = opened;

    // The veil stands from the horizon of the scene camera down.
    double horizon_y = 0.0;
    if (!hud_anchor_.horizon(horizon_y)) {
        failed = "the horizon of the scene camera";
        return false;
    }
    hud_now_.veil.horizon_y = horizon_y;
    hud_now_.veil.time = hud_clock_;
    hud_now_.veil.dt = hud_dt_;
    // The weather of the band, the strip, the veil and the hand's rain warning: sky.weather.v1's record, the
    // mockup's while it is silent.
    bridge_weather(band, sheet.forecast, hud_now_.veil, hud_now_.hand, hud_now_.sky);

    // The signs: the tree they stand at and the line of the first brook on the ground.
    SignMarkerFrame& signs = hud_now_.signs;
    signs.trunk_placed = hud_now_.tree_placed;
    signs.trunk = hud_now_.trunk;
    signs.height = hud_now_.tree_height;
    const std::vector<VivariumStreamView>& streams = resources_.get_streams();
    if (!streams.empty()) {
        // The brook's line on the water the camera sees (SignMarker::follow_brook, B0285); its flank is told once
        // when it changes, so a sign beside the water names what put it there.
        const float left = hud_now_.cover.surface.position.x;
        std::string brook_line;
        SignMarker::follow_brook(streams.front(), ground_surface_.arrays, ground_surface_.revision, hud_anchor_,
                                 hud_ground_, left, left + hud_now_.cover.surface.size.x, hud_brook_, signs.brook,
                                 brook_line);
        if (brook_line != hud_brook_.told) {
            hud_brook_.told = brook_line;
            note(::godot::String(brook_line.c_str()));
        }
    }
    signs.elapsed = hud_clock_;
    signs.opened = opened;
    // The signs themselves: the ports' sign cases, the mockup's signs() over this frame's values while they are
    // silent (N0461, N0449).
    bridge_signs(game, signs, patch, clock, band, hud_now_.hand);

    // The focus: the patch record of the plant in focus.
    FocusClusterFrame& view = hud_now_.focus;
    view.trunk = hud_now_.trunk;
    view.height = hud_now_.tree_height;
    view.area = focus + 1u;
    view.debug = hud_.debug_on();
    // A NEW STAGE IS ANNOUNCED ONCE. The mockup raises S.stage when growth fills and posts NOVO ESTÁGIO with the
    // stage's word (m.html 1033); here ase-flora's patch record carries the stage, and the frame that first sees a
    // higher one than the last posts the toast. The first frame only learns the stage, a dead plant grows none.
    if (patch != nullptr && patch->stage < GODOT_STAGE_COUNT) {
        const bool rose = hud_stage_seen_ != GODOT_STAGE_NONE && patch->stage > hud_stage_seen_ &&
                          !patch->dead;
        hud_stage_seen_ = patch->stage;
        if (rose) {
            std::string stage_word;
            if (!word_line(hud_.theme(), HUD_WORD_STAGE + std::to_string(patch->stage), stage_word, failed)) {
                return false;
            }
            const std::vector<HudPortValue> reached{HudPortValue{"stage", stage_word}};
            note(::godot::String(("hud stage reached " + std::to_string(patch->stage)).c_str()));
            if (!hud_.toast(HUD_TOAST_NEW_STAGE, reached, VivariumCarryBridge::COLOUR_LIFE, failed)) {
                return false;
            }
        }
    }
    if (patch != nullptr) {
        view.stage = patch->stage;
        view.cond_known = patch->cond_known;
        view.wilted = patch->wilted;
        view.dead = patch->dead;
        view.soil_mm = patch->soil_mm;
        view.soil_rel = patch->soil_rel;
        view.coverage = patch->coverage;
    }
    // Growth, the days to the next stage and its mark on the year ring, the loose soil and the sap: the ports' values
    // first, every field no port carries yet the mockup's, written by the bridge (N0454, N0456, N0457).
    bridge_patch(view, patch, clock, sheet.year);
    view.elapsed = opened;
    hud_now_.fan.elapsed = opened;
    hud_now_.trail.elapsed = opened;
    // Every block that moves holds its motion while the still mode stands (read_hud_still): the frame and each block
    // frame the HUD hands on without passing it down itself.
    hud_now_.still = hud_still_;
    for (bool* held : {&hud_now_.band.still, &hud_now_.signs.still, &hud_now_.focus.still, &hud_now_.fan.still,
                       &hud_now_.prune.still, &hud_now_.trail.still, &hud_now_.sheet.still, &hud_now_.sheet.year.still,
                       &hud_now_.sheet.forecast.still, &hud_now_.sheet.glass.still, &hud_now_.rack.still,
                       &hud_now_.hand.still, &hud_now_.debug.still}) {
        *held = hud_still_;
    }

    fill_hud_world(patch, origin, *spec, opened);
    // debug_patch names the patch in focus, the selected one or without a selection the first (patch_focus_place),
    // as the mockup shows the line in every open drawer (PLAN_ASE_VIVARIUM_PHASE_06_HUD, debug_layer).
    fill_hud_debug(patch);
    hud_now_.debug_data = &debug_;
    return true;
}

}  // namespace ase::adp::godot
