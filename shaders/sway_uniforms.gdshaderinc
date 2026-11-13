/**
 * ASE GODOT ADAPTER - SWAY UNIFORMS
 *
 * @file        sway_uniforms.gdshaderinc
 * @brief       The uniforms of the swing of a branch
 * @description THE SWING OF AN ORGAN, one building block for every shader that draws an organ on a branch (bark, stand-in
 *              and bound organ, usecase:101335 animate the branch sway). Its seven values stand in a data texture beside
 *              the instances, GODOT_SWAY_TEXELS per instance (fill_instances, godot_plant_scene.cpp): `sway_data` is that
 *              texture (GODOT_SWAY_DATA), `sway_row` the instances of one of its rows (GODOT_SWAY_ROW,
 *              GODOT_SWAY_ROW_INSTANCES). A shader takes this block into its head with
 *              #include "res://shaders/sway_uniforms.gdshaderinc" and the swing itself with sway_swing.gdshaderinc in its
 *              vertex(); an instance without a record, and every mesh without the texture, reads zeros and stands still.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_018
 * @category   render/pipeline/stage
 * @usecase    usecase:100387 - view the plant
 * @actor      Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.01.00000
 */
uniform sampler2D sway_data : hint_default_black, filter_nearest, repeat_disable;
uniform int sway_row = 1;
