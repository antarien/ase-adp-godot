/**
 * ASE GODOT ADAPTER - HUD UI ITEM SHADOW
 *
 * @file        godot_hud_ui_item_shadow.cpp
 * @brief       Rasters a drawing's mask, blurs it by the Gaussian of drop-shadow and keeps it as a texture
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_item_shadow.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_item_shadow.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** The first sample index at or after the position `at` along an axis of `count` samples (sample centres at +0.5). */
int64_t first_sample(double at, int64_t count) {
    const auto index = static_cast<int64_t>(::godot::Math::ceil(at - GODOT_HALF));
    return ase::math::clamp(index, static_cast<int64_t>(0), count);
}

/**
 * Marks every sample of `hit` (`across` x `down`) whose centre lies within `reach` of the segment from `a` to `b`,
 * all in samples: a stroke with round caps and round joins is the union of these capsules.
 */
void mark_stroke(std::vector<uint8_t>& hit, int64_t across, int64_t down, const ::godot::Vector2& a,
                 const ::godot::Vector2& b, double reach) {
    const double ax = a.x;
    const double ay = a.y;
    const double bx = b.x;
    const double by = b.y;
    const int64_t left = first_sample(ase::math::min(ax, bx) - reach, across);
    const int64_t right = first_sample(ase::math::max(ax, bx) + reach, across);
    const int64_t top = first_sample(ase::math::min(ay, by) - reach, down);
    const int64_t bottom = first_sample(ase::math::max(ay, by) + reach, down);
    const double dx = bx - ax;
    const double dy = by - ay;
    const double length2 = dx * dx + dy * dy;
    const double reach2 = reach * reach;
    for (int64_t row = top; row < bottom; ++row) {
        const double py = static_cast<double>(row) + GODOT_HALF;
        for (int64_t column = left; column < right; ++column) {
            const double px = static_cast<double>(column) + GODOT_HALF;
            const double along =
                length2 > 0.0 ? ase::math::clamp(((px - ax) * dx + (py - ay) * dy) / length2, 0.0, 1.0) : 0.0;
            const double ex = ax + along * dx - px;
            const double ey = ay + along * dy - py;
            if (ex * ex + ey * ey <= reach2) {
                hit[static_cast<size_t>(row * across + column)] = 1;
            }
        }
    }
}

/**
 * Fills every sample of `hit` (`across` x `down`) inside the runs of one path by the nonzero rule, the SVG default:
 * each run closes from its last point to its first, a sample row's crossings are taken in order of x and the spans
 * where the sum of their directions is not 0 are inside.
 */
void fill_nonzero(std::vector<uint8_t>& hit, int64_t across, int64_t down,
                  const std::vector<::godot::PackedVector2Array>& runs) {
    std::vector<double> xs;
    std::vector<int64_t> turns;
    for (int64_t row = 0; row < down; ++row) {
        const double y = static_cast<double>(row) + GODOT_HALF;
        xs.clear();
        turns.clear();
        for (const ::godot::PackedVector2Array& run : runs) {
            const int64_t count = run.size();
            for (int64_t index = 0; index < count; ++index) {
                const ::godot::Vector2 a = run[index];
                const ::godot::Vector2 b = run[(index + 1) % count];
                const bool downward = b.y > a.y;
                const double low = downward ? a.y : b.y;
                const double high = downward ? b.y : a.y;
                if (!(y >= low && y < high)) {
                    continue;
                }
                const double x =
                    a.x + (y - a.y) * (static_cast<double>(b.x) - a.x) / (static_cast<double>(b.y) - a.y);
                // In order of x as it comes: a row crosses a drawing a handful of times.
                size_t at = xs.size();
                xs.push_back(x);
                turns.push_back(downward ? 1 : -1);
                while (at > 0 && xs[at - 1] > xs[at]) {
                    const double moved_x = xs[at - 1];
                    const int64_t moved_turn = turns[at - 1];
                    xs[at - 1] = xs[at];
                    turns[at - 1] = turns[at];
                    xs[at] = moved_x;
                    turns[at] = moved_turn;
                    --at;
                }
            }
        }
        int64_t winding = 0;
        for (size_t index = 0; index + 1 < xs.size(); ++index) {
            winding += turns[index];
            if (winding == 0) {
                continue;
            }
            const int64_t from = first_sample(xs[index], across);
            const int64_t to = first_sample(xs[index + 1], across);
            for (int64_t column = from; column < to; ++column) {
                hit[static_cast<size_t>(row * across + column)] = 1;
            }
        }
    }
}

/**
 * Blurs `values` (`across` x `down`) by a Gaussian of `sigma` pixels cut at `radius`, along the rows and then the
 * columns (the Gaussian separates); what lies outside the mask counts as 0, the mask's margin holds the reach.
 */
