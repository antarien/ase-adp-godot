#pragma once

/**
 * ASE GODOT ADAPTER - HOST START CONFIGURATION
 *
 * @file        godot_host_start_config.hpp
 * @brief       The start configuration of the embedded host: the stage reader, the ports, the operations and the patches
 * @description What GodotHostResourceManager::boot() and self_test() need, filled by the node from
 *              res://config/vivarium_start.json and from the stage Godot loaded this library from: the reader of
 *              the staged bundle, the host ports and submit operations of every owner the view talks to, the
 *              seat of the figure, and the patches, the ground square and the brooks the game host lays. One
 *              purpose of the dissolved godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header
 *              ratchet): only the boot path and the units that build a configuration read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-21
 * @modified    2026-11-23
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/port/types.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/**
 * Reads one file of the staged bundle by its name (the bundle index, a manifest it names) into
 * `out`; false when the stage holds no such file. The node reads through Godot's FileAccess, so
 * the stage may lie inside the exported package (Android); a test reads the build's bundle
 * directory. The manager itself never opens a stage file.
 */
using VivariumStageReadFn = bool (*)(const char* name, std::string& out, void* user);

/**
 * One patch of the start configuration (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS, section on patch and species): the
 * game host asks ase-pl-flora for it with one lay request on the scene port after its start.
 */
struct VivariumPatchSpec {
    uint32_t    place = 0;     // where the view draws it; every place stands once
    std::string species;       // catalog id of the species sown on it (modules/ase-plant/data/plant_species.json)
    uint32_t    seed = 0;      // seed of the plant: the same seed grows the same plant
    uint8_t     soil = 0;      // GODOT_SOIL_* ground the view draws
    double      x = 0.0;       // centre of the patch, world metres east; ase-terrain levels the ground there
    double      z = 0.0;       // centre of the patch, world metres north
    double      half_m = 0.0;  // half edge of the patch square, metres; the levelled disc has this radius
    double      capacity_mm = 0.0;  // field capacity of its soil, mm; ase-pl-flora files it at ase-hydro
    double      gravity_rel = 0.0;  // gravity of the patch relative to g; ase-pl-flora files it at ase-physics
    double      age_days = 0.0;     // Startstand: game days since germination at the world start, 0 for a seed; ase-plant derives the stage from it (usecase:100390)
};

/**
 * The measured square of terrain the view draws as the ground of the scene (PLAN_ASE_VIVARIUM_PHASE_03_
 * ASSETS 03.8): one lay request on the ground port of ase-pl-flora, which orders the samples at
 * ase-terrain.
 */
struct VivariumGroundSpec {
    double x = 0.0;       // centre of the square, world metres east
    double z = 0.0;       // centre of the square, world metres north
    double half_m = 0.0;  // half edge of the square, metres
    double step_m = 0.0;  // distance of two samples, metres
    double height_scale_m = 0.0;  // height amplitude of the relief, metres: one shape request on the elevation port (TER_GEN_HGT_SCL)
    double base_height_m = 0.0;   // ground level of the relief above NN0, metres: the same request (TER_GEN_BAS_HGT)
};

/**
 * One brook of the start configuration: one lay request on the brook port of ase-pl-flora, which has
 * ase-terrain carve its bed and registers its water with ase-hydro.
 */
struct VivariumBrookSpec {
    double x = 0.0;             // start of the bed, world metres east
    double z = 0.0;             // start of the bed, world metres north
    double end_x = 0.0;         // end of the bed, world metres east
    double end_z = 0.0;         // end of the bed, world metres north
    double half_m = 0.0;        // half width of the bed, metres
    double depth_m = 0.0;       // depth of the bed below the ground, metres
    double base_m3_s = 0.0;     // base flow without rain, m3/s; ase-hydro holds its bounds
    double catchment_m2 = 0.0;  // area whose rain runs into the brook, m2
};

/**
 * One piece of the start stock of the figure's bag (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.8, usecase:5378 - regulate
 * basic needs): once the figure of the seat stands, one stock request on survival.stock.v1, which ase-pl-survival files
 * as HUB_EQP_FOOD_REQ; ase-equipment lays the piece into the pouch and ase-entropy spoils it.
 */
struct VivariumStockSpec {
    std::string item;   // item_art.json key of the food; the request carries its hash as the kind
    double      kg = 0.0;  // fresh mass of the piece, kg
};

