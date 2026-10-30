#pragma once

/**
 * ASE GODOT ADAPTER - SUN AND MOON IN THE SKY OF THE CARE VIEW
 *
 * @file        godot_plant_sky.hpp
 * @brief       The moon light and the discs of sun and moon in the care view's 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11: sun, moon and the day cycle from ase-pl-sky. The sky
 *              port carries the sun record always and a moon record for every visible moon - Scorpii and
 *              Amilox (SkyHostScneExptSystem); the view gives every moon a light of its own along its
 *              direction and stands a disc for the sun and one for every moon in the sky, so sun and moons
 *              take turns in the sky and lighting the scene. The sun light itself stays with
 *              light_plant_scene. A comet record, filed while the comet is visible, stands a head and a
 *              tail in its comet slot (usecase:101274 - track the comets). Nothing of the sky is computed
 *              here: every direction, colour, strength and length is read as the port carried it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101384 - place sun and moon spatially
 * @actor       cycle/DAY/NIGHT (semi/nbp) Day/Night Cycle
 * @created     2026-10-26
 * @modified    2026-10-28
 * @version     00.00.03.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>

#include <godot_cpp/classes/node3d.hpp>

namespace ase::adp::godot {

/**
 * Adds the hidden sun disc (GODOT_PLANT_SUN_DISC) and, for every one of the GODOT_SKY_MOONS_MAX moon slots, a
 * dark moon light (GODOT_PLANT_MOON and the slot number) and a hidden moon disc (GODOT_PLANT_MOON_DISC and the
 * slot number) under the scene root, and for every one of the GODOT_SKY_COMETS_MAX comet slots a hidden head
 * (GODOT_PLANT_COMET_HEAD and the slot number) and a hidden tail (GODOT_PLANT_COMET_TAIL and the slot number).
 * build_plant_scene calls it once.
 */
void build_plant_sky(::godot::Node3D* scene);

/**
 * Sets every moon slot from the moon record in it - the light shines along the negative of the moon's
 * direction in its colour at its strength (CEL_MOON_CONTRIB for Amilox, 0 for Scorpii), the disc stands on
 * that side of the sky; a slot without a record goes dark. Stands the sun disc along the sun direction while
 * the sun is above the horizon plane. Each disc stands at GODOT_SKY_DISC_DISTANCE from the camera, a body at
 * infinity. Every comet slot with a record shows its head along the comet's direction and its tail as a cone
 * from the head along the tail direction, as long as the arc of the record; a slot without one stays hidden.
 * A sky the port did not carry yet changes nothing.
 */
void light_plant_sky(::godot::Node3D* scene, const VivariumSkyView& sky);

}  // namespace ase::adp::godot