void blur_gaussian(std::vector<float>& values, int64_t across, int64_t down, double sigma, int64_t radius) {
    if (!(sigma > 0.0) || radius <= 0) {
        return;
    }
    std::vector<double> kernel(static_cast<size_t>(radius + radius + 1), 0.0);
    double total = 0.0;
    for (int64_t step = -radius; step <= radius; ++step) {
        const double weight = ::godot::Math::exp(-static_cast<double>(step * step) / (2.0 * sigma * sigma));
        kernel[static_cast<size_t>(step + radius)] = weight;
        total += weight;
    }
    for (double& weight : kernel) {
        weight /= total;
    }
    std::vector<float> pass(values.size(), 0.0f);
    for (int64_t row = 0; row < down; ++row) {
        for (int64_t column = 0; column < across; ++column) {
            double sum = 0.0;
            for (int64_t step = -radius; step <= radius; ++step) {
                const int64_t from = column + step;
                if (from >= 0 && from < across) {
                    sum += values[static_cast<size_t>(row * across + from)] * kernel[static_cast<size_t>(step + radius)];
                }
            }
            pass[static_cast<size_t>(row * across + column)] = static_cast<float>(sum);
        }
    }
    for (int64_t row = 0; row < down; ++row) {
        for (int64_t column = 0; column < across; ++column) {
            double sum = 0.0;
            for (int64_t step = -radius; step <= radius; ++step) {
                const int64_t from = row + step;
                if (from >= 0 && from < down) {
                    sum += pass[static_cast<size_t>(from * across + column)] * kernel[static_cast<size_t>(step + radius)];
                }
            }
            values[static_cast<size_t>(row * across + column)] = static_cast<float>(sum);
        }
    }
}

/** True when `a` and `b` differ by no more than GODOT_ITEM_SHADOW_SAME along either axis. */
bool same_axis(const ::godot::Vector2& a, const ::godot::Vector2& b) {
    return ::godot::Math::abs(a.x - b.x) <= GODOT_ITEM_SHADOW_SAME &&
           ::godot::Math::abs(a.y - b.y) <= GODOT_ITEM_SHADOW_SAME;
}

/** A colour channel 0..1 as a byte. */
uint8_t channel_byte(double value) {
    return static_cast<uint8_t>(::godot::Math::round(ase::math::clamp(value, 0.0, 1.0) * GODOT_ITEM_SHADOW_BYTE));
}

}  // namespace

const UiItemSpriteShadow& UiItemShadow::of(const std::vector<UiItemSpritePath>& paths, double stroke_scale,
                                           const ::godot::Transform2D& local, double scale, double spread,
                                           const ::godot::Color& shade) {
    for (const UiItemSpriteShadow& cast : kept_) {
        if (same_axis(cast.axis_x, local.columns[0]) && same_axis(cast.axis_y, local.columns[1]) &&
            ::godot::Math::abs(cast.scale - scale) <= GODOT_ITEM_SHADOW_SAME &&
            ::godot::Math::abs(cast.spread - spread) <= GODOT_ITEM_SHADOW_SAME && cast.shade == shade) {
            return cast;
        }
    }
    if (static_cast<int64_t>(kept_.size()) >= GODOT_ITEM_SHADOW_KEPT) {
        kept_.erase(kept_.begin());
    }
    UiItemSpriteShadow cast;
    cast.axis_x = local.columns[0];
    cast.axis_y = local.columns[1];
    cast.scale = scale;
    cast.spread = spread;
    cast.shade = shade;
    raster(paths, stroke_scale, cast);
    kept_.push_back(cast);
    return kept_.back();
}

void UiItemShadow::clear() {
    kept_.clear();
}