/**
 * One part of the camp fire (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.3, usecase:5436 - kindle fire): a stone of the
 * ring, a log, a stick or the tinder, each one rigid body of ase-physics. role names the export in
 * vivarium_assets.json and variant its node; mass, make and hull come from that node's extras, never from here.
 */
struct VivariumCampPartSpec {
    std::string role;           // role of the asset list whose export holds the part (camp.stone, camp.log, ...)
    uint32_t    variant = 0;    // node of the export, the suffix of its make (camp_stone.<variant>)
    double      east = 0.0;     // metres east of the stem foot of the patch of camp.place
    double      north = 0.0;    // metres north of that stem foot
    double      drop = 0.0;     // height of the centre of mass the part is laid at, metres above that stem foot
    double      tx = 0.0;       // turn the part is laid in, rotation vector in radians, east component
    double      ty = 0.0;       // turn the part is laid in, up component
    double      tz = 0.0;       // turn the part is laid in, north component
    double      fall = 0.0;     // fall speed of the part in still air, m/s
    bool        fuel = false;   // the part burns: ase-pl-ingle gives it a fire cell of ase-fire
};

/**
 * Everything boot() and self_test() need. The node fills it from res://config/vivarium_start.json
 * and from the stage Godot loaded this library from; no value of it lives in a unit.
 */
