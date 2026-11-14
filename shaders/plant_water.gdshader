/**
 * ASE GODOT ADAPTER - PLANT WATER
 *
 * @file        plant_water.gdshader
 * @brief       The water ribbon of a brook, with the sky it reflects and the strokes of its current
 * @description The water of the care view (usecase:101339 - show the ground around plant): the plane in the bed
 *              of every brook at its bed plus the depth ase-hydro gives the brook, and no water where no bed lies
 *              (build_water_table, godot_plant_water.hpp; B0281). The vertex
 *              colour carries the water in the range a colour holds: red the velocity of the stream whose bed holds the
 *              vertex over `velocity_full` (GODOT_WATER_VELOCITY_FULL_M_S), green the depth of the table over
 *              GODOT_WATER_DEPTH_FULL_M, blue and alpha the course of that stream. The velocity moves the ripples along
 *              the course, the depth turns the shallow colour to the deep one; the view computes no water. The two
 *              colours are those of brook water, not of the sea: murky, dark, and slightly green from the algae and the
 *              suspended silt a lowland brook carries; the shallow colour shows the bed through it.
 *              Over that own colour lies the sky the brook reflects (PLAN_ASE_VIVARIUM_PHASE_06_HUD row N07, m.html
 *              drawWater 429 to 433): the far colour at depth z 0, the near colour at depth z 1, z a row of the screen
 *              measured as the ground measures it (the global ground_rows light_plant_ground_season sets), so water
 *              and ground rows share one depth. The mockup fills opaquely; the operator's order keeps the water murky green and lets
 *              the reflection show only where there is light to mirror, so the reflection is unlit light ADDED on top
 *              (brook_reflect_mix, the token brook_reflect_share, times the reflected sky) and never takes anything of
 *              the own colour, its opacity or its gloss: a dark night sky adds next to nothing, the sun and the moon
 *              mirror themselves through the gloss the scene's lights throw. The strokes are glints weighted by the
 *              brightness of the sky they mirror. Along the course brook_start to brook_end run brook_streak.x strokes, the k-th starting at
 *              fract(k * gap + TIME * speed) of the course and covering `length` of it without wrapping past its end,
 *              brook_streak_px wide on the screen, in brook_streak_color over its alpha. Across, a stroke stands on the
 *              axis of the winding bed, not on the straight line between the ends, which the bed leaves by up to its
 *              width: the axis is the middle of the wet strip, whose rims the shader finds in the depth texture of the
 *              ground within brook_half of the surface point (the stream record's half width). The defaults of the globals
 *              (project.godot) leave the water as its own colours paint it.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_066
 * @category   render/pipeline/stage
 * @usecase    usecase:101339 - show the ground around plant
 * @actor      Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-11
 * @modified    2026-11-12
 * @version     00.00.02.00003
 */