void UiItemShadow::raster(const std::vector<UiItemSpritePath>& paths, double stroke_scale,
                          UiItemSpriteShadow& cast) const {
    if (!(cast.scale > 0.0)) {
        return;
    }
    const ::godot::Transform2D basis(cast.axis_x, cast.axis_y, ::godot::Vector2());
    // The drawing's bounds under the axes in frame units, and the widest half stroke.
    bool any = false;
    ::godot::Vector2 low;
    ::godot::Vector2 high;
    double half = 0.0;
    for (const UiItemSpritePath& path : paths) {
        if (path.stroked) {
            half = ase::math::max(half, path.width * stroke_scale * GODOT_HALF);
        }
        for (const UiSvgOutline& outline : path.outlines) {
            for (int64_t index = 0; index < outline.points.size(); ++index) {
                const ::godot::Vector2 point = basis.xform(outline.points[index]);
                low = any ? low.min(point) : point;
                high = any ? high.max(point) : point;
                any = true;
            }
        }
    }
    if (!any) {
        return;
    }
    const double sigma = cast.spread * GODOT_ITEM_SHADOW_SIGMA_PER_BLUR;
    const auto margin = static_cast<float>(half + sigma * GODOT_ITEM_SHADOW_SIGMA_REACH);
    cast.corner = low - ::godot::Vector2(margin, margin);
    const ::godot::Vector2 span = high - low + ::godot::Vector2(margin + margin, margin + margin);
    cast.width = static_cast<int64_t>(::godot::Math::ceil(static_cast<double>(span.x) * cast.scale));
    cast.height = static_cast<int64_t>(::godot::Math::ceil(static_cast<double>(span.y) * cast.scale));
    if (cast.width <= 0 || cast.height <= 0 || cast.width > GODOT_ITEM_SHADOW_PIXEL_MAX ||
        cast.height > GODOT_ITEM_SHADOW_PIXEL_MAX) {
        return;
    }
    // One pixel of the texture is one pixel of the frame.
    cast.size = ::godot::Vector2(static_cast<float>(static_cast<double>(cast.width) / cast.scale),
                                 static_cast<float>(static_cast<double>(cast.height) / cast.scale));

    // The mask in samples: every path's fill by the nonzero rule and its stroke at its width, counted once.
    const double per = cast.scale * static_cast<double>(GODOT_ITEM_SHADOW_SAMPLES);  // samples per frame unit
    const int64_t across = cast.width * GODOT_ITEM_SHADOW_SAMPLES;
    const int64_t down = cast.height * GODOT_ITEM_SHADOW_SAMPLES;
    std::vector<uint8_t> hit(static_cast<size_t>(across * down), 0);
    std::vector<::godot::PackedVector2Array> runs;
    for (const UiItemSpritePath& path : paths) {
        runs.clear();
        const double reach = path.width * stroke_scale * GODOT_HALF * per;
        for (const UiSvgOutline& outline : path.outlines) {
            ::godot::PackedVector2Array run;
            for (int64_t index = 0; index < outline.points.size(); ++index) {
                run.push_back((basis.xform(outline.points[index]) - cast.corner) * static_cast<float>(per));
            }
            if (path.stroked && reach > 0.0 && run.size() > 0) {
                for (int64_t index = 0; index + 1 < run.size(); ++index) {
                    mark_stroke(hit, across, down, run[index], run[index + 1], reach);
                }
                // A closed run's last segment, and a single point's round cap.
                mark_stroke(hit, across, down, run[run.size() - 1], outline.closed ? run[0] : run[run.size() - 1],
                            reach);
            }
            runs.push_back(run);
        }
        if (path.filled) {
            fill_nonzero(hit, across, down, runs);
        }
    }

    // The coverage of each pixel from its samples, then the Gaussian of drop-shadow.
    const auto share = static_cast<float>(1.0 / static_cast<double>(GODOT_ITEM_SHADOW_SAMPLES * GODOT_ITEM_SHADOW_SAMPLES));
    std::vector<float> cover(static_cast<size_t>(cast.width * cast.height), 0.0f);
    for (int64_t row = 0; row < down; ++row) {
        for (int64_t column = 0; column < across; ++column) {
            if (hit[static_cast<size_t>(row * across + column)] != 0) {
                const int64_t pixel =
                    (row / GODOT_ITEM_SHADOW_SAMPLES) * cast.width + column / GODOT_ITEM_SHADOW_SAMPLES;
                cover[static_cast<size_t>(pixel)] += share;
            }
        }
    }
    blur_gaussian(cover, cast.width, cast.height, sigma * cast.scale,
                  static_cast<int64_t>(::godot::Math::ceil(sigma * GODOT_ITEM_SHADOW_SIGMA_REACH * cast.scale)));

    // The shade's colour in every pixel, its alpha times the coverage.
    ::godot::PackedByteArray bytes;
    bytes.resize(cast.width * cast.height * GODOT_ITEM_SHADOW_CHANNELS);
    uint8_t* out = bytes.ptrw();
    const uint8_t red = channel_byte(cast.shade.r);
    const uint8_t green = channel_byte(cast.shade.g);
    const uint8_t blue = channel_byte(cast.shade.b);
    for (int64_t pixel = 0; pixel < cast.width * cast.height; ++pixel) {
        uint8_t* at = out + pixel * GODOT_ITEM_SHADOW_CHANNELS;
        at[0] = red;
        at[1] = green;
        at[2] = blue;
        at[3] = channel_byte(static_cast<double>(cover[static_cast<size_t>(pixel)]) * cast.shade.a);
    }
    const ::godot::Ref<::godot::Image> image =
        ::godot::Image::create_from_data(static_cast<int32_t>(cast.width), static_cast<int32_t>(cast.height), false,
                                         ::godot::Image::FORMAT_RGBA8, bytes);
    cast.texture = ::godot::ImageTexture::create_from_image(image);
}

}  // namespace ase::adp::godot
