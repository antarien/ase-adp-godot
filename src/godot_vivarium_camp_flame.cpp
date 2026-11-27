/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW CAMP FLAME
 *
 * @file        godot_vivarium_camp_flame.cpp
 * @brief       follow_camp_flames - the flame, light, smoke, sparks and ash bed of every fire cell of the camp
 * @description PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP, Endabnahme im Bild B2 to B6 (usecase:5436 - kindle fire). ase-fire
 *              keeps one fire cell per fuel body and ase-pl-ingle lays each on ingle.fire.v1 (IngleHostFireExptSystem):
 *              its stage marks, its fuel left and laid, its heat and its fuel body. The view computes nothing of the
 *              fire; it stands one flame over the drawn body of every cell (godot_vivarium_camp.cpp) and shapes it
 *              from what the owner holds:
 *
 *                the tongue   camp_flame.gdshader on a quad upright toward the eye; its height grows with the size of
 *                             the cell, its fuel left and its heat, lower over charcoal, a glow at the foot while the
 *                             wood heats and while it glows as ember, gone under ash;
 *                the light    an omni light on the ground and the figure, strongest over a flame;
 *                the steam    white puffs while wood that does not burn yet boils its water off (ingle.left.v1 drying,
 *                             mark HUB_FIRE_CELL_DRYG);
 *                the smoke    particles rising while the cell burns or glows, as many as its heat gives, and while
 *                             wood that does not burn yet gives off its gases before they catch (ingle.left.v1
 *                             smoking, mark HUB_FIRE_CELL_SMOK); a log far from the fire steams first, smokes later
 *                             and catches last, a log that touches a burning one catches at once;
 *                the sparks   particles thrown up by a flame only;
 *                the bed      a grey disc where the cell is ash.
 *
 *              The fire grows as its cells catch, the tinder first, the sticks and logs after it, each bringing its own
 *              tongue; it shrinks as each cell burns its fuel down. Every cell notes its stage and whether its tongue
 *              stands in the camera frustum, at its first frame and per change (`camp flame cell=N body=B stage=S
 *              framed F`), so the acceptance judges each picture from the lines of the same frame.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_068
 * @category    ecs/module
 * @usecase     usecase:5436 - kindle fire
 * @actor       Funken (FNKN)
 * @created     2026-11-23
 * @modified    2026-11-25
 * @version     00.00.02.00001 [feat]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_camp.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture1_d.hpp>
#include <godot_cpp/classes/gradient_texture2d.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/omni_light3d.hpp>
#include <godot_cpp/classes/particle_process_material.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>
#include <cstdint>
#include <unordered_set>

