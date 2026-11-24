#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the 3D scene of the care view: its nodes, the view section, camera and water
 * @description All compile-time constants of the scene sub-type; NO runtime values - those belong in the views.
 *              The plant geometry tolerances, the node names of the scene, the keys of the "view" section of the
 *              start configuration, the exposure of the camera, the zoom of the near view, the material and
 *              template roles, the stand-in of a seed, the water at NN0 and the ground tolerances. The view
 *              computes none of the plant, the light or the water; these are its names and its look.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter types.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated
 *              by purpose into sub-headers and stays a pure collector of them. These values are read by the scene,
 *              the plant geometry, the water and the ground of the view.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-22
 * @modified    2026-11-24
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: none.
 */

/**
 * Plant geometry (godot_plant_geometry.hpp): the template, ground and instance records go unchanged into the
 * arrays the instances are drawn with - no organ and no template is computed here.
 */
constexpr float GODOT_GEOMETRY_EPSILON = 1.0e-6f;   // a length or extent shorter than this counts as none
constexpr float GODOT_GEOMETRY_UPRIGHT = 0.99f;     // |up share| above which a direction counts as upright: its frame is taken against east, not against up
constexpr double GODOT_ORDER_EVERY = -1.0;          // split order below the stem's (0): every organ of a class lies above it

/**
 * The 3D scene of the care view (godot_plant_scene.hpp, PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2):
 * the names of its nodes and the keys of the "view" section of the start configuration - the near
 * view and the visible orders by frame time (03.6, V8) are numbers of that section. Every plant stands as
 * one node per body (GODOT_PLANT_BODY_PREFIX and the patch's object id) under GODOT_PLANT_BODIES, its six
 * instance nodes under it.
 */
