/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD SURVIVAL BRIDGE
 *
 * @file        godot_vivarium_hud_bridge_survival.cpp
 * @brief       VivariumView - the body, the camp fire, the air and the rest the survival HUD draws from its frame
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5 and the survival rows U01 to U08: the rail CORPO, the states on
 *              the rail SINAIS, the card FOGUEIRA, the air mark and the rest card draw from their frame and never ask
 *              whether a port delivered. The readers take the owners' records: survival.body.v1 of ase-pl-survival
 *              (GodotHostResourceManager::read_survival_bodies) and ingle.fire.v1 of ase-pl-ingle
 *              (GodotHostResourceManager::read_survival_fires). Where a record or a key is missing, the bridge
 *              writes the value of the accepted mockup (hud_bridge_survival_* of SHA, read through UiTheme) and logs
 *              the gap once on its edge (open_gap) with the keys it waits for. Nothing of this stands in the picture.
 *              The body, the camp fire and the air take the record over the moment it arrives; the air reads breath
 *              of survival.body.v1, the share of the breathability layer of ase-gis in the cell under the seat's
 *              figure (GIS_LOCAL_BREATH, N0532, the field breath of VivariumSurvivalBodyView), and the rest card reads
 *              is_resting and sleeping of survival.rest.v1, a port of its own because the body record is full at the
 *              width of the host (VivariumSurvivalRestView in godot_host_resource_manager_class.hpp; sleeping is
 *              MTB_IS_SLEEPING, the sleep action BDI_CONST_ACTION_SLEEP of ase-bdi, N0555). The spoil ring reads
 *              spoil of survival.food.v1, the mass-weighted rot of the food the seat's figure carries
 *              (ENT_FOOD_ROT_CARRIED of ase-entropy, SurvivalHostFoodExptSystem, point T.8 of
 *              PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP; GodotHostResourceManager::read_survival_foods).
 *
 *              BRIDGE GAPS (register carrier N0448): N0530 the body of the seat's figure in survival.body.v1, its
 *              rest and sleep in survival.rest.v1 and the spoil of its food in survival.food.v1. N0532 the air at
 *              the seat's figure, breath in survival.body.v1. B0657 the fire cells of ingle.fire.v1. B0662 the smoke of the card: the owner's grade
 *              breath_state weighed against breath_normal (ATM_CONST_BREATHABILITY_NORMAL), both laid per record by
 *              IngleHostFireExptSystem of ase-pl-ingle; the gap opens only while a record misses one of them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0530, N0532, N0555, B0657, B0662, D2500 (the soot of survival.smoke.v1 for the smoke plume), carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_vivarium_hud_bridge_read.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstdint>
#include <string>

