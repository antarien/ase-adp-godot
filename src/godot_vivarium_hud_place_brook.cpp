/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD PLACES: THE BROOK
 *
 * @file        godot_vivarium_hud_place_brook.cpp
 * @brief       The point of the first brook's bed a tap in the mode water lands in, for the place lines
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.11: the search behind the place line `brook` of
 *              VivariumView::note_hud_places (godot_vivarium_hud_place.cpp), taken out of it as a unit of its own.
 *
 *              THE MIDDLE OF THE DRAWN LINE IS NO POINT OF THE BED. The line joins the ground under the brook's two
 *              ends, and the bed between them is cut below both; the camera's ray through that middle meets the
 *              ground beside the brook (run 20261113-v12-brook: "the tap lies 3.750000 m from brook", reach 1.6 m).
 *              So the middle is carried back to its share of the world's line - the screen share s over the ends'
 *              depths wa and wb is the world share s * wa / (wb * (1 - s) + s * wa) - and the ground there is
 *              placed again, the bed the brook runs in. A brook column without a landing pixel: no place line (the
 *              set's count names place lines only, and the acceptance takes exactly that many), but part of the
 *              set's change, so it is written once on its change.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.39.00039 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_hud_place_brook.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/math/math.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

bool brook_place(const UiGround& ground, const UiAnchor& anchor, const ::godot::Transform2D& frame,
                 const ::godot::Transform3D& eye, const ::godot::PackedVector2Array& line,
                 const VivariumStreamView& stream, const std::vector<VivariumTerrainView>& terrain, float pick_px,
                 ::godot::Rect2& tap, std::string& detail, std::string& unplaced) {
    detail.clear();
    unplaced.clear();
    const ::godot::Vector2 middle = (line[0] + line[line.size() - 1]) * GODOT_HALF;
    double head_y = 0.0;
    double tail_y = 0.0;
    ::godot::Vector2 head;
    ::godot::Vector2 tail;
    if (!(ground.height(stream.x, stream.z, head_y) && ground.height(stream.end_x, stream.end_z, tail_y) &&
          ground.surface(anchor, stream.x, stream.z, 0.0, head) &&
          ground.surface(anchor, stream.end_x, stream.end_z, 0.0, tail) && head != tail)) {
        return false;
    }
    const float wa = -eye.xform_inv(::godot::Vector3(static_cast<float>(stream.x), static_cast<float>(head_y),
                                                     static_cast<float>(stream.z))).z;
    const float wb = -eye.xform_inv(::godot::Vector3(static_cast<float>(stream.end_x), static_cast<float>(tail_y),
                                                     static_cast<float>(stream.end_z))).z;
    const ::godot::Vector2 run = tail - head;
    const float seen = (middle - head).dot(run) / run.length_squared();
    const float share = seen * wa / (wb * (1.0f - seen) + seen * wa);
    const double at_x = stream.x + (stream.end_x - stream.x) * share;
    const double at_z = stream.z + (stream.end_z - stream.z) * share;
    // THE PLACE IS WHERE A TAP LANDS IN THE BED, NOT WHERE THE BED LIES. A tap is resolved by pick_ground,
    // the terrain sample projected nearest to the finger, and the floor of a bed cut below the near bank
    // projects onto the same pixels as that bank: run 20261113-v12c-brook placed the floor at y=765, the
    // bank lay there, the tap found 16,18.125, 3.375 m short of the line. So the water's face (lift depth_m,
    // the surface terrain_kit draws the brook at, what the eye sees as water) is tried first and then the
    // floor, and a candidate stands only when pick_ground carries its own point back into the bed, within
    // half_m of the brook's line; the pick and its distance stand in the line, so a miss names itself.
    // A TAP LANDS ON A WHOLE PIXEL. At the float point of the floor pick_ground found the bed (run
    // 20261113-v12d-brook: pick=16,21.5 off=0), at the pixel the line names, 359,765, the bank (16,18.125):
    // under a grazing eye the floor and the near bank project less than a pixel apart. So the place is
    // chosen from the column of whole pixels through the water's face, from the ground half_m before the
    // line to the ground half_m behind it (the drawn width of the brook), each pixel resolved as a tap is
    // (the nearest projected sample within pick_px, a later one winning a tie), and it is the middle of
    // the longest run of pixels that land within half_m of the line, the pixel with the widest margin
    // either way. One pass projects every sample once and keeps the nearest per pixel; `run=` names the
    // margin, and a column without a landing pixel names itself in the set (`brook none`).
    const float run_x = static_cast<float>(stream.end_x - stream.x);
    const float run_z = static_cast<float>(stream.end_z - stream.z);
    const float span = run_x * run_x + run_z * run_z;
    const double across = span > 0.0f ? stream.half_m / static_cast<double>(ase::math::sqrt(span)) : 0.0;
    ::godot::Vector2 face;
    ::godot::Vector2 before;
    ::godot::Vector2 behind;
    if (!(span > 0.0f && ground.surface(anchor, at_x, at_z, stream.depth_m, face) &&
          ground.surface(anchor, at_x - run_z * across, at_z + run_x * across, 0.0, before) &&
          ground.surface(anchor, at_x + run_z * across, at_z - run_x * across, 0.0, behind))) {
        return false;
    }
    const ::godot::Vector2 seen_face = frame.xform(face);
    const float seen_before = frame.xform(before).y;
    const float seen_behind = frame.xform(behind).y;
    const float column_x = ase::math::floor(seen_face.x + GODOT_HALF);
    const float top = ase::math::floor(ase::math::min(seen_face.y, ase::math::min(seen_before, seen_behind)));
    const float foot = ase::math::ceil(ase::math::max(seen_face.y, ase::math::max(seen_before, seen_behind)));
    const size_t count = static_cast<size_t>(foot - top) + 1;
    std::vector<float> best_px(count, -1.0f);
    std::vector<float> best_x(count, 0.0f);
    std::vector<float> best_z(count, 0.0f);
    // Each sample through the camera of this frame that UiAnchor::follow read once (P0694), back onto the
    // canvas through the HUD's frame: the same pixel Camera3D::unproject_position gives, without two calls
    // across the extension boundary per sample.
    for (const VivariumTerrainView& sample : terrain) {
        const ::godot::Vector3 place(static_cast<float>(sample.x), static_cast<float>(sample.height),
                                     static_cast<float>(sample.z));
        ::godot::Vector2 framed;
        if (!anchor.place(place, framed)) {
            continue;
        }
        const ::godot::Vector2 on_canvas = frame.xform(framed);
        if (ase::math::abs(on_canvas.x - column_x) > pick_px) {
            continue;
        }
        for (size_t k = 0; k < count; ++k) {
            const float px = on_canvas.distance_to(::godot::Vector2(column_x, top + static_cast<float>(k)));
            if (px <= pick_px && (best_px[k] < 0.0f || px <= best_px[k])) {
                best_px[k] = px;
                best_x[k] = static_cast<float>(sample.x);
                best_z[k] = static_cast<float>(sample.z);
            }
        }
    }
    size_t run_from = 0;
    size_t run_length = 0;
    size_t open_from = 0;
    size_t open_length = 0;
    for (size_t k = 0; k < count; ++k) {
        bool lands = false;
        if (!(best_px[k] < 0.0f)) {
            const float along = ase::math::min(
                1.0f, ase::math::max(0.0f, ((best_x[k] - static_cast<float>(stream.x)) * run_x +
                                            (best_z[k] - static_cast<float>(stream.z)) * run_z) / span));
            const float off_x = best_x[k] - (static_cast<float>(stream.x) + run_x * along);
            const float off_z = best_z[k] - (static_cast<float>(stream.z) + run_z * along);
            lands = static_cast<double>(ase::math::sqrt(off_x * off_x + off_z * off_z)) <= stream.half_m;
        }
        open_from = lands && open_length == 0 ? k : open_from;
        open_length = lands ? open_length + 1 : 0;
        if (open_length > run_length) {
            run_from = open_from;
            run_length = open_length;
        }
    }
    if (run_length == 0) {
        unplaced = "debug brook none column x=" + std::to_string(static_cast<int64_t>(column_x)) +
                   " y=" + std::to_string(static_cast<int64_t>(top)) + ".." +
                   std::to_string(static_cast<int64_t>(foot)) + " lands no pixel within half=" +
                   std::to_string(stream.half_m) + " of the brook";
        return false;
    }
    const size_t middle_k = run_from + run_length / 2;
    // place_line cuts toward zero, so the rect stands half a pixel inside the tapped one and the round trip through
    // the frame's inverse cannot fall to the pixel before it.
    const ::godot::Vector2 pixel(column_x + GODOT_HALF, top + static_cast<float>(middle_k) + GODOT_HALF);
    tap = ::godot::Rect2(frame.affine_inverse().xform(pixel), ::godot::Vector2());
    detail = " pick=" + std::to_string(best_x[middle_k]) + "," + std::to_string(best_z[middle_k]) +
             " run=" + std::to_string(run_length) + "/" + std::to_string(count) +
             " half=" + std::to_string(stream.half_m);
    return true;
}

}  // namespace ase::adp::godot