shader_type spatial;
render_mode cull_back;
uniform vec4 shallow_color : source_color = vec4(0.15, 0.23, 0.17, 1.0);
uniform vec4 deep_color : source_color = vec4(0.05, 0.12, 0.10, 1.0);
uniform float velocity_full = 2.0;
// the exposure lift of the frame (light_plant_environment): an unlit colour is divided by it to stand as drawn
global uniform float scene_lift;
// the rows of the ground's depth (light_plant_ground_season, row N06): the screen row of z 0 (the square's far edge),
// of z 1 (the HUD frame's foot), and the power of the mockup's rowY
global uniform vec3 ground_rows;
// the brook's reflection and strokes (light_plant_brook_reflection, row N07)
global uniform vec4 brook_far_color;
global uniform vec4 brook_near_color;
global uniform float brook_reflect_mix;
global uniform vec4 brook_streak_color;
global uniform vec4 brook_streak;
global uniform float brook_streak_px;
global uniform vec2 brook_start;
global uniform vec2 brook_end;
global uniform float brook_half;
// the ground behind the water, for the axis of the bed the strokes run on (row N07, register B0263)
uniform sampler2D ground_depth : hint_depth_texture, filter_nearest;
// the walk to a rim of the wet strip: RIM_STEPS even steps over RIM_REACH half widths (the winding bed cut along the
// line's normal is up to 1.2 of its width wide, and a point of the strip lies up to that far from either rim), then
// RIM_REFINE halvings of the last step, which leave a thousandth of a half width
const int RIM_STEPS = 24;
const float RIM_REACH = 3.0;
const int RIM_REFINE = 7;
varying float flow;
varying float deep;
varying vec2 course;
varying vec3 world_pos;
float ripple_hash(vec2 p) { vec3 q = fract(vec3(p.xyx) * 0.1031); q += dot(q, q.yzx + 33.33); return fract((q.x + q.y) * q.z); }
float ripple_at(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(ripple_hash(i), ripple_hash(i + vec2(1.0, 0.0)), u.x),
               mix(ripple_hash(i + vec2(0.0, 1.0)), ripple_hash(i + vec2(1.0, 1.0)), u.x), u.y);
}
// The view distance of the ground behind the water at the screen place `uv` (reversed depth, Vulkan's NDC). Read at
// level 0: the walk below runs inside a branch, where a texture read has no derivatives.
float ground_distance(vec2 uv, mat4 inv_projection) {
    vec4 view = inv_projection * vec4(uv * 2.0 - 1.0, textureLod(ground_depth, uv, 0.0).r, 1.0);
    return -view.z / view.w;
}
// True where the world point `at` on the table lies over ground below it: the ground the camera sees through that
// point lies farther than the point itself. A point off the screen or behind the camera counts as dry.
bool wet_at(vec3 at, mat4 view, mat4 projection, mat4 inv_projection) {
    vec4 eye = view * vec4(at, 1.0);
    vec4 clip = projection * eye;
    if (clip.w <= 0.0) {
        return false;
    }
    vec2 uv = clip.xy / clip.w * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) {
        return false;
    }
    return ground_distance(uv, inv_projection) > -eye.z;
}
// The distance from the wet point `from` along the level direction `side` to the rim of the wet strip, at most
// `reach`: even steps until the first dry point, then halvings between the last wet and the first dry one.
float rim_distance(vec3 from, vec3 side, float reach, mat4 view, mat4 projection, mat4 inv_projection) {
    float wet = 0.0;
    float dry = reach;
    bool found = false;
    for (int i = 1; i <= RIM_STEPS && !found; i++) {
        float s = reach * float(i) / float(RIM_STEPS);
        if (wet_at(from + side * s, view, projection, inv_projection)) {
            wet = s;
        } else {
            dry = s;
            found = true;
        }
    }
    if (!found) {
        return reach;
    }
    for (int k = 0; k < RIM_REFINE; k++) {
        float middle = 0.5 * (wet + dry);
        if (wet_at(from + side * middle, view, projection, inv_projection)) {
            wet = middle;
        } else {
            dry = middle;
        }
    }
    return 0.5 * (wet + dry);
}
// 1 where the share `share` of the course lies on one of the strokes of the current, else 0 (m.html drawWater 433:
// the k-th stroke starts at (k * 0.23 + t * 0.02) mod 1 of the course and covers six steps of 0.008 of it). The share
// is the one of the line from brook_start to brook_end, the t ase-terrain's bed winds by (TerrainMutCarvSystem), so
// a stroke stands where the mockup's u puts it; across, the fragment shader keeps it on the axis of the bed.
float streak_at(float share) {
    if (brook_streak_px <= 0.0 || share < 0.0 || share > 1.0) {
        return 0.0;
    }
    int count = int(brook_streak.x);
    for (int k = 0; k < count; k++) {
        float begin = fract(float(k) * brook_streak.y + TIME * brook_streak.w);
        if (share >= begin && share <= min(begin + brook_streak.z, 1.0)) {
            return 1.0;
        }
    }
    return 0.0;
}
void vertex() {
    flow = COLOR.r * velocity_full;
    deep = COLOR.g;
    course = vec2(COLOR.b, COLOR.a) * 2.0 - 1.0;
    world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}
