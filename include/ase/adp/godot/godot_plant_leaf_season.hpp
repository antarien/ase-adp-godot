#pragma once

/**
 * ASE GODOT ADAPTER - THE LEAF OF THE CARE VIEW IN THE COLOUR OF ITS SEASON
 *
 * @file        godot_plant_leaf_season.hpp
 * @brief       The season's colour and the dry olive the leaf of the care view's 3D scene is painted in
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, register B0278. The mockup paints the leaf with leafCol
 *              (m.html 486): the colour of the season, turned toward an olive while the soil lies dry. LeafSeason
 *              (godot_hud_leaf_season.hpp) derives both as the shade; this unit hands them to every leaf material at
 *              once as two global shader parameters. The stand-in and the organ shader read them only on the leaf's
 *              material (GODOT_LEAF_SEASON, godot_plant_look_shader.hpp), and the dry olive only where the instance
 *              uniform condition names a wilting stand, which the scene sets from the patch every frame. Whether a
 *              leaf hangs at all in a season is the plant's (PhytomerLeafFallSystem), never this unit's. The mockup's
 *              light() multiplies the colour as well; in the 3D scene the sun and the ambient light of the sky port
 *              light the leaf, so the shade carries no light.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.01.00000
 */

#include <godot_cpp/variant/color.hpp>

namespace ase::adp::godot {

// The global shader parameters of the leaf's season; project.godot declares each under [shader_globals], since a
// shader that reads a global the project does not declare does not compile. They stand here and not in types.hpp,
// which nearly every unit of the adapter includes.
constexpr const char* GODOT_LEAF_SEASON_COLOR = "leaf_season_color";  // global: the season's colour, its alpha the share it takes of the leaf
constexpr const char* GODOT_LEAF_DRY_COLOR = "leaf_dry_color";        // global: the dry olive, its alpha the share a wilting leaf turns toward it
constexpr float GODOT_LEAF_NOTE_STEP = 0.01f;                         // a channel that moves less than this since the frame before writes no new log line
constexpr const char* GODOT_LEAF_NOTE_META = "ase_leaf_noted";        // meta of the RenderingServer: the values the last log line named; Godot answers global_shader_parameter_get only in the editor

/**
 * The leaf's season as LeafSeason::shade derives it for this frame (m.html leafCol 486). The defaults leave the leaf
 * as its material paints it: no season's colour, no dry olive.
 */
struct LeafSeasonShade {
    bool shown = false;                                            // false before the HUD stands: the leaf keeps its own colour
    ::godot::Color season = ::godot::Color(0.0f, 0.0f, 0.0f, 0.0f);  // LEAF_SEASON_* of the season; alpha 0 lays no colour over the leaf
    ::godot::Color dry = ::godot::Color(0.0f, 0.0f, 0.0f, 0.0f);     // LEAF_DRY with leaf_dry_share as alpha; alpha 0 leaves a wilting leaf in the wilted colour
};

/**
 * Hands `shade` to every leaf material at once (the GODOT_LEAF_* globals above). A shade not shown hands the
 * defaults, so the leaf stays as its material paints it. The view calls it every frame after light_plant_scene.
 */
void light_plant_leaf_season(const LeafSeasonShade& shade);

}  // namespace ase::adp::godot
