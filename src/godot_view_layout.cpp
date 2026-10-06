/**
 * ASE GODOT ADAPTER - VIEW LAYOUT PLACEMENT IMPLEMENTATION
 *
 * @file        godot_view_layout.cpp
 * @brief       place_view, band_offset, in_one_band - the bands of the Vivarium layout on any surface
 * @description PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3: the Vivarium fills every handset of the
 *              market. The safe part of the visible surface takes the layout at a scale of at most
 *              one; what it has to spare in height goes half above and half below the middle band,
 *              so the header stays at the top and the buttons at the bottom; what it has to spare in
 *              width centres the column. Plain arithmetic, checked in tests/test_godot_view_layout.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-06
 * @modified    2026-10-06
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

namespace ase::adp::godot {

ViewPlacement place_view(const ViewBands& bands, const ViewSurface& surface) {
    ViewPlacement placement;
    const float safe_width = surface.width - surface.inset_left - surface.inset_right;
    const float safe_height = surface.height - surface.inset_top - surface.inset_bottom;
    if (bands.width <= 0.0f || bands.height <= 0.0f || safe_width <= 0.0f || safe_height <= 0.0f) {
        return placement;  // scale 0: nothing has room
    }
    placement.scale = ase::math::min(GODOT_LAYOUT_SCALE_MAX,
                                     ase::math::min(safe_width / bands.width, safe_height / bands.height));
    const float spare_width = safe_width - bands.width * placement.scale;
    const float spare_height = safe_height - bands.height * placement.scale;
    placement.offset_x = surface.inset_left + spare_width * GODOT_HALF;
    placement.top_y = surface.inset_top;
    placement.middle_y = surface.inset_top + spare_height * GODOT_HALF;
    placement.bottom_y = surface.inset_top + spare_height;
    return placement;
}

float band_offset(const ViewBands& bands, const ViewPlacement& placement, float layout_y) {
    if (layout_y < bands.top_end) {
        return placement.top_y;
    }
    if (layout_y < bands.bottom_start) {
        return placement.middle_y;
    }
    return placement.bottom_y;
}

bool in_one_band(const ViewBands& bands, float top, float bottom) {
    if (top > bottom || top < 0.0f || bottom > bands.height) {
        return false;
    }
    if (top < bands.top_end) {
        return bottom <= bands.top_end;
    }
    if (top < bands.bottom_start) {
        return bottom <= bands.bottom_start;
    }
    return true;
}

}  // namespace ase::adp::godot