namespace ase::adp::godot {

namespace {

constexpr const char* kCampFlameShader = "res://shaders/camp_flame.gdshader";  // the tongue of a burning cell
constexpr const char* kCampFlameName = "CampFlame_";  // the prefix of a flame node, followed by the entity of its cell
constexpr const char* kFlameTongue = "Tongue";        // the quad camp_flame.gdshader draws upright toward the eye
constexpr const char* kFlameGlow = "Glow";            // the light the fire throws on the ground and the figure
constexpr const char* kFlameSmoke = "Smoke";          // the smoke particles rising from the cell
constexpr const char* kFlameSteam = "Steam";          // the steam puffs of wood that boils its water off
constexpr const char* kFlameSparks = "Sparks";        // the sparks a flame throws up
constexpr const char* kFlameBed = "Bed";              // the ash bed the cell leaves
constexpr const char* kFlameHeatShare = "heat_share";  // instance uniform of camp_flame.gdshader
constexpr const char* kFlameStage = "stage";           // instance uniform of camp_flame.gdshader
constexpr const char* kFlameSeed = "seed";             // instance uniform of camp_flame.gdshader
constexpr uint32_t kStageHeating = 0u;   // the cell heats toward ignition: no mark of ingle.fire.v1 set
constexpr uint32_t kStageFlame = 1u;     // mark flame (HUB_FIRE_CELL_FLAM)
constexpr uint32_t kStageCharcoal = 2u;  // mark charcoal (HUB_FIRE_CELL_CHAR)
constexpr uint32_t kStageEmber = 3u;     // mark ember (HUB_FIRE_CELL_EMBR)
constexpr uint32_t kStageAsh = 4u;       // mark ash (HUB_FIRE_CELL_ASH)
constexpr uint32_t kFlameNoteShift = 3u;  // stage << 3 | steam | smoke | framed: one key per noted state of a flame
constexpr uint32_t kFlameNoteSteam = 4u;  // the bit of the key while the wood steams
constexpr uint32_t kFlameNoteSmoke = 2u;  // the bit of the key while the unburnt wood smokes
constexpr float kFlameTallM = 0.9f;       // tongue of the largest cell at full heat and full fuel, metres
constexpr float kFlameWideShare = 0.55f;  // width of a tongue over its height
constexpr float kFlameLowShare = 0.45f;   // a charcoal tongue's height over the flame's: the volatiles are burnt
constexpr float kFlameFuelFloor = 0.35f;  // the share of the height a tongue keeps over its last fuel
constexpr float kFlameSeedStep = 0.37f;   // phase between the flicker of two cells, per entity id modulo kFlameSeedSpan
constexpr uint32_t kFlameSeedSpan = 97u;  // the entity ids fold onto this many phases
constexpr float kGlowEnergy = 2.4f;       // energy of the light of a full flame
constexpr float kGlowRangeM = 4.0f;       // reach of that light, metres
constexpr float kGlowLiftM = 0.35f;       // the light and the smoke start this far above the body
constexpr float kGlowCharShare = 0.6f;    // the light of a charcoal bed over a flame's
constexpr float kGlowEmberShare = 0.3f;   // the light of ember over a flame's
constexpr float kGlowHeatShare = 0.2f;    // the light of heating wood over a flame's, at full heat
constexpr int32_t kSmokeAmount = 48;      // smoke puffs alive at full heat
constexpr double kSmokeLifeS = 4.0;       // seconds a puff rises
constexpr float kSmokeSpeedLow = 0.35f;   // rise of a puff, metres per second, slowest
constexpr float kSmokeSpeedHigh = 0.6f;   // and fastest
constexpr float kSmokeScaleLow = 0.25f;   // size of a puff, smallest
constexpr float kSmokeScaleHigh = 0.6f;   // and largest
constexpr float kSmokeSpreadDeg = 12.0f;  // cone of the rise, degrees
constexpr float kSmokeLiftAccel = 0.15f;  // the buoyancy of hot smoke, metres per second squared upward
constexpr float kSmokeEmberShare = 0.3f;  // smoke of ember over a flame's
constexpr float kSmokeSourceM = 0.12f;    // radius of the sphere the particles start in
constexpr float kSmokePuffM = 0.5f;       // edge of the quad a puff is drawn on
constexpr float kSmokeUnburntShare = 0.5f;  // smoke of wood that gives off its gases before they catch, over a flame's
constexpr int32_t kSteamAmount = 32;      // steam puffs alive while the wood boils its water off
constexpr double kSteamLifeS = 2.5;       // seconds a steam puff rises before it is gone in the air
constexpr float kSteamSpeedLow = 0.25f;   // rise of a steam puff, metres per second, slowest
constexpr float kSteamSpeedHigh = 0.45f;  // and fastest
constexpr float kSteamScaleLow = 0.15f;   // size of a steam puff, smallest
constexpr float kSteamScaleHigh = 0.4f;   // and largest
constexpr float kSteamSpreadDeg = 18.0f;  // cone of the rise, degrees
constexpr float kSteamLiftAccel = 0.1f;   // the buoyancy of warm vapour, metres per second squared upward
constexpr float kSteamPuffM = 0.4f;       // edge of the quad a steam puff is drawn on
constexpr int32_t kSparkAmount = 24;      // sparks alive over a full flame
constexpr double kSparkLifeS = 1.2;       // seconds a spark flies
constexpr float kSparkSpeedLow = 0.8f;    // throw of a spark, metres per second, slowest
constexpr float kSparkSpeedHigh = 1.6f;   // and fastest
constexpr float kSparkFallAccel = 1.0f;   // a spark falls back at this, metres per second squared
constexpr float kSparkScaleLow = 0.02f;   // size of a spark, smallest
constexpr float kSparkScaleHigh = 0.04f;  // and largest
constexpr float kSparkSpreadDeg = 25.0f;  // cone of the throw, degrees
constexpr float kSparkQuadM = 1.0f;       // edge of the quad a spark is drawn on, before its scale
constexpr float kBedRadiusM = 0.35f;      // ash bed of the largest cell, metres
constexpr float kBedThickM = 0.02f;       // thickness of the bed
constexpr float kParticleBoxM = 3.0f;     // edge of the box the particles of a cell are culled by
constexpr float kParticleCoreShare = 0.35f;  // the share of a spot's radius that stays dense before it fades out
constexpr float kParticleCoreAlpha = 0.7f;   // the alpha at the edge of that dense core
constexpr int32_t kParticleSpotPx = 64;      // edge of the texture a round spot is drawn from, pixels

/** The stage of a fire cell from its marks: ase-fire keeps a cell in one stage, the latest mark names it. */
uint32_t stage_of(const VivariumSurvivalFireView& cell) {
    const auto mark = [](double value) { return !std::isnan(value) && value != 0.0; };
    return mark(cell.ash)        ? kStageAsh
           : mark(cell.ember)    ? kStageEmber
           : mark(cell.charcoal) ? kStageCharcoal
           : mark(cell.flame)    ? kStageFlame
                                 : kStageHeating;
}

/** A value of the record as a number, 0 where the record lacks it. */
float measured(double value) {
    return std::isnan(value) ? 0.0f : static_cast<float>(value);
}

/**
 * A material for particles drawn as round, soft spots toward the eye, coloured by the ramp of their process material.
 * Godot's particle billboard builds its turn from normalised axes and drops the scale of every particle unless
 * FLAG_BILLBOARD_KEEP_SCALE stands: without it a spark of kSparkScaleLow to kSparkScaleHigh is drawn at the full edge
 * of its quad. The radial texture fades each quad from its centre to its rim, so a puff is a disc and a spark a dot,
 * never a square.
 */
::godot::Ref<::godot::StandardMaterial3D> particle_material() {
    ::godot::Ref<::godot::Gradient> falloff;
    falloff.instantiate();
    ::godot::PackedFloat32Array offsets;
    offsets.push_back(0.0f);
    offsets.push_back(kParticleCoreShare);
    offsets.push_back(1.0f);
    ::godot::PackedColorArray colors;
    colors.push_back(::godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
    colors.push_back(::godot::Color(1.0f, 1.0f, 1.0f, kParticleCoreAlpha));
    colors.push_back(::godot::Color(1.0f, 1.0f, 1.0f, 0.0f));
    falloff->set_offsets(offsets);
    falloff->set_colors(colors);
    ::godot::Ref<::godot::GradientTexture2D> spot;
    spot.instantiate();
    spot->set_gradient(falloff);
    spot->set_fill(::godot::GradientTexture2D::FILL_RADIAL);
    spot->set_fill_from(::godot::Vector2(0.5f, 0.5f));
    spot->set_fill_to(::godot::Vector2(0.5f, 0.0f));  // the rim of the disc touches the edge of the quad
    spot->set_width(kParticleSpotPx);
    spot->set_height(kParticleSpotPx);
    ::godot::Ref<::godot::StandardMaterial3D> material;
    material.instantiate();
    material->set_billboard_mode(::godot::BaseMaterial3D::BILLBOARD_PARTICLES);
    material->set_flag(::godot::BaseMaterial3D::FLAG_BILLBOARD_KEEP_SCALE, true);
    material->set_transparency(::godot::BaseMaterial3D::TRANSPARENCY_ALPHA);
    material->set_shading_mode(::godot::BaseMaterial3D::SHADING_MODE_UNSHADED);
    material->set_flag(::godot::BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    material->set_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO, spot);
    return material;
}

/** The look of one kind of particle: count, life, throw, size, cone, pull, quad and the colours they fade through. */
struct FlameParticles {
    const char*             name = "";
    int32_t                 amount = 0;
    double                  life = 0.0;
    float                   speed_low = 0.0f;
    float                   speed_high = 0.0f;
    float                   scale_low = 0.0f;
    float                   scale_high = 0.0f;
    float                   spread = 0.0f;
    float                   accel = 0.0f;
    float                   edge = 0.0f;
    ::godot::PackedColorArray ramp;
};

/** Particles under `holder` as `look` describes them, rising in their cone and fading along their ramp, not yet emitting. */
::godot::GPUParticles3D* stand_particles(::godot::Node3D* holder, const FlameParticles& look) {
    ::godot::Ref<::godot::Gradient> gradient;
    gradient.instantiate();
    ::godot::PackedFloat32Array offsets;
    const int64_t stops = look.ramp.size();
    for (int64_t i = 0; i < stops; ++i) {
        offsets.push_back(stops > 1 ? static_cast<float>(i) / static_cast<float>(stops - 1) : 0.0f);
    }
    gradient->set_offsets(offsets);
    gradient->set_colors(look.ramp);
    ::godot::Ref<::godot::GradientTexture1D> fade;
    fade.instantiate();
    fade->set_gradient(gradient);
    ::godot::Ref<::godot::ParticleProcessMaterial> process;
    process.instantiate();
    process->set_direction(::godot::Vector3(0.0f, 1.0f, 0.0f));
    process->set_spread(look.spread);
    process->set_gravity(::godot::Vector3(0.0f, look.accel, 0.0f));
    process->set_param_min(::godot::ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, look.speed_low);
    process->set_param_max(::godot::ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, look.speed_high);
    process->set_param_min(::godot::ParticleProcessMaterial::PARAM_SCALE, look.scale_low);
    process->set_param_max(::godot::ParticleProcessMaterial::PARAM_SCALE, look.scale_high);
    process->set_emission_shape(::godot::ParticleProcessMaterial::EMISSION_SHAPE_SPHERE);
    process->set_emission_sphere_radius(kSmokeSourceM);
    process->set_color_ramp(fade);
    ::godot::Ref<::godot::QuadMesh> quad;
    quad.instantiate();
    quad->set_size(::godot::Vector2(look.edge, look.edge));
    quad->set_material(particle_material());
    ::godot::GPUParticles3D* particles = memnew(::godot::GPUParticles3D);
    particles->set_name(look.name);
    particles->set_amount(look.amount);
    particles->set_lifetime(look.life);
    particles->set_process_material(process);
    particles->set_draw_pass_mesh(0, quad);
    particles->set_visibility_aabb(::godot::AABB(::godot::Vector3(-kParticleBoxM * 0.5f, 0.0f, -kParticleBoxM * 0.5f),
                                                 ::godot::Vector3(kParticleBoxM, kParticleBoxM * 2.0f, kParticleBoxM)));
    particles->set_position(::godot::Vector3(0.0f, kGlowLiftM, 0.0f));
    particles->set_emitting(false);
    holder->add_child(particles);
    return particles;
}

/** The flame of one fire cell under the camp root: its tongue, light, smoke, sparks and ash bed, not yet shaped. */
::godot::Node3D* stand_flame(::godot::Node3D* root, uint32_t cell, const ::godot::Ref<::godot::ShaderMaterial>& material) {
    ::godot::Node3D* flame = memnew(::godot::Node3D);
    flame->set_name(::godot::String(kCampFlameName) + ::godot::String::num_uint64(cell));
    root->add_child(flame);
    ::godot::Ref<::godot::QuadMesh> quad;
    quad.instantiate();
    quad->set_size(::godot::Vector2(1.0f, 1.0f));
    quad->set_center_offset(::godot::Vector3(0.0f, 0.5f, 0.0f));  // the foot of the tongue on the body
    ::godot::MeshInstance3D* tongue = memnew(::godot::MeshInstance3D);
    tongue->set_name(kFlameTongue);
    tongue->set_mesh(quad);
    tongue->set_material_override(material);
    tongue->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    flame->add_child(tongue);
    tongue->set_instance_shader_parameter(kFlameSeed, static_cast<float>(cell % kFlameSeedSpan) * kFlameSeedStep);
    ::godot::OmniLight3D* glow = memnew(::godot::OmniLight3D);
    glow->set_name(kFlameGlow);
    glow->set_color(::godot::Color(1.0f, 0.55f, 0.2f));
    glow->set_param(::godot::Light3D::PARAM_RANGE, kGlowRangeM);
    glow->set_param(::godot::Light3D::PARAM_ENERGY, 0.0f);
    glow->set_shadow(false);
    glow->set_position(::godot::Vector3(0.0f, kGlowLiftM, 0.0f));
    flame->add_child(glow);
    FlameParticles smoke{kFlameSmoke, kSmokeAmount, kSmokeLifeS, kSmokeSpeedLow, kSmokeSpeedHigh, kSmokeScaleLow,
                         kSmokeScaleHigh, kSmokeSpreadDeg, kSmokeLiftAccel, kSmokePuffM, {}};
    smoke.ramp.push_back(::godot::Color(0.42f, 0.4f, 0.38f, 0.0f));
    smoke.ramp.push_back(::godot::Color(0.36f, 0.35f, 0.34f, 0.45f));
    smoke.ramp.push_back(::godot::Color(0.5f, 0.5f, 0.5f, 0.0f));
    stand_particles(flame, smoke);
    FlameParticles steam{kFlameSteam, kSteamAmount, kSteamLifeS, kSteamSpeedLow, kSteamSpeedHigh, kSteamScaleLow,
                         kSteamScaleHigh, kSteamSpreadDeg, kSteamLiftAccel, kSteamPuffM, {}};
    steam.ramp.push_back(::godot::Color(0.95f, 0.96f, 0.98f, 0.0f));
    steam.ramp.push_back(::godot::Color(0.92f, 0.93f, 0.95f, 0.35f));
    steam.ramp.push_back(::godot::Color(1.0f, 1.0f, 1.0f, 0.0f));
    stand_particles(flame, steam);
    FlameParticles sparks{kFlameSparks, kSparkAmount, kSparkLifeS, kSparkSpeedLow, kSparkSpeedHigh, kSparkScaleLow,
                          kSparkScaleHigh, kSparkSpreadDeg, -kSparkFallAccel, kSparkQuadM, {}};
    sparks.ramp.push_back(::godot::Color(1.0f, 0.85f, 0.4f, 1.0f));
    sparks.ramp.push_back(::godot::Color(1.0f, 0.35f, 0.05f, 0.8f));
    sparks.ramp.push_back(::godot::Color(0.6f, 0.1f, 0.0f, 0.0f));
    stand_particles(flame, sparks);
    ::godot::Ref<::godot::CylinderMesh> disc;
    disc.instantiate();
    disc->set_top_radius(kBedRadiusM);
    disc->set_bottom_radius(kBedRadiusM);
    disc->set_height(kBedThickM);
    ::godot::Ref<::godot::StandardMaterial3D> ash;
    ash.instantiate();
    ash->set_albedo(::godot::Color(0.24f, 0.23f, 0.22f));
    disc->set_material(ash);
    ::godot::MeshInstance3D* bed = memnew(::godot::MeshInstance3D);
    bed->set_name(kFlameBed);
    bed->set_mesh(disc);
    bed->set_visible(false);
    flame->add_child(bed);
    return flame;
}

/**
 * The flame of one cell as its owner holds it: the tongue stands at the height its size, its fuel left and its heat
 * give, the light and the smoke follow the stage and the heat, the sparks fly only from a flame and the bed lies only
 * under ash. `heat_share` is the heat of the cell over the hottest cell, `size_share` the fuel the cell was laid with
 * over the largest cell, `fuel_share` the fuel it has left over the fuel it was laid with. `steaming` and `smoking` are
 * the marks of wood that does not burn yet: they follow the owner, never the heat share, so a cool log far from the
 * fire steams and smokes in the picture as soon as ase-fire says its face boils or gives off its gases. Returns the
 * tongue's height.
 */
float shape_flame(::godot::Node3D* flame, uint32_t stage, float heat_share, float size_share, float fuel_share,
                  bool steaming, bool smoking) {
    const bool burning = stage == kStageFlame || stage == kStageCharcoal;
    const float size = kFlameTallM * ::godot::Math::sqrt(size_share);
    const float tall = burning ? size * (kFlameFuelFloor + (1.0f - kFlameFuelFloor) * fuel_share) * heat_share *
                                     (stage == kStageCharcoal ? kFlameLowShare : 1.0f)
                               : size * kFlameLowShare;  // heating wood and ember glow at the foot
    auto* tongue = ::godot::Object::cast_to<::godot::MeshInstance3D>(flame->get_node_or_null(::godot::NodePath(kFlameTongue)));
    if (tongue != nullptr) {
        tongue->set_visible(stage != kStageAsh && tall > 0.0f);
        tongue->set_scale(::godot::Vector3(::godot::Math::max(tall * kFlameWideShare, kBedThickM),
                                           ::godot::Math::max(tall, kBedThickM),
                                           ::godot::Math::max(tall * kFlameWideShare, kBedThickM)));
        tongue->set_instance_shader_parameter(kFlameHeatShare, heat_share);
        tongue->set_instance_shader_parameter(kFlameStage, static_cast<float>(stage));
    }
    const float light = stage == kStageFlame      ? 1.0f
                        : stage == kStageCharcoal ? kGlowCharShare
                        : stage == kStageEmber    ? kGlowEmberShare
                        : stage == kStageHeating  ? kGlowHeatShare
                                                  : 0.0f;
    auto* glow = ::godot::Object::cast_to<::godot::OmniLight3D>(flame->get_node_or_null(::godot::NodePath(kFlameGlow)));
    if (glow != nullptr) {
        glow->set_param(::godot::Light3D::PARAM_ENERGY, kGlowEnergy * light * heat_share);
    }
    auto* smoke = ::godot::Object::cast_to<::godot::GPUParticles3D>(flame->get_node_or_null(::godot::NodePath(kFlameSmoke)));
    if (smoke != nullptr) {
        const float puffs = burning                                ? heat_share
                            : stage == kStageEmber                 ? kSmokeEmberShare * heat_share
                            : stage == kStageHeating && smoking    ? kSmokeUnburntShare
                                                                   : 0.0f;
        smoke->set_amount_ratio(puffs);
        smoke->set_emitting(puffs > 0.0f);
    }
    auto* steam = ::godot::Object::cast_to<::godot::GPUParticles3D>(flame->get_node_or_null(::godot::NodePath(kFlameSteam)));
    if (steam != nullptr) {
        const bool rising = stage == kStageHeating && steaming;
        steam->set_amount_ratio(rising ? 1.0f : 0.0f);
        steam->set_emitting(rising);
    }
    auto* sparks = ::godot::Object::cast_to<::godot::GPUParticles3D>(flame->get_node_or_null(::godot::NodePath(kFlameSparks)));
    if (sparks != nullptr) {
        sparks->set_amount_ratio(stage == kStageFlame ? heat_share : 0.0f);
        sparks->set_emitting(stage == kStageFlame && heat_share > 0.0f);
    }
    auto* bed = ::godot::Object::cast_to<::godot::MeshInstance3D>(flame->get_node_or_null(::godot::NodePath(kFlameBed)));
    if (bed != nullptr) {
        bed->set_visible(stage == kStageAsh);
        const float spread = ::godot::Math::sqrt(size_share);
        bed->set_scale(::godot::Vector3(spread, 1.0f, spread));
    }
    return tall;
}

}  // namespace

void follow_camp_flames(VivariumCampState& camp, const std::vector<VivariumSurvivalFireView>& fires,
                        const ::godot::Camera3D* eye, std::vector<::godot::String>& lines) {
    if (camp.root == nullptr) {
        return;
    }
    if (!camp.flame_loaded) {
        camp.flame_loaded = true;
        const ::godot::Ref<::godot::Shader> shader = load_plant_shader(kCampFlameShader);
        if (shader.is_valid()) {
            camp.flame_material.instantiate();
            camp.flame_material->set_shader(shader);
        } else {
            lines.push_back(::godot::String("camp flame shader ") + kCampFlameShader + " did not load");
        }
    }
    if (camp.flame_material.is_null()) {
        return;
    }
    // THE SHARES OF THE CAMP: the hottest cell and the largest laid fuel of this frame, so every flame is measured
    // against the fire it belongs to and none against a number of the view
    float hottest = 0.0f;
    float largest = 0.0f;
    for (const VivariumSurvivalFireView& cell : fires) {
        hottest = ::godot::Math::max(hottest, measured(cell.heat));
        largest = ::godot::Math::max(largest, measured(cell.fuel_laid));
    }
    std::unordered_set<uint32_t> seen;
    for (const VivariumSurvivalFireView& cell : fires) {
        if (std::isnan(cell.body)) {
            continue;  // a cell without its fuel body has no place to burn in the picture
        }
        const auto body = camp.drawn.find(static_cast<uint32_t>(cell.body));
        if (body == camp.drawn.end() || body->second == nullptr) {
            continue;  // ingle.body.v1 shows the body on a later frame
        }
        seen.insert(cell.object_id);
        ::godot::Node3D*& flame = camp.flames[cell.object_id];
        if (flame == nullptr) {
            flame = stand_flame(camp.root, cell.object_id, camp.flame_material);
        }
        flame->set_position(body->second->get_position());
        const uint32_t stage = stage_of(cell);
        const float laid = measured(cell.fuel_laid);
        const float heat_share = hottest > 0.0f ? measured(cell.heat) / hottest : 0.0f;
        const float size_share = largest > 0.0f ? laid / largest : 0.0f;
        const float fuel_share = laid > 0.0f ? ::godot::Math::clamp(measured(cell.fuel) / laid, 0.0f, 1.0f) : 0.0f;
        const bool steaming = !std::isnan(cell.drying) && cell.drying != 0.0;
        const bool smoking = !std::isnan(cell.smoking) && cell.smoking != 0.0;
        const float tall = shape_flame(flame, stage, heat_share, size_share, fuel_share, steaming, smoking);
        const ::godot::Vector3 heart = flame->get_position() + ::godot::Vector3(0.0f, tall * 0.5f, 0.0f);
        const bool framed = eye != nullptr && eye->is_position_in_frustum(heart);
        const uint32_t state = (stage << kFlameNoteShift) | (steaming ? kFlameNoteSteam : 0u) |
                               (smoking ? kFlameNoteSmoke : 0u) | (framed ? 1u : 0u);
        const auto noted = camp.flame_noted.find(cell.object_id);
        if (noted == camp.flame_noted.end() || noted->second != state) {
            camp.flame_noted[cell.object_id] = state;
            lines.push_back("camp flame cell=" + ::godot::String::num_uint64(cell.object_id) + " body=" +
                            ::godot::String::num_uint64(static_cast<uint32_t>(cell.body)) + " stage=" +
                            ::godot::String::num_uint64(stage) + " steam " + (steaming ? "1" : "0") + " smoke " +
                            (smoking ? "1" : "0") + " framed " + (framed ? "1" : "0") + " height " +
                            ::godot::String::num(tall, 2));
        }
    }
    for (auto flame = camp.flames.begin(); flame != camp.flames.end();) {
        if (seen.count(flame->first) == 0u) {
            if (flame->second != nullptr) {
                flame->second->queue_free();
            }
            camp.flame_noted.erase(flame->first);
            flame = camp.flames.erase(flame);
        } else {
            ++flame;
        }
    }
}

}  // namespace ase::adp::godot