void fragment() {
    vec2 dir = length(course) > 0.001 ? normalize(course) : vec2(0.0, 1.0);
    vec2 frame = vec2(dot(UV, vec2(dir.y, -dir.x)), dot(UV, dir));
    vec2 at = vec2(frame.x * 3.0, (frame.y - TIME * flow) * 1.6);
    float step_size = 0.08;
    float here = ripple_at(at);
    float across = ripple_at(at + vec2(step_size, 0.0)) - here;
    float along = ripple_at(at + vec2(0.0, step_size)) - here;
    NORMAL = normalize(NORMAL + (VIEW_MATRIX * vec4(across, 0.0, along, 0.0)).xyz * 1.5);
    // the reflection is light on top of the water, never a change of it: the murky green own colour, its opacity
    // and its gloss stay as they are, and the reflected sky only adds what it carries. A dark night sky adds next to
    // nothing; the sun and the moon mirror themselves through the gloss the scene's real lights throw
    float reflect_share = clamp(brook_reflect_mix, 0.0, 1.0);
    float row = clamp((FRAGCOORD.y / max(VIEWPORT_SIZE.y, 1.0) - ground_rows.x) / max(ground_rows.y - ground_rows.x, 0.00001), 0.0, 1.0);
    float depth = pow(row, 1.0 / max(ground_rows.z, 0.00001));
    vec3 sky = mix(brook_far_color.rgb, brook_near_color.rgb, depth);
    // the strokes are glints of that sky: as bright as the sky they mirror, gone when it is dark
    float sky_light = clamp(dot(sky, vec3(0.2126, 0.7152, 0.0722)), 0.0, 1.0);
    // THE STROKES RUN ON THE AXIS OF THE BED, as the mockup draws them along zb(u), the middle of its water. The bed
    // winds off the line from start to end by up to its width (TerrainMutCarvSystem), so that line misses the water.
    // Its section is symmetric about the axis and meets the table at both rims, so the axis is the middle of the wet
    // strip. From the fragment's point on the table the shader walks level along the line's normal to either side
    // until the ground no longer lies below the table (rim_distance); the point lies on the axis when the two
    // distances differ by no more than the stroke's width, brook_streak_px across the course on the screen turned
    // into metres along that normal. The water column behind the surface does not tell the axis: from the low camera
    // the ray through the water just past the near rim runs on to the far slope and is the longest of all
    // (acceptance/20261112-182727-b0263-brook4, the brightest water pixels on the near rim).
    vec2 axis = brook_end - brook_start;
    float axis_sq = dot(axis, axis);
    float share = axis_sq > 0.0 ? dot(world_pos.xz - brook_start, axis) / axis_sq : -1.0;
    vec2 axis_dir = axis_sq > 0.0 ? axis / sqrt(axis_sq) : vec2(0.0, 1.0);
    float on_axis = 0.0;
    if (brook_half > 0.0 && streak_at(share) > 0.0) {
        vec3 side = vec3(-axis_dir.y, 0.0, axis_dir.x);
        mat4 to_clip = PROJECTION_MATRIX * VIEW_MATRIX;
        vec4 clip_here = to_clip * vec4(world_pos, 1.0);
        vec4 clip_ahead = to_clip * vec4(world_pos + vec3(axis_dir.x, 0.0, axis_dir.y), 1.0);
        vec4 clip_side = to_clip * vec4(world_pos + side, 1.0);
        vec2 here_px = clip_here.xy / clip_here.w * 0.5 * VIEWPORT_SIZE;
        vec2 run_px = clip_ahead.xy / clip_ahead.w * 0.5 * VIEWPORT_SIZE - here_px;
        vec2 side_px = clip_side.xy / clip_side.w * 0.5 * VIEWPORT_SIZE - here_px;
        vec2 across_unit = length(run_px) > 0.0 ? vec2(-run_px.y, run_px.x) / length(run_px) : vec2(0.0, 1.0);
        float px_per_m = max(abs(dot(side_px, across_unit)), 0.0001);
        float width_m = brook_streak_px / px_per_m;
        float reach = brook_half * RIM_REACH;
        float to_left = rim_distance(world_pos, side, reach, VIEW_MATRIX, PROJECTION_MATRIX, INV_PROJECTION_MATRIX);
        float to_right = rim_distance(world_pos, -side, reach, VIEW_MATRIX, PROJECTION_MATRIX, INV_PROJECTION_MATRIX);
        on_axis = abs(to_left - to_right) <= width_m ? 1.0 : 0.0;
    }
    float stroke = on_axis * brook_streak_color.a * sky_light;
    vec3 own = mix(shallow_color.rgb, deep_color.rgb, clamp(deep, 0.0, 1.0));
    ALBEDO = own;
    EMISSION = (sky * reflect_share + brook_streak_color.rgb * stroke) / max(scene_lift, 1.0);
    ALPHA = mix(0.82, 0.95, clamp(deep, 0.0, 1.0));
    ROUGHNESS = 0.18;
    SPECULAR = 0.6;
}