constexpr const char* GODOT_PLANT_SCENE_ROOT = "PlantScene";      // name of the scene's root node
constexpr const char* GODOT_PLANT_CAMERA = "PlantCamera";         // the camera of the near view
constexpr const char* GODOT_PLANT_SUN = "PlantSun";               // the directional light the sun of the sky port sets
constexpr const char* GODOT_PLANT_MOON = "PlantMoon";             // prefix of the directional light of a moon slot: this and the slot number
constexpr const char* GODOT_PLANT_SUN_DISC = "PlantSunDisc";      // the sun as a disc in the sky, along its direction
constexpr const char* GODOT_PLANT_MOON_DISC = "PlantMoonDisc";    // prefix of the disc of a moon slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_COMET_HEAD = "PlantCometHead";  // prefix of the head of a comet slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_COMET_TAIL = "PlantCometTail";  // prefix of the tail of a comet slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_CLOUDS = "PlantClouds";         // node name of the multimesh of the cloud banks in the sky (CloudBank, N04)
constexpr float GODOT_SKY_CLOUD_DISTANCE = 900.0f;  // metres from the camera to a cloud bank: nearer than the discs, so a bank passes over sun and moon as the mockup draws it after them
constexpr const char* GODOT_PLANT_PLANET_DISC = "PlantPlanetDisc";  // prefix of the disc of a planet slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_COMPANION_DISC = "PlantCompanionDisc";  // prefix of the disc of a companion star slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_FRAGMENT_DISC = "PlantFragmentDisc";    // prefix of the disc of a moon half slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_BELT_GRAIN = "PlantBeltGrain";          // prefix of a particle of the asteroid belt arc: this and the particle number
constexpr const char* GODOT_PLANT_ENVIRONMENT = "PlantEnvironment";  // sky colour, ambient light and fog of the sky port
constexpr const char* GODOT_PLANT_HAZE = "PlantHaze";             // the shimmer of hot air in front of the camera (usecase:101515 - veil sight with weather)
constexpr const char* GODOT_PLANT_GROUND = "PlantGround";         // the ground from the terrain samples
constexpr const char* GODOT_PLANT_WATER = "PlantWater";           // the water of the brooks in their beds (usecase:101339 - show the ground around plant)
constexpr const char* GODOT_PLANT_BODIES = "PlantBodies";         // the node the plants stand under, one child per body
constexpr const char* GODOT_PLANT_BODY_PREFIX = "PlantBody";      // name of a plant's node: this and the object id of its patch
constexpr const char* GODOT_PLANT_BARK = "PlantBark";             // the continuous bark tube of every axis of the plant, the node of its two meshes below (usecase:101613)
constexpr const char* GODOT_PLANT_TUBES = "PlantTubes";           // the bark tube of the internodes up to the shown orders, every corner of the knot ring
constexpr const char* GODOT_PLANT_TWIGS = "PlantTwigs";           // the bark tube of the internodes above them, on evenly spaced corners of the knot ring
constexpr const char* GODOT_PLANT_SCARS = "PlantScars";           // the wood disks closing every cut stub on its tip knot up to the shown orders, fans of scar_indices drawn in the cut wood (PHASE_09_TREEGEN Schnittfläche)
constexpr const char* GODOT_PLANT_SCAR_TWIGS = "PlantScarTwigs";  // the same disks on the stubs above the shown orders, drawn by the coarse pass of the bark shader as the twigs they close
constexpr uint32_t GODOT_TWIG_LEAST_CORNERS = 3u;                 // corners of a knot ring a twig is drawn with at least: the fewest that close a surface, RENDER_DATA_TMPL_CYL_SIDES_CORS of ase-render-data
constexpr const char* GODOT_PLANT_LEAVES = "PlantLeaves";         // the leaves as instances
constexpr const char* GODOT_PLANT_LEAVES_CORS = "PlantLeavesCors";  // the leaves of a body in the far band as instances of the coarse 1 x 1 card (TREEGEN T.6, GODOT_TMPL_LEAF_CORS)
constexpr const char* GODOT_PLANT_MALE = "PlantMale";             // the male catkins as instances
constexpr const char* GODOT_PLANT_FEMALE = "PlantFemale";         // the female catkins as instances
constexpr const char* GODOT_PLANT_FRUIT = "PlantFruit";           // the fruit stands as instances
constexpr const char* GODOT_PLANT_SEEDS = "PlantSeeds";           // the seed bodies of ase-physics as instances, in world places (seed port)
constexpr const char* GODOT_VIEW_KEY = "view";                    // start configuration: the section of the 3D scene
constexpr const char* GODOT_VIEW_KEY_DISTANCE = "camera_distance_m";  // view: metres from the focused stem base to the camera, along the ground
constexpr const char* GODOT_VIEW_KEY_HEIGHT = "camera_height_m";  // view: metres of the camera above the focused stem base
constexpr const char* GODOT_VIEW_KEY_LOOK = "camera_look_m";      // view: metres above the stem base the camera looks at
constexpr const char* GODOT_VIEW_KEY_FOV = "camera_fov_deg";      // view: vertical field of view, degrees
constexpr const char* GODOT_VIEW_KEY_HEADING = "camera_heading_deg";  // view: compass direction the camera looks in, degrees from north towards east
constexpr const char* GODOT_VIEW_KEY_BUDGET = "frame_budget_ms";  // view: frame time the scene may take (OrderBudget)
constexpr const char* GODOT_VIEW_KEY_HEADROOM = "headroom";       // view: share of the budget below which a frame is calm
constexpr const char* GODOT_VIEW_KEY_CEILING = "order_ceiling";   // view: most orders ever drawn on the fine cylinder
constexpr const char* GODOT_VIEW_KEY_START = "order_start";       // view: orders drawn on the fine cylinder at the start
constexpr const char* GODOT_VIEW_KEY_CALM = "calm_frames";        // view: calm frames in a row that give one order more
constexpr const char* GODOT_VIEW_KEY_OVER = "over_frames";        // view: over frames in a row that take one order back
constexpr const char* GODOT_VIEW_KEY_PICK = "pick_px";            // view: logical pixels around a tap within which the middle of an internode is hit (cutting mode, 03.10 step 9)
constexpr const char* GODOT_VIEW_KEY_FLATTEN = "flatten_m";       // view: radius of the disc one tap in the flatten mode levels, metres (03.8)
constexpr const char* GODOT_VIEW_KEY_LEAST_RING = "least_ring_px"; // view: screen pixels a bark ring narrower than this is held at (plant_bark.gdshader, register B0482)
/**
 * The exposure of the scene camera (register D1721): the eye adapts to the night, a fixed camera does not. The
 * sky port hands the light as ase-pl-sky computes it - at night the sun light 0.05 and the ambient
 * 0.02/0.02/0.06 at intensity 0.15 (ATM_CONST_NIGHT_AMBIENT_*), by day at noon the sun 0.95 and the ambient
 * 0.7 (run 20261025-phase03-v46) - and the view keeps those numbers. It lifts the camera's exposure by the
 * square root of the ratio of GODOT_EXPOSURE_DAY_LIGHT to the light of the scene (scene_exposure), the partial
 * adaptation of an eye, never below 1 and at most
 * GODOT_EXPOSURE_LIFT_MAX, as an eye adapts to starlight: the night stays darker than the day, and the ground
 * and the birch stay to be seen. The multiplier is the camera's, so both renderers, mobile and Compatibility,
 * take it.
 */
