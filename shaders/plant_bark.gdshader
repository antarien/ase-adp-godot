/**
 * ASE GODOT ADAPTER - PLANT BARK
 *
 * @file        plant_bark.gdshader
 * @brief       The bark of the birch, white above and dark at the base
 * @description The bark of the internodes (usecase:100395 age the bark, DSGN_024): one material for the tubes and
 *              twigs that mixes the white bark with the stem-base bark by the share each instance carries in its
 *              custom data (INSTANCE_CUSTOM.r, GODOT_INSTANCE_CUSTOM_VALUES). Its uniforms take the colour, the
 *              albedo, normal and roughness textures, the roughness and the UV scale of the two bound materials
 *              (GODOT_ROLE_BARK, GODOT_ROLE_BARK_BASE); a texture a material lacks samples white, a flat normal or
 *              full roughness. The tiling of the section stands in the rest of the custom data (INSTANCE_CUSTOM.gba,
 *              computed by ase-render-data): the texture coordinates of the unit cylinder are multiplied by the tiles of
 *              the white bark around and along the tube, and those of the base bark once more by its tiles per white
 *              tile. An internode without a tile record carries GODOT_TILE_NONE, the scale of the material itself. It takes
 *              the swing of the branch by its two includes, sway_uniforms.gdshaderinc and sway_swing.gdshaderinc.
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
shader_type spatial;
#include "res://shaders/sway_uniforms.gdshaderinc"
uniform vec4 white_color : source_color = vec4(1.0);
uniform sampler2D white_albedo : source_color, hint_default_white, filter_linear_mipmap, repeat_enable;
uniform sampler2D white_normal : hint_normal, filter_linear_mipmap, repeat_enable;
uniform sampler2D white_rough : hint_default_white, filter_linear_mipmap, repeat_enable;
uniform float white_roughness = 1.0;
uniform vec2 white_uv = vec2(1.0);
uniform vec4 base_color : source_color = vec4(1.0);
uniform sampler2D base_albedo : source_color, hint_default_white, filter_linear_mipmap, repeat_enable;
uniform sampler2D base_normal : hint_normal, filter_linear_mipmap, repeat_enable;
uniform sampler2D base_rough : hint_default_white, filter_linear_mipmap, repeat_enable;
uniform float base_roughness = 1.0;
uniform vec2 base_uv = vec2(1.0);
varying float base_share;
varying vec3 tiles;
void vertex() {
    base_share = clamp(INSTANCE_CUSTOM.r, 0.0, 1.0);
    tiles = INSTANCE_CUSTOM.gba;
#include "res://shaders/sway_swing.gdshaderinc"
}
void fragment() {
    vec2 uw = UV * tiles.xy * white_uv;
    vec2 ub = UV * tiles.xy * tiles.z * base_uv;
    vec3 white = texture(white_albedo, uw).rgb * white_color.rgb;
    vec3 base = texture(base_albedo, ub).rgb * base_color.rgb;
    ALBEDO = mix(white, base, base_share);
    NORMAL_MAP = mix(texture(white_normal, uw).rgb, texture(base_normal, ub).rgb, base_share);
    ROUGHNESS = mix(texture(white_rough, uw).r * white_roughness, texture(base_rough, ub).r * base_roughness,
                    base_share);
}
