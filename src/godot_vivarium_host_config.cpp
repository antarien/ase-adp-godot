/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HOST CONFIGURATION
 *
 * @file        godot_vivarium_host_config.cpp
 * @brief       VivariumView - the ports and operations the game host is booted with, land in host_land
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 and PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8: everything
 *              GodotHostResourceManager::boot needs beside the stage stands in the start configuration -
 *              the name of every port and operation the view reads or writes, the patches the game host
 *              lays, the measured square of the ground and its brooks, and the pour.
 *              A missing or broken key stops the view with that key on its error panel; the manager
 *              checks the same configuration again before a host exists.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-10-18
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_vivarium_camp.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/variant/array.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

bool VivariumView::load_host_config(const ::godot::Dictionary& root) {
    ::godot::String text;
    ::godot::Dictionary ports;
    if (!read_section(root, "ports", ports)) {
        return fail_config("ports");
    }
    if (!read_text(ports, "patch", text)) {
        return fail_config("ports.patch");
    }
    start_.patch_port = utf8_of(text);
    // The stage forecasts of ase-plant (usecase:101120 clock the growth cycle): a port of their own beside
    // the patch port, whose record holds no further value; one record per patch with a forecast.
    if (!read_text(ports, "forecast", text)) {
        return fail_config("ports.forecast");
    }
    start_.forecast_port = utf8_of(text);
    // The soil water of ase-hydrology (usecase:101340 - show the bed condition): a port of its own beside the
    // patch port, whose record holds no further value; one record per patch whose soil row stands.
    if (!read_text(ports, "soil", text)) {
        return fail_config("ports.soil");
    }
    start_.soil_port = utf8_of(text);
    // The condition codes of ase-plant (usecase:101340 - show the bed condition): one record with the healthy,
    // the wilted and the dead code, so the view compares the condition of a patch against them and holds no
    // number of ase-plant of its own (D2454).
    if (!read_text(ports, "cond", text)) {
        return fail_config("ports.cond");
    }
    start_.cond_port = utf8_of(text);
    // The stage codes of ase-plant (usecase:101635 - show the plant stage): one record with the seed, the sprout,
    // the grow and the mature code, so the view turns the stage of a patch into its position among them and holds
    // no number of ase-plant of its own (D2455).
    if (!read_text(ports, "stage", text)) {
        return fail_config("ports.stage");
    }
    start_.stage_port = utf8_of(text);
    // The stem feet ase-physics holds (usecase:100708 land the dispersed seeds, N0487): a port of their own,
    // one record per patch whose stem stands, its value the height of the foot over the level of the patch.
    if (!read_text(ports, "stem", text)) {
        return fail_config("ports.stem");
    }
    start_.stem_port = utf8_of(text);
    // The autumn colours of the individuals (usecase:101646 colour the leaves in autumn): a port of their own, one
    // record per patch whose colour ase-phytomer measured, its value the share of the way to its own leaf fall.
    if (!read_text(ports, "autumn", text)) {
        return fail_config("ports.autumn");
    }
    start_.autumn_port = utf8_of(text);
    // The cap radii of the fruit bodies (usecase:101362 scale the organ templates): a port of their own, one record
    // per patch whose fruit body has a cap, its value the cap radius ase-phytomer passed on for its individual.
    if (!read_text(ports, "cap", text)) {
        return fail_config("ports.cap");
    }
    start_.cap_port = utf8_of(text);
    if (!read_text(ports, "clock", text)) {
        return fail_config("ports.clock");
    }
    start_.clock_port = utf8_of(text);
    if (!read_text(ports, "scene", text)) {
        return fail_config("ports.scene");
    }
    start_.scene_port = utf8_of(text);
    // The ground of the scene (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8): the square, the brooks and the
    // measured heights, one port each.
    if (!read_text(ports, "ground", text)) {
        return fail_config("ports.ground");
    }
    start_.ground_port = utf8_of(text);
    if (!read_text(ports, "brook", text)) {
        return fail_config("ports.brook");
    }
    start_.brook_port = utf8_of(text);
    if (!read_text(ports, "terrain", text)) {
        return fail_config("ports.terrain");
    }
    start_.terrain_port = utf8_of(text);
    // The cells of the ground (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2): ase-render-data names the
    // four corners of every grid square; the view hands them to the index buffer as they are.
    if (!read_text(ports, "ground_cell", text)) {
        return fail_config("ports.ground_cell");
    }
    start_.ground_cell_port = utf8_of(text);
    // The organ templates (usecase:101011 tessellate the organ templates): ase-render-data lays the corners and the
    // triangles of the fine and the coarse cylinder and of the unit leaf; the view builds its meshes from
    // them and computes no ring, no normal and no winding of its own.
    if (!read_text(ports, "template_corner", text)) {
        return fail_config("ports.template_corner");
    }
    start_.template_corner_port = utf8_of(text);
    if (!read_text(ports, "template_triangle", text)) {
        return fail_config("ports.template_triangle");
    }
    start_.template_triangle_port = utf8_of(text);
    // The tangent of every template corner (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.6): a port of its own,
    // since the corner record is full; the view joins it to the corner and computes no tangent.
    if (!read_text(ports, "template_tangent", text)) {
        return fail_config("ports.template_tangent");
    }
    start_.template_tangent_port = utf8_of(text);
    // The sky and the light of the scene (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3): the one port
    // of ase-pl-sky; the view lights the care scene from it and computes no light of its own.
    if (!read_text(ports, "sky", text)) {
        return fail_config("ports.sky");
    }
    start_.sky_port = utf8_of(text);
    // The visible star set of ase-pl-sky at its places on the celestial sphere: its own port, read only
    // when the turn record of the sky port names a set computed anew; the view turns it every frame.
    if (!read_text(ports, "stars", text)) {
        return fail_config("ports.stars");
    }
    start_.stars_port = utf8_of(text);
    // The seed bodies of ase-physics (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 4, register N0173): the
    // view draws every seed where the port says and computes none of its flight.
    if (!read_text(ports, "seed", text)) {
        return fail_config("ports.seed");
    }
    start_.seed_port = utf8_of(text);
    // The tool rack of ase-pl-toolrack (usecase:101625 carry a tool in hand): the view lays the rack of the
    // patch through the scene port, ase-physics drops it as a prop body, and the view draws the rack where
    // the rack port says and computes none of its height.
    if (!read_text(ports, "toolrack_scene", text)) {
        return fail_config("ports.toolrack_scene");
    }
    start_.toolrack_scene_port = utf8_of(text);
    if (!read_text(ports, "toolrack_rack", text)) {
        return fail_config("ports.toolrack_rack");
    }
    start_.toolrack_rack_port = utf8_of(text);
    // The bark tiling of the internodes (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2, the material data of
    // a section): ase-render-data computed it, the view scales the bark texture by it and computes none.
    if (!read_text(ports, "tile", text)) {
        return fail_config("ports.tile");
    }
    start_.tile_port = utf8_of(text);
    // The lineage of every shown organ (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.9, the round trip S0, S1, S0):
    // ase-phytomer gave it, the view names each organ by it across a release and a new resolution, computes none.
    if (!read_text(ports, "lineage", text)) {
        return fail_config("ports.lineage");
    }
    start_.lineage_port = utf8_of(text);
    // The sway of the organs on a branch (usecase:101335 animate the branch sway): ase-phytomer computed it, the view
    // swings the instances by it in their shaders and computes none of its values.
    if (!read_text(ports, "sway", text)) {
        return fail_config("ports.sway");
    }
    start_.sway_port = utf8_of(text);
    // The bend of every leaf card (PLAN_ASE_VIVARIUM_TREEGEN T.6, Gitter mit Biegung): ase-phytomer carries the bend
    // of the species, ase-render-data lays it on the leaf; the vertex shader lifts the leaf lattice by it and
    // computes none.
    if (!read_text(ports, "bend", text)) {
        return fail_config("ports.bend");
    }
    start_.bend_port = utf8_of(text);
    // The colour offset of every leaf card (PLAN_ASE_VIVARIUM_TREEGEN T.6, Farbe, Variante, Herbst): ase-phytomer
    // carries the tint spread of the species, ase-render-data draws the offset of each leaf by its lineage; the
    // fragment shader lays 1 + offset on the albedo and draws none.
    if (!read_text(ports, "tint", text)) {
        return fail_config("ports.tint");
    }
    start_.tint_port = utf8_of(text);
    // The atlas cell of every leaf card (PLAN_ASE_VIVARIUM_TREEGEN T.6, Farbe, Variante, Herbst): ase-phytomer carries
    // the atlas count of the species, ase-render-data draws the cell of each leaf by its lineage; the fragment shader
    // samples the texture at (u + cell) / cells and draws none.
    if (!read_text(ports, "atlas", text)) {
        return fail_config("ports.atlas");
    }
    start_.atlas_port = utf8_of(text);
    // The bark tube of every axis (register B0295 point 4, usecase:101613 show the plant bark tubes): ase-render-data
    // built it with frames carried by parallel transport, the view draws its vertices and computes no geometry.
    if (!read_text(ports, "bark", text)) {
        return fail_config("ports.bark");
    }
    start_.bark_port = utf8_of(text);
    // The knots, the feet and the cut faces of the bark tubes (PLAN_ASE_VIVARIUM_TREEGEN T.6, register N0543):
    // ase-pl-flora files one knot per internode, one foot per axis and one face per cut, page by page; the view lays
    // the tubes through them and computes no frame and no radius.
    if (!read_text(ports, "knot", text)) {
        return fail_config("ports.knot");
    }
    start_.knot_port = utf8_of(text);
    if (!read_text(ports, "foot", text)) {
        return fail_config("ports.foot");
    }
    start_.foot_port = utf8_of(text);
    if (!read_text(ports, "scar", text)) {
        return fail_config("ports.scar");
    }
    start_.scar_port = utf8_of(text);
    // The water of the brooks (usecase:100399 carry brook water): ase-hydro computed it, the view lays it into the
    // bed and computes none of its values.
    if (!read_text(ports, "stream", text)) {
        return fail_config("ports.stream");
    }
    start_.stream_port = utf8_of(text);
    // The water of every kind beside the brooks (PLAN_ASE_VIVARIUM_PHASE_10_HYDRO): the river current of ase-pl-water,
    // the pit water and the groundwater of ase-pl-hydrogeology. ase-hydro computes all three and ase-gis answers
    // them per cell; the plugins only carry them, and the view reads each port once its plugin offers it.
    if (!read_text(ports, "current", text)) {
        return fail_config("ports.current");
    }
    start_.current_port = utf8_of(text);
    // What the water of each current cell carries (D2508): the algae, the rot and the salt, joined to the current
    // cell of the same row; the lake and the sea shaders read them.
    if (!read_text(ports, "quality", text)) {
        return fail_config("ports.quality");
    }
    start_.quality_port = utf8_of(text);
    if (!read_text(ports, "pit", text)) {
        return fail_config("ports.pit");
    }
    start_.pit_port = utf8_of(text);
    if (!read_text(ports, "groundwater", text)) {
        return fail_config("ports.groundwater");
    }
    start_.groundwater_port = utf8_of(text);
    // The salt of every chunk (H.9, I-4): the water half of ase-hydro, the soil half of ase-pedosphere and the sum
    // ase-gis adds; ase-pl-hydrogeology carries all three and the debug status line of the HUD shows their world sums.
    if (!read_text(ports, "salt", text)) {
        return fail_config("ports.salt");
    }
    start_.salt_port = utf8_of(text);
    // The level requests of the player (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8 point 2): a port of their
    // own, the patch port takes only pours.
    if (!read_text(ports, "level", text)) {
        return fail_config("ports.level");
    }
    start_.level_port = utf8_of(text);
    // The flatten requests of the player at a place (03.8): a port of their own, they name no patch.
    if (!read_text(ports, "flatten", text)) {
        return fail_config("ports.flatten");
    }
    start_.flatten_port = utf8_of(text);
    // The spade strokes of the player at a place: one port per direction, so the direction is never a branch.
    if (!read_text(ports, "lower", text)) {
        return fail_config("ports.lower");
    }
    start_.lower_port = utf8_of(text);
    if (!read_text(ports, "raise", text)) {
        return fail_config("ports.raise");
    }
    start_.raise_port = utf8_of(text);
    // The hoe strokes at a place (soltar, furar; B0649): a port of their own, they loosen the topsoil and move
    // no height.
    if (!read_text(ports, "loosen", text)) {
        return fail_config("ports.loosen");
    }
    start_.loosen_port = utf8_of(text);
    // The pace requests of the debug mode (the time lapse of this host's clock): a port of their own,
    // they name no object.
    if (!read_text(ports, "pace", text)) {
        return fail_config("ports.pace");
    }
    start_.pace_port = utf8_of(text);
    // The seek requests of the debug mode (this host's clock moved to a computed moment): a port of
    // their own beside the time lapse, they name no object.
    if (!read_text(ports, "seek", text)) {
        return fail_config("ports.seek");
    }
    start_.seek_port = utf8_of(text);
    // The real position of the host (its real seconds since the start, sleep included), filed every
    // frame: a port of its own, it names no object; a return from the background is one such filing.
    if (!read_text(ports, "real", text)) {
        return fail_config("ports.real");
    }
    start_.real_port = utf8_of(text);
    // The date requests of the debug mode (this host's clock moved to a SURA date): a port of their own
    // beside the seek, they name no object; ase-calendar turns the date into the hour.
    if (!read_text(ports, "date", text)) {
        return fail_config("ports.date");
    }
    start_.date_port = utf8_of(text);
    // The day record (N0460): the day's sunrise, sunset and daylight, the season and the shares of the day, the sun
    // and the year, a port of its own because the clock and the date record are full; the sheet reads it once
    // ase-pl-flora offers it.
    if (!read_text(ports, "day", text)) {
        return fail_config("ports.day");
    }
    start_.day_port = utf8_of(text);
    // The phase records (N0460): one per day period with its start hour and share, entity per item.
    if (!read_text(ports, "phase", text)) {
        return fail_config("ports.phase");
    }
    start_.phase_port = utf8_of(text);
    // The season records (N0460): one per season with its first LUN and hours of light, entity per item.
    if (!read_text(ports, "season", text)) {
        return fail_config("ports.season");
    }
    start_.season_port = utf8_of(text);
    // The weather of the sky (N0449, usecase:5606 observe the sky): the current record and one forecast record per
    // coming period on one port, ase-atmosphere's codes of the kinds on a port of their own; the HUD reads both
    // once ase-pl-flora offers them.
    if (!read_text(ports, "weather", text)) {
        return fail_config("ports.weather");
    }
    start_.weather_port = utf8_of(text);
    if (!read_text(ports, "weather_code", text)) {
        return fail_config("ports.weather_code");
    }
    start_.weather_code_port = utf8_of(text);
    // The tap in the birch and its bark vessel (N0457): one record with running, held_l and cap_l; the HUD reads it
    // once ase-pl-flora offers the port.
    if (!read_text(ports, "sap", text)) {
        return fail_config("ports.sap");
    }
    start_.sap_port = utf8_of(text);
    // The looseness raster of the soil (N0456, CAUSA_ASE_TERRAIN loosened soil): one record per macro cell with x, z,
    // cell_m and cpct_share 0..1, the compaction ase-terrain owns (TerrainStaMcroCpctComponent, ase-gis
    // GIS_CAT_LITHO_CPCT), filed by FloraHostCpctExptSystem of ase-pl-flora; the looseness is its complement.
    // GodotHostResourceManager::read_loose reads it; until the port is offered the
    // HUD's loose bridge keeps the share of SHA.
    if (!read_text(ports, "loose", text)) {
        return fail_config("ports.loose");
    }
    start_.loose_port = utf8_of(text);
    // The relief of the ground (the height amplitude and the ground level of the whole terrain): a port
    // of its own, it names no object.
    if (!read_text(ports, "elevation", text)) {
        return fail_config("ports.elevation");
    }
    start_.elevation_port = utf8_of(text);
    // The place of the view's camera (03.11 point 7, the level of detail): a port of its own, it names no
    // object.
    if (!read_text(ports, "view", text)) {
        return fail_config("ports.view");
    }
    start_.view_port = utf8_of(text);
    // One port per organ class, keyed by the class's word (GodotHostResourceManager::organ_word).
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        if (!read_text(ports, GodotHostResourceManager::organ_word(c), text)) {
            return fail_config("ports.<organ>");
        }
        start_.organ_ports[c] = utf8_of(text);
    }
    // One instance port per drawn class, keyed by the class's word and GODOT_INSTANCE_SUFFIX (03.11 step 2).
    for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
        const std::string key = std::string(GodotHostResourceManager::organ_word(c)) + GODOT_INSTANCE_SUFFIX;
        if (!read_text(ports, key.c_str(), text)) {
            return fail_config("ports.<organ>_instance");
        }
        start_.instance_ports[c] = utf8_of(text);
    }

    ::godot::Dictionary operations;
    if (!read_section(root, "operations", operations)) {
        return fail_config("operations");
    }
    if (!read_text(operations, "irrigate", text)) {
        return fail_config("operations.irrigate");
    }
    start_.op_irrigate = utf8_of(text);
    if (!read_text(operations, "prune", text)) {
        return fail_config("operations.prune");
    }
    start_.op_prune = utf8_of(text);
    if (!read_text(operations, "level", text)) {
        return fail_config("operations.level");
    }
    start_.op_level = utf8_of(text);
    if (!read_text(operations, "flatten", text)) {
        return fail_config("operations.flatten");
    }
    start_.op_flatten = utf8_of(text);
    if (!read_text(operations, "lower", text)) {
        return fail_config("operations.lower");
    }
    start_.op_lower = utf8_of(text);
    if (!read_text(operations, "raise", text)) {
        return fail_config("operations.raise");
    }
    start_.op_raise = utf8_of(text);
    if (!read_text(operations, "loosen", text)) {
        return fail_config("operations.loosen");
    }
    start_.op_loosen = utf8_of(text);
    if (!read_text(operations, "pace", text)) {
        return fail_config("operations.pace");
    }
    start_.op_pace = utf8_of(text);
    if (!read_text(operations, "seek", text)) {
        return fail_config("operations.seek");
    }
    start_.op_seek = utf8_of(text);
    if (!read_text(operations, "seek_date", text)) {
        return fail_config("operations.seek_date");
    }
    start_.op_seek_date = utf8_of(text);
    if (!read_text(operations, "real", text)) {
        return fail_config("operations.real");
    }
    start_.op_real = utf8_of(text);
    if (!read_text(operations, "shape", text)) {
        return fail_config("operations.shape");
    }
    start_.op_shape = utf8_of(text);
    if (!read_text(operations, "view", text)) {
        return fail_config("operations.view");
    }
    start_.op_view = utf8_of(text);
    if (!read_text(operations, "lay", text)) {
        return fail_config("operations.lay");
    }
    start_.op_lay = utf8_of(text);
    if (!load_patches(root)) {
        return false;  // the failing key stands on the error panel already
    }
    if (!load_ground(root)) {
        return false;
    }
    if (!load_brooks(root)) {
        return false;
    }
    if (!load_figure_config(root)) {
        return false;  // the figure's ports, operations and place (godot_vivarium_figure.cpp)
    }
    const char* camp_missing = read_camp_config(root, start_);  // ase-pl-ingle's ports, the camp place and its parts (T.0, T.3)
    if (camp_missing != nullptr) {
        return fail_config(camp_missing);
    }

    if (!read_number(root, "irrigate_amount_mm", start_.irrigate_amount_mm) ||
        !(start_.irrigate_amount_mm > 0.0)) {
        return fail_config("irrigate_amount_mm");
    }
    return true;
}

}  // namespace ase::adp::godot