struct VivariumStartConfig {
    VivariumStageReadFn read_stage = nullptr;  // REQUIRED: reads the bundle index and the manifests
    void*       read_stage_user = nullptr;     // handed back to read_stage unchanged
    std::string library_dir;         // ABSOLUTE directory the unit libraries lie in; EMPTY: every
                                     // library is opened by its file name and the platform linker
                                     // resolves it in the app's own namespace (Android, PLAN 02.1)
    std::string files_dir;           // ABSOLUTE directory of the stage on this device's file system:
                                     // the bundle's data root lies below it (the stage itself on a
                                     // desktop, the directory its data files are placed in where the
                                     // stage is a package)
    std::string log_file;            // ABSOLUTE log file under user://, empty = callback only
    bool        start_fresh = false; // the start screen chose "offline, from the initial state": the
                                     // host starts without the last state of the owners
                                     // (KernelEmbeddedHost::set_start_fresh); false = resume from it
    std::string patch_port;          // host port of the patches (ase-pl-flora: flora.patch.v1)
    std::string forecast_port;       // host port of the stage forecasts, one record per forecast patch (ase-pl-flora: flora.forecast.v1)
    std::string soil_port;           // host port of the soil water, one record per patch whose soil row stands (ase-pl-flora: flora.soil.v1, usecase:101340 - show the bed condition)
    std::string cond_port;           // host port of the condition codes of ase-plant, one record (ase-pl-flora: flora.cond.v1, usecase:101340 - show the bed condition)
    std::string stage_port;          // host port of the stage codes of ase-plant, one record (ase-pl-flora: flora.stage.v1, usecase:101635 - show the plant stage)
    std::string stem_port;           // host port of the stem feet ase-physics holds, one record per standing stem (ase-pl-flora: flora.stem.v1)
    std::string autumn_port;         // host port of the autumn colours, one record per patch whose colour stands (ase-pl-flora: flora.autumn.v1, usecase:101646 colour the leaves in autumn)
    std::string cap_port;            // host port of the cap radii, one record per patch whose fruit body has a cap (ase-pl-flora: flora.cap.v1, usecase:101362 scale the organ templates)
    std::string clock_port;          // host port of the game clock (ase-pl-flora: flora.clock.v1)
    std::string scene_port;          // host port the patches are laid through (ase-pl-flora: flora.scene.v1)
    std::string organ_ports[GODOT_ORGAN_COUNT];  // one host port per organ class, index = GODOT_ORGAN_*
    std::string instance_ports[GODOT_DRAWN_COUNT];  // one instance port per drawn class, index = GODOT_ORGAN_* before the root
    std::string ground_port;         // host port the ground square is laid through (ase-pl-flora: flora.ground.v1)
    std::string brook_port;          // host port the brooks are laid through (ase-pl-water: water.brook.v1, H.8)
    std::string terrain_port;        // host port of the terrain samples (ase-pl-flora: flora.terrain.v1)
    std::string ground_cell_port;    // host port of the ground cells (ase-pl-flora: flora.ground.cell.v1)
    std::string sky_port;            // host port of the sky and the light of the scene (ase-pl-sky: sky.scene.v1)
    std::string stars_port;          // host port of the visible star set at its places on the celestial sphere (ase-pl-sky: sky.stars.v1)
    std::string seed_port;           // host port of the seed bodies of ase-physics (ase-pl-flora: flora.seed.v1)
    std::string toolrack_scene_port; // host port the tool rack of a patch is laid through (ase-pl-toolrack: toolrack.scene.v1)
    std::string toolrack_rack_port;  // host port of the tool rack bodies of ase-physics (ase-pl-toolrack: toolrack.rack.v1)
    std::string survival_body_port;  // host port of each figure's body for the rail CORPO (survival.body.v1, N0530, P0715)
    std::string survival_rest_port;  // host port of each figure's rest and sleep for the rest card (survival.rest.v1, U05, N0555)
    std::string survival_fire_port;  // host port of every fire cell for the card FOGUEIRA (ase-pl-ingle: ingle.fire.v1, B0657)
    std::string survival_left_port;  // host port of the ash and the burn time of every fire cell for the card FOGUEIRA (ase-pl-ingle: ingle.left.v1, E0257)
    std::string survival_food_port;  // host port of the spoil of the food each figure carries for the ring ESTRAGO (survival.food.v1, U07, N0530)
    std::string survival_smoke_port; // host port of the soot in the air of each figure's cell for the smoke plume of the air mark (survival.smoke.v1, U06, D2500)
    std::string survival_stock_port; // host port the start stock of the bag is filed through (survival.stock.v1, T.8)
    std::string camp_place_port;     // host port the camp place is laid through (ase-pl-ingle: ingle.camp.v1, T.0, T.3)
    std::string camp_hull_port;      // host port each hull corner of a camp make is filed through (ase-pl-ingle: ingle.hull.v1, T.1)
    std::string camp_part_port;      // host port each part of the camp is laid through (ase-pl-ingle: ingle.part.v1, T.3)
    std::string camp_body_port;      // host port of the place and turn of every camp part body (ase-pl-ingle: ingle.body.v1, T.3)
    std::string camp_refuel_port;    // host port of the keepers laying wood this pass (ase-pl-ingle: ingle.refuel.v1, T.7)
    std::string tile_port;           // host port of the bark tiling of the internodes (ase-pl-flora: flora.tile.v1)
    std::string lineage_port;        // host port of the lineage of every shown organ (ase-pl-flora: flora.lineage.v1, TREEGEN T.9)
    std::string sway_port;           // host port of the sway of the organs on a branch (ase-pl-flora: flora.sway.v1)
    std::string bend_port;           // host port of the bend of every leaf card (ase-pl-flora: flora.bend.v1, TREEGEN T.6)
    std::string tint_port;           // host port of the colour offset of every leaf card (ase-pl-flora: flora.tint.v1, TREEGEN T.6)
    std::string atlas_port;          // host port of the atlas cell of every leaf card (ase-pl-flora: flora.atlas.v1, TREEGEN T.6)
    std::string bark_port;           // host port of the vertices of the bark tube of every axis (ase-pl-flora: flora.mesh.bark.v1)
    std::string knot_port;           // host port of the knots where a branch leaves its axis (ase-pl-flora: flora.mesh.knot.v1, N0543)
    std::string foot_port;           // host port of the foot flare of every stem at the ground (ase-pl-flora: flora.mesh.foot.v1, N0543)
    std::string scar_port;           // host port of the scars a cut branch leaves on its axis (ase-pl-flora: flora.mesh.scar.v1, N0543)
    std::string stream_port;         // host port of the water of the brooks (ase-pl-water: water.stream.v1, H.8)
    std::string current_port;        // host port of the flowing water of every river cell (ase-pl-water: water.current.v1, H.13), read once offered
    std::string quality_port;        // host port of what the water of every current cell carries (ase-pl-water: water.quality.v1, D2508), read once offered
    std::string pit_port;            // host port of the open water of every dug pit (ase-pl-hydrogeology: hydrogeology.pit.v1, H.4), read once offered
    std::string groundwater_port;    // host port of the groundwater of every chunk (ase-pl-hydrogeology: hydrogeology.groundwater.v1, H.3), read once offered
    std::string salt_port;           // host port of the salt of every chunk, water half, soil half and sum (ase-pl-hydrogeology: hydrogeology.salt.v1, H.9 I-4), read once offered
    std::string level_port;          // host port the player levels a patch through (ase-pl-flora: flora.level.v1)
    std::string flatten_port;        // host port the player flattens the ground at a place through (ase-pl-flora: flora.flatten.v1)
    std::string lower_port;          // host port the player digs the ground down at a place through (ase-pl-flora: flora.lower.v1)
    std::string raise_port;          // host port the player heaps the ground up at a place through (ase-pl-flora: flora.raise.v1)
    std::string loosen_port;         // host port the player loosens the soil at a place through, the hoe (ase-pl-flora: flora.loosen.v1, B0649)
    std::string pace_port;          // host port the debug mode asks the pace of the game clock through (ase-pl-flora: flora.pace.v1)
    std::string seek_port;           // host port the debug mode moves the game clock to a computed hour through (ase-pl-flora: flora.seek.v1)
    std::string real_port;           // host port every frame files the absolute real position on, sleep included (ase-pl-flora: flora.real.v1)
    std::string date_port;           // host port of the SURA date record and of the date jumps of the debug mode (ase-pl-flora: flora.date.v1)
    std::string day_port;            // host port of the day record: sunrise, sunset, daylight, season and the shares of day and year (ase-pl-flora: flora.day.v1, N0460)
    std::string phase_port;          // host port of the six phase records: start hour and share of each period (ase-pl-flora: flora.phase.v1, N0460)
    std::string season_port;         // host port of the four season records: first LUN and hours of light (ase-pl-flora: flora.season.v1, N0460)
    std::string weather_port;        // host port of the current weather and the forecast of each coming period (ase-pl-sky: sky.weather.v1, N0449)
    std::string weather_code_port;   // host port of the one record of ase-atmosphere's weather kind codes (ase-pl-sky: sky.weather.code.v1, N0449)
    std::string sap_port;            // host port of the one record of the birch's tap and bark vessel (ase-pl-flora: flora.sap.v1, N0457)
    std::string loose_port;          // host port of the soil's looseness raster, one record per cell of its ase-gis layer (ase-terrain, the owner of the compaction: terrain.loose.v1, N0456)
    std::string elevation_port;      // host port the relief of the ground is shaped through (ase-pl-flora: flora.elevation.v1)
    std::string view_port;           // host port the view hands the place of its camera through (ase-pl-flora: flora.view.v1)
    std::string template_corner_port;   // host port of the corners of the organ templates (ase-pl-flora: flora.template.corner.v1)
    std::string template_triangle_port;  // host port of the triangles of the organ templates (ase-pl-flora: flora.template.triangle.v1)
    std::string template_tangent_port;   // host port of the tangents of the organ template corners (ase-pl-flora: flora.template.tangent.v1, T.6)
    std::string figure_body_port;    // host port of the figures the view draws (ase-pl-avatar: avatar.body.v1)
    std::string figure_camera_port;  // host port of the player camera while it follows a figure (ase-pl-avatar: avatar.camera.v1)
    std::string figure_spawn_port;   // host port the view asks for the figure of its seat through (ase-pl-avatar: avatar.spawn.v1)
    std::string figure_stick_port;   // host port the stick of the left half leads the figure through (ase-pl-avatar: avatar.stick.v1)
    std::string figure_orbit_port;   // host port the drag of the right half turns the player camera through (ase-pl-avatar: avatar.orbit.v1)
    std::string figure_switch_port;  // host port the button cam.mode switches the camera through (ase-pl-avatar: avatar.switch.v1)
    std::string figure_release_port; // host port soul.mode hands the figure to the AI through (ase-pl-avatar: avatar.release.v1)
    std::string figure_seize_port;   // host port soul.mode takes the figure back from the AI through (ase-pl-avatar: avatar.seize.v1)
    std::string figure_hold_port;    // host port the hand of Phase 06 puts its thing into the figure's hand through (ase-pl-avatar: avatar.hold.v1)
    std::string figure_drop_port;    // host port the hand of Phase 06 empties the figure's hand through (ase-pl-avatar: avatar.drop.v1)
    std::string figure_don_port;     // host port the pack of the seat goes on the figure's back through (ase-pl-avatar: avatar.don.v1, N0590)
    std::string figure_fill_port;   // host port brookTap fills the vessel of the figure's hand through (ase-pl-avatar: avatar.fill.v1)
    std::string figure_drain_port;   // host port the release of the can lets its litres go through (ase-pl-avatar: avatar.drain.v1)
    std::string figure_gear_port;   // host port of the pieces every figure carries (ase-pl-avatar: avatar.gear.v1)
    std::string figure_code_port;    // host port of the codes gait, soul and camera of the body record are compared against (ase-pl-avatar: avatar.code.v1)
    std::string figure_rig_port;     // host port the scene reports the measured skeleton of a figure through (ase-pl-avatar: avatar.rig.v1)
    std::string figure_turn_port;    // host port the drag of the right half turns the figure itself through (ase-pl-avatar: avatar.turn.v1)
    std::string figure_area_port;    // host port the view binds a figure to the area plate it stands on through (ase-pl-avatar: avatar.area.v1)
    std::string figure_boundary_port;  // host port of the bumps of walking bodies against the chunk edge (ase-pl-avatar: avatar.boundary.v1)
    std::string op_measure;         // submit operation that reports eye height and height of a figure's skeleton
    std::string op_bind;             // submit operation that binds a figure to the area plate it stands on (07.7, usecase:101612)
    std::string op_spawn;           // submit operation that asks for the figure of a seat at a place
    std::string op_steer;            // submit operation that leads the figure with the stick
    std::string op_turn;             // submit operation that turns the player camera by a drag
    std::string op_switch;           // submit operation that moves the active mark between fixed view and player camera
    std::string op_release;          // submit operation that hands the soul of the figure to the AI (SS/PC to AI/PC)
    std::string op_seize;            // submit operation that takes the soul of the figure back (AI/PC to SS/PC)
    std::string op_hold;             // submit operation that puts an item into the right hand of the figure (07.10)
    std::string op_drop;             // submit operation that empties the right hand of the figure (07.10)
    std::string op_don;              // submit operation that puts an item on the back of the figure (N0590, usecase:101332)
    std::string don;                 // the item key the figure of this seat wears on its back from its first stand (ranzen)
    std::string op_fill;             // submit operation that fills the vessel in the figure's hand at the brook (07.14)
    std::string op_drain;            // submit operation that lets litres of the vessel in the figure's hand go (07.14)
    std::string op_stock;            // submit operation that lays one piece of food into the figure's bag (T.8)
    std::vector<VivariumStockSpec> stock;  // the start stock of the bag of the figure of this seat, one request per piece
    double      camp_x = 0.0;        // the centre of the stone ring, world metres east (entry camp, measured in T.0)
    double      camp_z = 0.0;        // the centre of the stone ring, world metres north (entry camp, measured in T.0)
    uint32_t    camp_place = 0;      // the patch whose stem foot the parts are placed from (entry camp.place)
    std::vector<VivariumCampPartSpec> camp_parts;  // every body of the camp fire, one part request each (camp.parts)
    uint32_t    figure_seat = 0;     // the seat of this view: ase-player names the figure it spawns after it
    double      figure_x = 0.0;      // spawn place of the figure, world metres east
    double      figure_z = 0.0;      // spawn place of the figure, world metres north
    uint32_t    figure_place = 0;    // place of the patch whose plate the figure of this seat is bound to (07.7): a relation, never found by where the figure stands
    std::string figure_character;    // key of the character the figure of this seat plays, as data/creation_characters.json of ase-creation names it; the spawn request carries its hash
    std::string op_irrigate;        // submit operation that pours on one patch
    std::string op_prune;            // submit operation that cuts the branch an internode sits on (03.10 step 9)
    std::string op_level;            // submit operation that levels the ground of one patch (03.8 point 2)
    std::string op_flatten;          // submit operation that flattens a disc of the ground at a place (03.8)
    std::string op_lower;            // submit operation that digs a disc of the ground down at a place (a spade stroke)
    std::string op_raise;            // submit operation that heaps a disc of the ground up at a place (a spade stroke)
    std::string op_loosen;           // submit operation that loosens a block of the topsoil at a place (a hoe stroke, B0649)
    std::string op_pace;             // submit operation that runs the host's game clock at a time scale (debug time lapse)
    std::string op_seek;             // submit operation that moves the host's game clock to an absolute game hour (a host joining a world)
    std::string op_seek_date;        // submit operation that moves the host's game clock to a SURA date, ase-calendar computes its hour
    std::string op_real;             // submit operation that files the host's real seconds since its start (a return from the background is one such filing)
    std::string op_shape;            // submit operation that sets the height amplitude and the ground level of the relief
    std::string op_view;             // submit operation that tells the engine where the view's camera stands (level of detail)
    std::string op_lay;              // submit operation that lays one patch, the ground square or a brook
    double      irrigate_amount_mm = 0.0;  // water one press of Regar pours, mm
    std::vector<VivariumPatchSpec> patches;  // the patches the game host lays, every place once
    VivariumGroundSpec ground;               // the square of terrain the view draws
    std::vector<VivariumBrookSpec> brooks;   // the brooks the game host lays
};

}  // namespace ase::adp::godot