constexpr float GODOT_EXPOSURE_DAY_LIGHT = 1.0f;   // light of the scene that needs no lift: the sun at noon plus its ambient share
constexpr float GODOT_EXPOSURE_LIFT_MAX = 32.0f;   // largest lift of the exposure: the night stays at least this much darker than the day's light
constexpr float GODOT_ZOOM_START = 1.0f;   // zoom of the near view at the start: the camera as the view section places it
constexpr float GODOT_ZOOM_STEP = 0.8f;    // one wheel turn or pinch towards the birch: distance, height and look point times this
constexpr float GODOT_ZOOM_MIN = 0.08f;    // closest zoom: 0.8 m from a seedling at the configured 10 m
constexpr float GODOT_ZOOM_MAX = 1.6f;     // farthest zoom: the whole ground square in sight
constexpr float GODOT_TUBE_MID = 0.5f;                            // middle of the unit tube template (y from 0 to 1): where a tap picks an internode
constexpr double GODOT_MS_PER_SECOND = 1000.0;                    // the order budget takes frame times in milliseconds
constexpr const char* GODOT_ROLE_BARK = "bark_white";             // material role of the white bark of the tubes and twigs
constexpr const char* GODOT_ROLE_BARK_BASE = "bark_base";         // material role of the stem-base bark, mixed in by the share each internode carries
constexpr const char* GODOT_ROLE_WOOD = "wood_cut";               // material role of the cut wood on the disk that closes a cut stub (GODOT_PLANT_SCARS)
constexpr const char* GODOT_ROLE_LEAF = "leaf";                   // template role of a leaf
constexpr const char* GODOT_ROLE_MALE = "male";                   // template role of a male catkin
constexpr const char* GODOT_ROLE_FEMALE = "female";               // template role of a female catkin
constexpr const char* GODOT_ROLE_FRUIT = "fruit";                 // template role of a fruit stand
constexpr const char* GODOT_ROLE_NUTLET = "nutlet";               // template role of a winged nutlet, built in metres (ase-assgen birch_nutlet)
/**
 * The stand-in of a seed while the nutlet template is missing: the unit leaf at the width of a birch nutlet with
 * both wings, ϑ OTL/NUT/WIDTH of DSGN_024 (ONWInit 0.004 m; BSBI Plant Crib: "total width of nutlet with wings
 * 3-5 mm"). A bound template carries its own size and is drawn at scale one.
 */
constexpr float GODOT_NUTLET_STAND_IN_M = 0.004f;                 // m - width the stand-in leaf of a seed is drawn at

/**
 * The water of the care view (usecase:101339 - show the ground around plant). The water stands at NN0 and nowhere
 * else: one plane at GODOT_WATER_TABLE_M over every ground cell below it (build_water_table,
 * godot_plant_water.hpp), so a dug hole, a carved bed and a hollow of the ground below NN0 hold water. The vertex
 * colour carries the water in the range a colour holds: red the velocity of the stream whose bed holds the vertex
 * over GODOT_WATER_VELOCITY_FULL_M_S, green the depth of the table over GODOT_WATER_DEPTH_FULL_M, blue and alpha
 * the course of that stream. The velocity moves the ripples along the course, the depth turns the shallow colour
 * to the deep one; the view computes no water. The ribbon of a river along a course above the table
 * (build_water, GODOT_WATER_STATIONS sections) is drawn by no view now.
 * The two colours are those of brook water, not of the sea: murky, dark, and slightly green from the
 * algae and the suspended silt a lowland brook carries; the shallow colour shows the bed through it.
 */
constexpr float GODOT_WATER_TABLE_M = 0.0f;              // NN0, the water table: the datum ase-terrain generates its ground around (GEN_DEFAULT_BASE_HEIGHT)
constexpr uint32_t GODOT_WATER_STATIONS = 32u;           // sections of the ribbon along one bed
constexpr float GODOT_WATER_VELOCITY_FULL_M_S = 2.0f;    // velocity the red channel reads 1 at, a fast mountain brook
constexpr float GODOT_WATER_DEPTH_FULL_M = 0.5f;         // depth the green channel reads 1 at, the deep colour
constexpr float GODOT_WATER_SWING_SHARE = 1.0f;          // farthest the ribbon follows the deepest line of a winding bed off its filed course, as a share of the bed width

constexpr float GODOT_GROUND_LEVEL_TOLERANCE_M = 0.01f;                    // a vertex within this of a disc's levelled height is levelled ground (mark_ground)
constexpr float GODOT_GROUND_TANGENT_BIAS = 1.0f / 32767.0f;               // least second share of a coded tangent, Godot's bias of Vector3::octahedron_tangent_encode

}  // namespace ase::adp::godot
