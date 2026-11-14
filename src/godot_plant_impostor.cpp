/**
 * ASE GODOT ADAPTER - IMPOSTOR OF A FAR PLANT IMPLEMENTATION
 *
 * @file        godot_plant_impostor.cpp
 * @brief       From which distance a body is drawn as an impostor card, and how large that card and its texture are
 * @description See godot_plant_impostor.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_plant_impostor.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The box around a body: the smallest and largest x, y and z of every point put into it. */
struct BodyBox {
    float low[3] = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()};
    float high[3] = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                     std::numeric_limits<float>::lowest()};
};

/** Widens `box` to the point (x, y, z). */
void widen(BodyBox& box, float x, float y, float z) {
    const float point[3] = {x, y, z};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        box.low[axis] = ase::math::min(box.low[axis], point[axis]);
        box.high[axis] = ase::math::max(box.high[axis], point[axis]);
    }
}

/** The pixels a length spans at the reach, whole and at least one. */
uint32_t pixels_of(float extent, float widest) {
    const float pixels = ase::math::ceil(extent / (2.0f * widest));
    return pixels < 1.0f ? 1u : static_cast<uint32_t>(pixels);
}

}  // namespace

PlantImpostorFrame impostor_of(const PlantBodyTube& tube, const std::vector<const std::vector<float>*>& rows,
                               float view_height, float tan_half_fov) {
    PlantImpostorFrame frame;
    // the widest ring of the tube: the third value of every custom1 record
    float widest = 0.0f;
    for (std::size_t at = 2; at < tube.custom1.size(); at += GODOT_BARK_CUSTOM_VALUES) {
        widest = ase::math::max(widest, tube.custom1[at]);
    }
    if (!(widest > 0.0f) || !(view_height > 0.0f) || !(tan_half_fov > 0.0f)) {
        return frame;
    }

    // the box around the vertices of the tube and the origins of the instance records
    BodyBox box;
    for (std::size_t at = 0; at + 2 < tube.arrays.positions.size(); at += 3) {
        widen(box, tube.arrays.positions[at], tube.arrays.positions[at + 1], tube.arrays.positions[at + 2]);
    }
    for (const std::vector<float>* buffer : rows) {
        if (buffer == nullptr) {
            continue;
        }
        for (std::size_t at = 0; at + GODOT_INSTANCE_VALUES <= buffer->size(); at += GODOT_INSTANCE_VALUES) {
            widen(box, (*buffer)[at + 3], (*buffer)[at + 7], (*buffer)[at + 11]);
        }
    }

    frame.reach = widest * view_height / tan_half_fov;
    frame.width = box.high[0] - box.low[0];
    frame.height = box.high[1] - box.low[1];
    frame.depth = box.high[2] - box.low[2];
    frame.centre_x = 0.5f * (box.low[0] + box.high[0]);
    frame.centre_y = 0.5f * (box.low[1] + box.high[1]);
    frame.centre_z = 0.5f * (box.low[2] + box.high[2]);
    frame.texture_width = pixels_of(frame.width, widest);
    frame.texture_height = pixels_of(frame.height, widest);
    return frame;
}

}  // namespace ase::adp::godot
