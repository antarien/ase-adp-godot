#pragma once

/**
 * ASE GODOT ADAPTER - THE GROUND OF THE CARE VIEW IN THE COLOUR OF ITS SEASON
 *
 * @file        godot_plant_ground_season.hpp
 * @brief       The season, the snow, the depth and the horizon the ground of the care view's 3D scene is painted in
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N06. The mockup fills every row of the ground with
 *              groundCol (m.html 426 to 428): the colour of the season, turned toward the snow while it snows,
 *              darkened toward the horizon and blended toward the horizon's colour there. GroundSeason
 *              (godot_hud_ground_season.hpp) derives the colours and shares as the shade; this unit hands them to
 *              every soil material of GODOT_GROUND_SHADER_FILE at once as global shader parameters, with the rows
 *              the depth is measured on. The mockup's z is a row of the screen (rowY, m.html 358): 0 on the horizon,
 *              where its ridges stand on the ground, 1 on the foot of the frame. In the 3D scene the ground ends at
 *              the far edge of the square, so that edge is the horizon: plant_ground_far_edge finds the point of it
 *              straight ahead of the camera, its screen row is z 0, and the ridges of row N05 stand on the same point
 *              (light_plant_horizon_ridge), so no band of sky lies between the hills and the ground. The mockup's
 *              light() multiplies the row as well; in the 3D scene the sun and the ambient light of the sky port
 *              light the ground, so the shade carries no light. Nothing of the terrain is computed here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.01.00001
 */

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

// The global shader parameters of the ground's season; project.godot declares each under [shader_globals], since a
// shader that reads a global the project does not declare does not compile. They stand here and not in types.hpp,
// which nearly every unit of the adapter includes.
constexpr const char* GODOT_GROUND_SEASON_COLOR = "ground_season_color";      // global: the season's colour, snow mixed in
constexpr const char* GODOT_GROUND_SEASON_MIX = "ground_season_mix";          // global: share the season's colour takes of the cover, 0 before the HUD stands
constexpr const char* GODOT_GROUND_DEPTH = "ground_depth";                    // global: floor and span of the darkening, floor + span * z
constexpr const char* GODOT_GROUND_HORIZON_COLOR = "ground_horizon_color";    // global: the horizon's colour the far rows blend toward
constexpr const char* GODOT_GROUND_HORIZON_SHARE = "ground_horizon_share";    // global: share of the blend on the horizon, times (1 - z)
constexpr const char* GODOT_GROUND_ROWS = "ground_rows";                      // global: row of z 0 (the far edge) and of z 1 (the frame's foot) as shares of the screen's height, and rowY's power
constexpr const char* GODOT_GROUND_SNOW = "ground_snow";                      // global: the snow's colour, its alpha the share it takes of the bare soil and the banks while it snows, 0 otherwise
constexpr float GODOT_GROUND_NOTE_STEP = 0.01f;                              // a share or channel that moves less than this since the frame before writes no new log line
constexpr const char* GODOT_GROUND_NOTE_META = "ase_ground_noted";            // meta of the RenderingServer: the values the last log line named; Godot answers global_shader_parameter_get only in the editor

/**
 * The ground's season as GroundSeason::shade derives it for this frame (m.html groundCol 426 to 428). The defaults
 * leave the ground as its soils paint it: no season's colour, no darkening, no horizon blend.
 */
struct GroundSeasonShade {
    bool shown = false;                // false before the HUD stands: the ground keeps its cover
    ::godot::Color season;             // GROUND_SEASON_* of the season, toward GROUND_SNOW by ground_snow_share while it snows
    ::godot::Color snow = ::godot::Color(0.0f, 0.0f, 0.0f, 0.0f);  // GROUND_SNOW with ground_snow_share as alpha while it snows: the mockup whitens every row, so the snow lies on the bare soil and the banks as on the cover; alpha 0 otherwise
    float depth_floor = 1.0f;          // ground_depth_floor: the row on the horizon
    float depth_span = 0.0f;           // ground_depth_span: added toward the frame's foot, times z
    ::godot::Color horizon;            // the horizon's colour times ground_horizon_dim
    float horizon_share = 0.0f;        // ground_horizon_share: the blend on the horizon, times (1 - z)
    float foot_row = 0.0f;             // the screen row of the HUD frame's foot, pixels: z 1
    float row_power = 1.0f;            // ground_row_power: z = ((row - far) / (foot - far)) ^ (1 / power)
};

/**
 * The point of the square's far edge straight ahead of the camera of `scene`, at the height of the water table, the
 * square's middle at world `centre_x`, `centre_z` and its side `side_m`: the camera's view flattened onto the ground
 * from the middle by half a side. False, `edge` untouched, for a scene without camera, a camera looking straight
 * down or a square without side, and for an edge that does not lie ahead of the camera.
 */
bool plant_ground_far_edge(const ::godot::Node3D* scene, double centre_x, double centre_z, double side_m,
                           ::godot::Vector3& edge);

/**
 * Hands `shade` to every soil material at once (the GODOT_GROUND_* globals above): the season's colour and its share
 * of the cover, the darkening, the horizon's colour and share, and the rows of z: the screen row the camera of
 * `scene` sees `edge` on (plant_ground_far_edge) as z 0, the shade's foot row as z 1, both as shares of the
 * viewport's visible height, since the shader reads its row in render pixels and the canvas stretches. A shade not
 * shown, no edge (nullptr), a viewport without height or an edge that stands at or below the foot hands the defaults,
 * so the ground stays as its soils paint it. The view calls it every frame after light_plant_scene.
 */
void light_plant_ground_season(::godot::Node3D* scene, const GroundSeasonShade& shade, const ::godot::Vector3* edge);

}  // namespace ase::adp::godot