namespace ase::adp::godot {

namespace {

/** The token `token`, which must be 0 or 1, into `out`; false with the token's name in `failed`. */
bool read_state(const UiTheme& theme, const char* token, bool& out, std::string& failed) {
    double state = 0.0;
    if (!read_bridge_in(theme, token, 0.0, 1.0, state, failed) || !(state == 0.0 || state == 1.0)) {
        failed = std::string(token) + ": 0 or 1";
        return false;
    }
    out = state == 1.0;
    return true;
}

/**
 * Whether the owner grades the air of `cell` worse than normal, into `toxic`; false while the record lacks one of
 * the two grades. Both are ase-atmosphere's: ATM_GAS_BTH_STATE and the grade it names normal air with
 * (ATM_CONST_BREATHABILITY_NORMAL), so the comparison takes no threshold of the adapter.
 */
bool graded_toxic(const VivariumSurvivalFireView& cell, bool& toxic) {
    if (std::isnan(cell.breath_state) || std::isnan(cell.breath_normal)) {
        return false;
    }
    toxic = cell.breath_state > cell.breath_normal;
    return true;
}

}  // namespace

bool VivariumView::load_hud_bridge_survival(const UiTheme& theme) {
    // hud_bridge_survival_* of SHA (N0530): the mockup's body of the figure for the rail CORPO; every row counts, the
    // levels lie within the rail's full level (hud_survival_rail_level_full), stress and fatigue are shares; health
    // stands on the full level like the mockup's 64 and is filed as the share its owner MTB_HEALTH carries.
    std::string failed;
    double level_full = 0.0;
    if (!theme.measure("hud_survival_rail_level_full", level_full) || !(level_full > 0.0) ||
        !theme.measure("hud_bridge_survival_temp", hud_bridge_body_.temp.value) ||
        !read_bridge_in(theme, "hud_bridge_survival_temp_stress", 0.0, 1.0, hud_bridge_body_.temp_stress, failed) ||
        !read_bridge_in(theme, "hud_bridge_survival_fatigue", 0.0, 1.0, hud_bridge_body_.fatigue.value, failed) ||
        !read_bridge_in(theme, "hud_bridge_survival_hunger", 0.0, level_full, hud_bridge_body_.hunger.value, failed) ||
        !read_bridge_in(theme, "hud_bridge_survival_thirst", 0.0, level_full, hud_bridge_body_.thirst.value, failed) ||
        !read_bridge_in(theme, "hud_bridge_survival_energy", 0.0, level_full, hud_bridge_body_.energy.value, failed) ||
        !read_bridge_in(theme, "hud_bridge_survival_health", 0.0, level_full, hud_bridge_body_.health.value, failed)) {
        return fail_config("hud_bridge_survival_*: temp, temp_stress and fatigue 0..1, the levels 0..level_full");
    }
    hud_bridge_body_.health.value /= level_full;
    hud_bridge_body_.temp.counts = true;
    hud_bridge_body_.hunger.counts = true;
    hud_bridge_body_.thirst.counts = true;
    hud_bridge_body_.energy.counts = true;
    hud_bridge_body_.fatigue.counts = true;
    hud_bridge_body_.health.counts = true;
    // The states on the rail SINAIS (U02): which of the four the mockup's body shows, each 0 or 1.
    if (!read_state(theme, "hud_bridge_survival_poisoned", hud_bridge_body_.poisoned, failed) ||
        !read_state(theme, "hud_bridge_survival_bleeding", hud_bridge_body_.bleeding, failed) ||
        !read_state(theme, "hud_bridge_survival_emaciated", hud_bridge_body_.emaciated, failed) ||
        !read_state(theme, "hud_bridge_survival_well_rested", hud_bridge_body_.well_rested, failed)) {
        return fail_config(failed.c_str());
    }

    // hud_bridge_survival_fire_* of SHA (B0657): the mockup's camp fire for the card FOGUEIRA (U04); shown and toxic
    // are 0 or 1, the stage one of the five stages, the fuel a share of the fuel the cell was laid with.
    double fire_stage = 0.0;
    if (!read_state(theme, "hud_bridge_survival_fire_shown", hud_bridge_fire_.shown, failed) ||
        !read_state(theme, "hud_bridge_survival_fire_toxic", hud_bridge_fire_.toxic, failed)) {
        return fail_config(failed.c_str());
    }
    if (!read_bridge_in(theme, "hud_bridge_survival_fire_stage", 0.0, SurvivalFireFrame::STAGE_LAST, fire_stage,
                        failed) ||
        !read_bridge_in(theme, "hud_bridge_survival_fire_fuel", 0.0, 1.0, hud_bridge_fire_.fuel, failed) ||
        !read_bridge_in(theme, "hud_bridge_survival_fire_oxygen", 0.0, 1.0, hud_bridge_fire_.oxygen, failed) ||
        fire_stage != static_cast<double>(static_cast<int32_t>(fire_stage))) {
        return fail_config("hud_bridge_survival_fire_*: stage a whole 0..4, fuel and oxygen 0..1");
    }
    hud_bridge_fire_.stage = static_cast<int32_t>(fire_stage);
    // hud_bridge_survival_fire_ash and _burn_left of SHA (E0257): the mockup's ash and burn time while the host
    // offers no ingle.left.v1; neither is negative, and a burn time of 0 is a cell that burns at no rate.
    if (!theme.measure("hud_bridge_survival_fire_ash", hud_bridge_fire_.ash_kg) || hud_bridge_fire_.ash_kg < 0.0 ||
        !theme.measure("hud_bridge_survival_fire_burn_left", hud_bridge_fire_.burn_left) ||
        hud_bridge_fire_.burn_left < 0.0) {
        return fail_config("hud_bridge_survival_fire_ash, _burn_left: 0 or above");
    }
    hud_bridge_fire_.burning = hud_bridge_fire_.burn_left > 0.0;

    // hud_bridge_survival_rest_* of SHA (N0530): the mockup's resting figure for the rest card (U05); both 0 or 1.
    // hud_bridge_survival_air_toxic of SHA (N0530): the mockup's state poison for the air mark (U06); 0 or 1.
    if (!read_state(theme, "hud_bridge_survival_rest_shown", hud_bridge_rest_.shown, failed) ||
        !read_state(theme, "hud_bridge_survival_rest_sleeping", hud_bridge_rest_.sleeping, failed) ||
        !read_state(theme, "hud_bridge_survival_air_toxic", hud_bridge_air_.toxic, failed)) {
        return fail_config(failed.c_str());
    }
    // hud_bridge_survival_food_* of SHA (N0530): the mockup's food of the bag for the spoil ring (U07); shown 0 or 1,
    // the spoil a share 0 to 1.
    if (!read_state(theme, "hud_bridge_survival_food_shown", hud_bridge_food_.shown, failed) ||
        !read_bridge_in(theme, "hud_bridge_survival_food_spoil", 0.0, 1.0, hud_bridge_food_.spoil, failed)) {
        return fail_config(failed.c_str());
    }
    return true;
}

void VivariumView::bridge_body(SurvivalBodyFrame& body) {
    // THE EDGE OF THE BRIDGE FOR N0530: survival.body.v1 of ase-pl-survival (SurvivalHostBodyExptSystem) lays per
    // figure the MTB_* values ase-metabolism writes and the four TAG_MTB_* states HubThrsValSystem writes as 1 or 0
    // (GodotHostResourceManager::read_survival_bodies). While a record of the seat's figure stands, the rail CORPO
    // and the states on the rail SINAIS draw it: a key the record carries counts, a key it lacks is a row the rail
    // does not show. Without the port or the record both stand on the mockup's body (hud_bridge_survival_*,
    // load_hud_bridge_survival) and the gap stays logged, its line naming every field of the record in its order.
    const char* act = "survival.body.v1(effective_temp,temp_stress,hunger,thirst,energy,fatigue,health,poisoned,"
                      "bleeding,emaciated,well_rested)";
    const VivariumSurvivalBodyView* seat = nullptr;
    if (figure_controls_.led_known) {
        for (const VivariumSurvivalBodyView& candidate : resources_.get_survival_bodies()) {
            if (candidate.object_id == figure_controls_.led) {
                seat = &candidate;
                break;
            }
        }
    }
    if (seat == nullptr) {
        open_gap(SurvivalBodyFrame::GAP_BODY, act);
        body = hud_bridge_body_;
        return;
    }
    close_gap(SurvivalBodyFrame::GAP_BODY, act);
    const auto take = [](double value, SurvivalBodyValue& out) {
        out.counts = !std::isnan(value);
        out.value = out.counts ? value : 0.0;
    };
    take(seat->effective_temp, body.temp);
    take(seat->hunger, body.hunger);
    take(seat->thirst, body.thirst);
    take(seat->energy, body.energy);
    take(seat->fatigue, body.fatigue);
    take(seat->health, body.health);
    body.temp_stress = std::isnan(seat->temp_stress) ? 0.0 : seat->temp_stress;
    // A state key the record lacks is a state not judged (its source NOT_FOUND), and the rail shows it not.
    const auto mark = [](double value) { return !std::isnan(value) && value != 0.0; };
    body.poisoned = mark(seat->poisoned);
    body.bleeding = mark(seat->bleeding);
    body.emaciated = mark(seat->emaciated);
    body.well_rested = mark(seat->well_rested);
}

void VivariumView::bridge_fire(SurvivalFireFrame& fire) {
    // THE EDGE OF THE BRIDGE FOR B0657: ingle.fire.v1 of ase-pl-ingle (IngleHostFireExptSystem, Production) lays one
    // record per fire cell of ase-fire, its object the entity of the HUB_FIRE_CELL row, its keys the marks FLAM, CHAR,
    // EMBR and ASH, the fuel left and laid, the heat, the fuel body and the air of ase-atmosphere with its grade
    // ATM_GAS_BTH_STATE (GodotHostResourceManager::read_survival_fires). The card shows the cell laid with the most
    // fuel, the log of the camp; its marks name the stage and its fuel the share LENHA. Without the port or a cell
    // the card stands on the mockup's camp fire (hud_bridge_survival_fire_*, load_hud_bridge_survival) and the gap
    // stays logged, its line naming every key of the record in the record's order.
    const char* act =
        "ingle.fire.v1(flame,charcoal,ember,ash,fuel,fuel_laid,heat,body,oxygen,toxicity,breath_state)";
    const VivariumSurvivalFireView* cell = nullptr;
    for (const VivariumSurvivalFireView& candidate : resources_.get_survival_fires()) {
        if (cell == nullptr || candidate.fuel_laid > cell->fuel_laid) {
            cell = &candidate;
        }
    }
    if (cell == nullptr) {
        open_gap(SurvivalFireFrame::GAP_FIRE, act);
        fire = hud_bridge_fire_;
        return;
    }
    close_gap(SurvivalFireFrame::GAP_FIRE, act);
    // A mark the record lacks is a mark not set. ase-fire keeps a cell in one stage at a time; the latest mark the
    // cell carries names it.
    const auto mark = [](double value) { return !std::isnan(value) && value != 0.0; };
    fire.shown = true;
    fire.stage = mark(cell->ash)        ? SurvivalFireFrame::STAGE_LAST
                 : mark(cell->ember)    ? SurvivalFireFrame::STAGE_EMBER
                 : mark(cell->charcoal) ? SurvivalFireFrame::STAGE_CHARCOAL
                 : mark(cell->flame)    ? SurvivalFireFrame::STAGE_FLAME
                                        : SurvivalFireFrame::STAGE_IGNITE;
    // THE FUEL OF THE CARD (N0536): the remaining fuel is state of the burning body, kept by ase-fire on its fire
    // cell (HUB_FIRE_CELL_FUEL and HUB_FIRE_CELL_FUEL_LAID, one row per fuel body, PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP
    // "Wärme am Ort des gelandeten Brennstoffs" point 1) and laid per cell on ingle.fire.v1. What the fire does to the
    // place is its heat, which ase-fire writes into the ase-gis raster cell by cell; the card reads the body, not a cell.
    const bool weighed = !std::isnan(cell->fuel) && !std::isnan(cell->fuel_laid) && cell->fuel_laid > 0.0;
    fire.fuel = weighed ? cell->fuel / cell->fuel_laid : 0.0;
    // WHAT THE FIRE LEAVES AND HOW LONG IT LASTS (E0257): ingle.left.v1 of ase-pl-ingle (IngleHostLeftExptSystem)
    // lays the kg of ash ase-fire files for the cell (HUB_ENT_ASH_REQ_ASH) and the seconds its fuel lasts at the burn
    // rate of this tick (HUB_FIRE_CELL_BURN_LEFT), joined onto the cell by GodotHostResourceManager::read_survival_left.
    // While the host offers the port, a missing ash_kg is a cell ase-fire has booked no ash for yet, and a missing
    // burn_left a cell that heats or is ash; without the port both stand on the mockup's fire and the gap stays logged.
    const char* left = "ingle.left.v1(ash_kg,burn_left)";
    if (resources_.survival_left_offered()) {
        close_gap(SurvivalFireFrame::GAP_LEFT, left);
        fire.ash_kg = std::isnan(cell->ash_kg) ? 0.0 : cell->ash_kg;
        fire.burning = !std::isnan(cell->burn_left);
        fire.burn_left = fire.burning ? cell->burn_left : 0.0;
    } else {
        open_gap(SurvivalFireFrame::GAP_LEFT, left);
        fire.ash_kg = hud_bridge_fire_.ash_kg;
        fire.burning = hud_bridge_fire_.burning;
        fire.burn_left = hud_bridge_fire_.burn_left;
    }
    // THE SMOKE OF THE CARD (B0662): the owner's grade breath_state over the grade it gives normal air. While the
    // record lacks one of the two, the smoke stands on the mockup's air and the gap stays logged with both keys.
    // THE AIR STREAM: the oxygen of the air the fire draws, ATM_OXYGEN_FACTOR as ase-atmosphere publishes it and
    // IngleHostFireExptSystem lays it with each fire cell; the card fades its air stream with it. The plugin leaves
    // the key out while the owner has not published it, like both breath keys, so the three share one gap.
    const char* graded = "ingle.fire.v1(breath_state,breath_normal,oxygen)";
    const bool breathed = !std::isnan(cell->oxygen);
    fire.oxygen = breathed ? cell->oxygen : hud_bridge_fire_.oxygen;
    bool toxic = false;
    if (!graded_toxic(*cell, toxic)) {
        open_gap(SurvivalFireFrame::GAP_MARKS, graded);
        fire.toxic = hud_bridge_fire_.toxic;
        return;
    }
    if (!breathed) {
        open_gap(SurvivalFireFrame::GAP_MARKS, graded);
        fire.toxic = toxic;
        return;
    }
    close_gap(SurvivalFireFrame::GAP_MARKS, graded);
    fire.toxic = toxic;
}

void VivariumView::bridge_air(SurvivalAirFrame& air) {
    // THE EDGE OF THE BRIDGE FOR N0532: the air the seat's figure breathes at its own place. ase-atmosphere grades the
    // air (ATM_GAS_BTH_STATE), GisLyrAtmoBthRstSystem of ase-gis lays it into the breathability layer GIS_CAT_ATMO_BTH
    // as its share of the way from normal air to no air, GisRstrEnrcSystem samples the cell under the figure as
    // GIS_LOCAL_BREATH, and survival.body.v1 lays it as breath (SurvivalHostBodyExptSystem,
    // GodotHostResourceManager::read_survival_bodies). The share is 0 at normal or better air, so the mark shows
    // poison exactly while the share stands above 0: the boundary is the writer's zero, never a threshold of the
    // adapter. Without the record of the seat's figure or its key (no breath raster in the figure's chunk) the mark
    // stands on the mockup's state poison (hud_bridge_survival_air_toxic, load_hud_bridge_survival) under its gap.
    const char* act = "survival.body.v1(breath from GIS_LOCAL_BREATH)";
    const VivariumSurvivalBodyView* seat = nullptr;
    if (figure_controls_.led_known) {
        for (const VivariumSurvivalBodyView& candidate : resources_.get_survival_bodies()) {
            if (candidate.object_id == figure_controls_.led) {
                seat = &candidate;
                break;
            }
        }
    }
    if (seat == nullptr || std::isnan(seat->breath)) {
        open_gap(SurvivalAirFrame::GAP_AIR, act);
        air = hud_bridge_air_;
    } else {
        close_gap(SurvivalAirFrame::GAP_AIR, act);
        air.toxic = seat->breath > 0.0;
    }
    // THE EDGE OF THE BRIDGE FOR D2500: the soot in the air of the seat's own cell, the facet Rauch of
    // PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP. ase-fire lays the soot of every fire cell on the raster cell of its fuel
    // body (FireSmokCellCalcSystem), ase-gis sums it per cell under each figure as GIS_LOCAL_SOOT
    // (GisLyrNthrSmokCellSystem), survival.smoke.v1 lays it as soot (SurvivalHostSmokExptSystem,
    // GodotHostResourceManager::read_survival_smokes). The plume stands while the value stands above 0, the writer's
    // zero. Without the port, the record of the seat's figure or its key the frame carries 0 soot under its gap, so
    // the mark stays on the grade of the air alone; the mockup draws no soot of its own.
    const char* smoked = "survival.smoke.v1(soot from GIS_LOCAL_SOOT)";
    const VivariumSurvivalSmokeView* cell = nullptr;
    if (resources_.survival_smoke_offered() && figure_controls_.led_known) {
        for (const VivariumSurvivalSmokeView& candidate : resources_.get_survival_smokes()) {
            if (candidate.object_id == figure_controls_.led) {
                cell = &candidate;
                break;
            }
        }
    }
    if (cell == nullptr || std::isnan(cell->soot)) {
        open_gap(SurvivalAirFrame::GAP_SOOT, smoked);
        air.soot = 0.0;
        return;
    }
    close_gap(SurvivalAirFrame::GAP_SOOT, smoked);
    air.soot = cell->soot;
}

void VivariumView::bridge_food(SurvivalSpoilFrame& food) {
    // THE EDGE OF THE BRIDGE FOR N0530 (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.8): ase-equipment lays every piece of
    // food into the bag of its bearer (EquipmentHubFoodSyncSystem from HUB_EQP_FOOD_REQ), ase-entropy rots it by the
    // temperature it lies at (EntropyFoodDcySystem) and lays the mass-weighted share of the bearer as
    // ENT_FOOD_ROT_CARRIED (EntropyFoodHubWrtSystem), and survival.food.v1 lays that share as spoil, one record per
    // figure that carries food (SurvivalHostFoodExptSystem, GodotHostResourceManager::read_survival_foods). While the
    // port stands, the ring shows the share of the seat's figure; a figure without a record carries no food, and the
    // ring is hidden as the mockup hides it for a bag without food (st.food null). Without the port, or with a record
    // that misses its key, the ring stands on the mockup's food (hud_bridge_survival_food_*, load_hud_bridge_survival)
    // under its gap.
    const char* act = "survival.food.v1(spoil from ENT_FOOD_ROT_CARRIED)";
    if (!resources_.survival_food_offered()) {
        open_gap(SurvivalSpoilFrame::GAP_SPOIL, act);
        food = hud_bridge_food_;
        return;
    }
    const VivariumSurvivalFoodView* seat = nullptr;
    if (figure_controls_.led_known) {
        for (const VivariumSurvivalFoodView& candidate : resources_.get_survival_foods()) {
            if (candidate.object_id == figure_controls_.led) {
                seat = &candidate;
                break;
            }
        }
    }
    if (seat != nullptr && std::isnan(seat->spoil)) {
        open_gap(SurvivalSpoilFrame::GAP_SPOIL, act);
        food = hud_bridge_food_;
        return;
    }
    close_gap(SurvivalSpoilFrame::GAP_SPOIL, act);
    food.shown = seat != nullptr;
    food.spoil = seat != nullptr ? seat->spoil : 0.0;
}

void VivariumView::bridge_rest(SurvivalRestFrame& rest) {
    // THE EDGE OF THE BRIDGE FOR N0530: ase-metabolism writes MTB_IS_RESTING per figure (MetabolismRstSystem derives
    // MetabolismRstgTag from BDI_CURRENT_ACTION in Regulation) and survival.rest.v1 lays it as is_resting
    // (SurvivalHostRstExptSystem, GodotHostResourceManager::read_survival_rests). While the record of the seat's
    // figure carries it, the card shows exactly while the owner says the figure rests; without the record or the key
    // it stands on the mockup's resting figure (hud_bridge_survival_rest_*, load_hud_bridge_survival) under its gap.
    //
    // Whether the resting figure sits or sleeps is the action ase-bdi writes as BDI_CURRENT_ACTION: its body plan
    // (BdiIniPlnBodySystem) lays the sleep under the star constant BDI_CONST_ACTION_SLEEP beside BDI_CONST_ACTION_WAIT,
    // MetabolismRstSystem counts both as rest and writes MTB_IS_SLEEPING, and survival.rest.v1 lays it as sleeping
    // (N0555). While the record carries it, the card shows DORMINDO exactly while the owner says the figure sleeps;
    // without the key the pose stays the mockup's, its gap logged on its own line.
    const char* act_rest = "survival.rest.v1(is_resting)";
    const char* act_sleep = "survival.rest.v1(sleeping from MTB_IS_SLEEPING)";
    rest = hud_bridge_rest_;
    const VivariumSurvivalRestView* seat = nullptr;
    if (figure_controls_.led_known) {
        for (const VivariumSurvivalRestView& candidate : resources_.get_survival_rests()) {
            if (candidate.object_id == figure_controls_.led) {
                seat = &candidate;
                break;
            }
        }
    }
    if (seat == nullptr || std::isnan(seat->is_resting)) {
        open_gap(SurvivalRestFrame::GAP_REST, act_rest);
    } else {
        close_gap(SurvivalRestFrame::GAP_REST, act_rest);
        rest.shown = seat->is_resting != 0.0;
    }
    if (seat == nullptr || std::isnan(seat->sleeping)) {
        open_gap(SurvivalRestFrame::GAP_REST, act_sleep);
    } else {
        close_gap(SurvivalRestFrame::GAP_REST, act_sleep);
        rest.sleeping = seat->sleeping != 0.0;
    }
}

}  // namespace ase::adp::godot
