/**
 * ASE GODOT ADAPTER - HUD UI GROUND
 *
 * @file        godot_hud_ui_ground.cpp
 * @brief       The ground as the terrain port laid it, read at a world place: its height and its point on the HUD
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_ground.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_ui_ground.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

void UiGround::follow(const std::vector<VivariumTerrainView>& samples, const VivariumGroundSpec& square,
                      uint64_t generation) {
    samples_ = &samples;
    if (followed_ && generation == generation_) {
        return;
    }
    followed_ = true;
    generation_ = generation;
    step_ = square.step_m;
    origin_x_ = square.x - square.half_m;
    origin_z_ = square.z - square.half_m;
    side_ = step_ > 0.0 ? static_cast<int64_t>(::godot::Math::round((square.half_m + square.half_m) / step_)) + 1 : 0;
    index_.assign(static_cast<size_t>(side_ * side_), GODOT_HUD_GROUND_NO_SAMPLE);
    for (size_t sample = 0; sample < samples.size(); ++sample) {
        const int64_t column = lattice(samples[sample].x, origin_x_);
        const int64_t row = lattice(samples[sample].z, origin_z_);
        if (column < 0 || row < 0 || column >= side_ || row >= side_) {
            continue;
        }
        index_[static_cast<size_t>(row * side_ + column)] = static_cast<int64_t>(sample);
    }
}

int64_t UiGround::side() const {
    return side_;
}

int64_t UiGround::lattice(double world, double origin) const {
    return static_cast<int64_t>(::godot::Math::round((world - origin) / step_));
}

const VivariumTerrainView* UiGround::at(int64_t column, int64_t row) const {
    if (samples_ == nullptr || column < 0 || row < 0 || column >= side_ || row >= side_) {
        return nullptr;
    }
    const int64_t sample = index_[static_cast<size_t>(row * side_ + column)];
    if (sample == GODOT_HUD_GROUND_NO_SAMPLE || static_cast<size_t>(sample) >= samples_->size()) {
        return nullptr;
    }
    return &(*samples_)[static_cast<size_t>(sample)];
}

bool UiGround::height(double x, double z, double& out) const {
    if (side_ < 2) {
        return false;
    }
    const double fx = (x - origin_x_) / step_;
    const double fz = (z - origin_z_) / step_;
    const auto last = static_cast<double>(side_ - 1);
    if (fx < 0.0 || fz < 0.0 || fx > last || fz > last) {
        return false;
    }
    // On the far edge the last cell carries the place: its share there is 1, not 0 of a cell off the square.
    const auto column = ase::math::min(static_cast<int64_t>(::godot::Math::floor(fx)), side_ - 2);
    const auto row = ase::math::min(static_cast<int64_t>(::godot::Math::floor(fz)), side_ - 2);
    const VivariumTerrainView* sw = at(column, row);
    const VivariumTerrainView* se = at(column + 1, row);
    const VivariumTerrainView* nw = at(column, row + 1);
    const VivariumTerrainView* ne = at(column + 1, row + 1);
    if (sw == nullptr || se == nullptr || nw == nullptr || ne == nullptr) {
        return false;
    }
    const double ax = fx - static_cast<double>(column);
    const double az = fz - static_cast<double>(row);
    const double south = sw->height + (se->height - sw->height) * ax;
    const double north = nw->height + (ne->height - nw->height) * ax;
    out = south + (north - south) * az;
    return true;
}

bool UiGround::across(double share_x, double share_z, ::godot::Vector3& out) const {
    const double edge = static_cast<double>(side_ - 1) * step_;
    const double x = origin_x_ + share_x * edge;
    const double z = origin_z_ + share_z * edge;
    double ground = 0.0;
    if (!height(x, z, ground)) {
        return false;
    }
    out = ::godot::Vector3(static_cast<float>(x), static_cast<float>(ground), static_cast<float>(z));
    return true;
}

bool UiGround::surface(const UiAnchor& anchor, double x, double z, double lift, ::godot::Vector2& out) const {
    double ground = 0.0;
    if (!height(x, z, ground)) {
        return false;
    }
    return anchor.place(
        ::godot::Vector3(static_cast<float>(x), static_cast<float>(ground + lift), static_cast<float>(z)), out);
}

bool UiGround::ring(const UiAnchor& anchor, double x, double z, double radius, int64_t steps,
                    ::godot::PackedVector2Array& out) const {
    out.clear();
    if (steps <= 0) {
        return false;
    }
    for (int64_t step = 0; step <= steps; ++step) {
        // A full turn in radians is pi over a half.
        const double turn = static_cast<double>(step) / static_cast<double>(steps) * ase::math::PI / GODOT_HALF;
        ::godot::Vector2 point;
        if (!surface(anchor, x + ::godot::Math::cos(turn) * radius, z + ::godot::Math::sin(turn) * radius, 0.0, point)) {
            out.clear();
            return false;
        }
        out.push_back(point);
    }
    return true;
}

}  // namespace ase::adp::godot
