/**
 * ASE GODOT ADAPTER - HUD WORLD DROP
 *
 * @file        godot_hud_world_drop.cpp
 * @brief       The things lying on the ground: drawn at their world place, darkened with the hour, hit to pick up
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_world_drop.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_world_drop.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <initializer_list>

namespace ase::adp::godot {

bool WorldDrop::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    sprites_.clear();
    places_.clear();
    rack_drawn_ = false;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_world_metre_scale", &metre_scale_},
             {"hud_world_light_floor", &light_floor_},
             {"hud_world_light_span", &light_span_},
             {"hud_world_thing_shadow_dx", &shadow_dx_},
             {"hud_world_thing_shadow_dy", &shadow_dy_},
             {"hud_world_thing_shadow_share", &shadow_share_},
             {"hud_world_thing_shadow_pad", &shadow_pad_},
             {"hud_world_thing_shadow_ry", &shadow_ry_},
             {"hud_ground_item_min", &item_min_},
             {"hud_ground_item_depth", &item_depth_},
             {"hud_ground_item_pad", &item_pad_},
             {"hud_ground_item_pad_low", &item_pad_low_},
             {"hud_ground_dot_radius", &dot_radius_},
             {"hud_ground_dot_dy", &dot_dy_},
             {"hud_ground_dot_alpha", &dot_alpha_},
             {"hud_ground_rack_hit_half_w", &rack_half_w_},
             {"hud_ground_rack_hit_rise", &rack_rise_},
             {"hud_ground_rack_hit_w", &rack_w_},
             {"hud_ground_rack_hit_h", &rack_h_},
             {"hud_drop_ground_max_y", &ground_max_y_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!theme.color("WORLD_THING_SHADOW", shadow_)) {
        failed = "WORLD_THING_SHADOW";
        return false;
    }
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 2) {
        failed = "hud.perception.levels";
        return false;
    }
    level_dot_ = levels[1];
    return true;
}

const UiItemSprite* WorldDrop::sprite(const std::string& art, std::string& failed) {
    for (const std::pair<std::string, UiItemSprite>& entry : sprites_) {
        if (entry.first == art) {
            return &entry.second;
        }
    }
    UiItemSprite drawing;
    if (theme_ == nullptr || !drawing.bind(*theme_, failed) || !drawing.set(art, failed)) {
        return nullptr;
    }
    sprites_.emplace_back(art, drawing);
    return &sprites_.back().second;
}

bool WorldDrop::metre(const UiAnchor& anchor, const ::godot::Vector3& place, ::godot::Vector2& at,
                      double& length) const {
    ::godot::Vector2 across;
    if (!anchor.place(place, at) || !anchor.place(place + ::godot::Vector3(1.0f, 0.0f, 0.0f), across)) {
        return false;
    }
    // pxm(z): the mockup's metre is this share of the scene's at the same place.
    length = static_cast<double>(at.distance_to(across)) * metre_scale_;
    return true;
}

bool WorldDrop::draw(UiPen& canvas, const ::godot::Transform2D& frame, const WorldDropFrame& now,
                     std::string& failed) {
    places_.clear();
    rack_drawn_ = false;
    if (now.anchor == nullptr) {
        failed = "WorldDrop drawn without the projection of the frame";
        return false;
    }
    // drawWorld's filter: brightness(floor + span * L) over the world group only. It is the pen's tint, which
    // VivariumHud::draw sets back to white after the tap hole and the sow hole close the group.
    const float bright =
        static_cast<float>(light_floor_ + light_span_ * ::ase::math::clamp(now.light, 0.0, 1.0));
    canvas.set_tint(::godot::Color(bright, bright, bright, 1.0f));

    // groundHits: the tool place in metres at its depth.
    ::godot::Vector2 rack;
    double rack_metre = 0.0;
    if (metre(*now.anchor, now.rack, rack, rack_metre)) {
        rack_hit_ = ::godot::Rect2(static_cast<float>(rack.x - rack_metre * rack_half_w_),
                                   static_cast<float>(rack.y - rack_metre * rack_rise_),
                                   static_cast<float>(rack_metre * rack_w_), static_cast<float>(rack_metre * rack_h_));
        rack_drawn_ = true;
    }

    // The farthest first (the foot highest on the frame), among equals the last moved on top.
    struct Seen {
        const GroundThing* thing;
        ::godot::Vector2 foot;
        double metre;
    };
    std::vector<Seen> seen;
    if (now.things != nullptr) {
        for (const GroundThing& thing : *now.things) {
            Seen entry{&thing, ::godot::Vector2(), 0.0};
            if (!metre(*now.anchor, thing.place, entry.foot, entry.metre)) {
                continue;  // behind the camera
            }
            std::size_t at = seen.size();
            seen.push_back(entry);
            while (at > 0 && (seen[at - 1].foot.y > entry.foot.y ||
                              (seen[at - 1].foot.y == entry.foot.y && seen[at - 1].thing->order > thing.order))) {
                seen[at] = seen[at - 1];
                --at;
            }
            seen[at] = entry;
        }
    }

    canvas.draw_set_transform_matrix(frame);
    for (const Seen& entry : seen) {
        const GroundThing& thing = *entry.thing;
        const UiItemSprite* drawing = sprite(thing.art, failed);
        if (drawing == nullptr) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
        const double length = thing.length_cm / 100.0 * entry.metre;
        const double box = ::ase::math::max(drawing->view_width(), drawing->view_height());
        const double k = box > 0.0 ? length / box : 0.0;

        if (!thing.at_rack) {
            // The flat shadow: an ellipse half the drawn length plus the pad wide.
            const ::godot::Vector2 centre = entry.foot + ::godot::Vector2(static_cast<float>(shadow_dx_),
                                                                          static_cast<float>(shadow_dy_));
            const ::godot::Transform2D oval(0.0f,
                                            ::godot::Size2(static_cast<float>(length * shadow_share_ + shadow_pad_),
                                                           static_cast<float>(shadow_ry_)),
                                            0.0f, centre);
            canvas.draw_set_transform_matrix(frame * oval);
            canvas.draw_circle(::godot::Vector2(), 1.0f, shadow_);
            canvas.draw_set_transform_matrix(frame);
        }

        // drawItemAt: the foot at the place, turned, the drawing's bottom middle on the foot. A thing whose file
        // stands as a body in the 3D scene (lay_tools) is that body; its box and hit box still come from here.
        const ::godot::Vector2 corner(static_cast<float>(-drawing->view_width() * k / 2.0),
                                      static_cast<float>(-drawing->view_height() * k));
        if (!thing.bodied) {
            UiItemSpritePlace place;
            const ::godot::Transform2D turned(static_cast<float>(thing.angle), entry.foot);
            drawing->draw_scaled(canvas, frame * turned, corner, k, place);
            canvas.draw_set_transform_matrix(frame);
        }

        if (thing.at_rack) {
            continue;  // a thing at the tool place is picked from the rack list, not from the ground
        }
        const double reach = ::ase::math::max(item_min_, length);
        GroundThingPlace drawn;
        drawn.key = thing.key;
        drawn.k = k;
        drawn.box = ::godot::Rect2(entry.foot.x + corner.x, entry.foot.y + corner.y,
                                   static_cast<float>(drawing->view_width() * k),
                                   static_cast<float>(drawing->view_height() * k));
        drawn.hit = ::godot::Rect2(static_cast<float>(entry.foot.x - reach / 2.0 - item_pad_),
                                   static_cast<float>(entry.foot.y - reach * item_depth_ - item_pad_),
                                   static_cast<float>(reach + 2.0 * item_pad_),
                                   static_cast<float>(reach * item_depth_ + item_pad_low_));
        places_.push_back(drawn);

        if (now.level >= level_dot_ && now.selected != thing.key) {
            ::godot::Color dot;
            if (!theme_->tone(thing.tone, dot)) {
                failed = "DOMAIN_" + thing.tone;
                canvas.draw_set_transform_matrix(::godot::Transform2D());
                return false;
            }
            dot.a *= static_cast<float>(dot_alpha_);
            canvas.draw_circle(entry.foot + ::godot::Vector2(0.0f, static_cast<float>(dot_dy_)),
                               static_cast<float>(dot_radius_), dot);
        }
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

std::string WorldDrop::hit(const ::godot::Vector2& point) const {
    for (std::size_t index = places_.size(); index > 0; --index) {
        if (places_[index - 1].hit.has_point(point)) {
            return places_[index - 1].key;
        }
    }
    return std::string();
}

bool WorldDrop::hit_rack(const ::godot::Vector2& point) const {
    return rack_drawn_ && rack_hit_.has_point(point);
}

}  // namespace ase::adp::godot
