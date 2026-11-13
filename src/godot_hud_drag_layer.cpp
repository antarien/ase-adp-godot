/**
 * ASE GODOT ADAPTER - HUD DRAG LAYER
 *
 * @file        godot_hud_drag_layer.cpp
 * @brief       A carried thing under the finger: the ghost, the place it stands over, the verdict and the release
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_drag_layer.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0452 (flora.stow.v1), N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.01.00003
 */

#include <ase/adp/godot/godot_hud_drag_layer.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>
#include <ase/math/random.hpp>

#include <cstring>

namespace ase::adp::godot {

namespace {

/** The same place: both name one, and it is the same one. */
bool same_place(const char* left, const std::string& right) {
    return left != nullptr && right == left;
}

}  // namespace

bool DragLayer::bind(const UiTheme& theme, std::string& failed) {
    double shadow_alpha = 0.0;
    const char* const measures[] = {"hud_gesture_drag_slop", "hud_ghost_alpha",     "hud_ghost_max_box",
                                    "hud_ghost_shadow_dy",   "hud_ghost_shadow_blur", "hud_ghost_shadow_alpha",
                                    "hud_hand_x",            "hud_hand_y",          "hud_drop_hand_radius",
                                    "hud_drop_spot_inset",   "hud_drop_spot_step"};
    double* const slots[] = {&slop_,      &ghost_alpha_, &max_box_, &shadow_dy_,   &shadow_blur_, &shadow_alpha,
                             &hand_x_,    &hand_y_,      &hand_radius_, &spot_inset_, &spot_step_};
    for (size_t i = 0; i < 11; ++i) {
        if (!theme.measure(measures[i], *slots[i])) {
            failed = measures[i];
            return false;
        }
    }
    if (!theme.color("OVERLAY_SHADOW_LIFTED", shade_)) {
        failed = "OVERLAY_SHADOW_LIFTED";
        return false;
    }
    shade_.a = static_cast<float>(shadow_alpha);
    if (!fit_.bind(theme, failed)) {
        return false;
    }
    theme_ = &theme;
    sprites_.clear();
    holding_ = false;
    moved_ = false;
    return true;
}

const UiItemSprite* DragLayer::sprite(const std::string& art, std::string& failed) {
    for (const std::pair<std::string, UiItemSprite>& entry : sprites_) {
        if (entry.first == art) {
            return &entry.second;
        }
    }
    UiItemSprite drawing;
    if (!drawing.bind(*theme_, failed) || !drawing.set(art, failed)) {
        return nullptr;
    }
    sprites_.emplace_back(art, drawing);
    return &sprites_.back().second;
}

double DragLayer::scale_in(const ContainerDef& container, const UiItemSprite& drawing, double length_cm) {
    return container.drawn_box(length_cm) / ase::math::max(drawing.view_width(), drawing.view_height());
}

const ContainerDef* DragLayer::container_of(const BagWindow& window, const char* place) {
    if (place == nullptr) {
        return nullptr;
    }
    if (std::strcmp(place, GODOT_HUD_CONTAINER_PACK) == 0) {
        return &window.pack_def();
    }
    if (std::strcmp(place, GODOT_HUD_CONTAINER_POUCH) == 0) {
        return &window.pouch_def();
    }
    return nullptr;
}

void DragLayer::take(const DragThing& thing, const ::godot::Vector2& point, const ::godot::Rect2& box, double k) {
    thing_ = thing;
    start_ = point;
    finger_ = point;
    grip_ = point - box.position;
    taken_k_ = k;
    ghost_k_ = k;
    target_ = DragTarget();
    verdict_ = FitCheckFrame();
    holding_ = true;
    moved_ = false;
}

bool DragLayer::move(const ::godot::Vector2& point) {
    if (!holding_) {
        return false;
    }
    finger_ = point;
    if (!moved_ && (point - start_).length() > slop_) {
        moved_ = true;
        return true;
    }
    return false;
}

bool DragLayer::aim(const DragLayerFrame& now, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.drag_layer.unbound";
        return false;
    }
    if (now.window == nullptr || now.belt == nullptr || now.bag_icon == nullptr || now.rack == nullptr ||
        now.world == nullptr) {
        failed = "hud.drag_layer.frame";
        return false;
    }
    if (!moved()) {
        return true;
    }
    const UiItemSprite* drawing = sprite(thing_.art, failed);
    if (drawing == nullptr) {
        return false;
    }

    // dropTarget: the windows, the hand, the belt, the bag icon, then the world above the ground line.
    target_ = DragTarget();
    // The hand, the belt and the bag icon stand on the bottom band, hand_band below the frame the finger is read
    // in; the windows and the world stand on the middle band.
    const ::godot::Vector2 at_hand = finger_ - ::godot::Vector2(0.0f, static_cast<float>(now.hand_band));
    const BagWindowDrop drop = now.window->over(finger_);
    if (drop.container != nullptr && drop.def != nullptr) {
        const double k = scale_in(*drop.def, *drawing, thing_.length_cm);
        target_.place = drop.container;
        target_.where = std::strcmp(drop.container, GODOT_HUD_CONTAINER_POUCH) == 0 ? GODOT_HUD_WHERE_POUCH
                                                                                    : GODOT_HUD_WHERE_PACK;
        target_.spot = finger_ - grip_ * static_cast<float>(k / taken_k_) - drop.corner;
    } else if ((at_hand - ::godot::Vector2(static_cast<float>(hand_x_), static_cast<float>(hand_y_))).length() <
               hand_radius_) {
        target_.place = GODOT_HUD_PLACE_HAND;
        target_.where = GODOT_HUD_WHERE_HAND;
    } else if (const char* holster = now.belt->over(at_hand)) {
        target_.place = holster;
        target_.where = GODOT_HUD_WHERE_BELT;
    } else if (const char* icon = now.bag_icon->over(at_hand)) {
        target_.place = icon;
        target_.where = GODOT_HUD_WHERE_PACK;
        target_.automatic = true;
    } else if (finger_.y < now.world->ground_line() && now.ground_hit) {
        target_.place = GODOT_HUD_PLACE_GROUND;
        if (now.rack->over(now.rack_world, finger_)) {
            target_.where = GODOT_HUD_WHERE_RACK;
            target_.rack = true;
        } else {
            target_.where = GODOT_HUD_WHERE_GROUND;
            target_.ground = now.ground;
        }
    }

    // ghost: the scale of the container under the finger, else of the one it came from, else of the pack.
    const ContainerDef* scale_from = container_of(*now.window, target_.place);
    if (scale_from == nullptr) {
        scale_from = container_of(*now.window, thing_.from);
    }
    if (scale_from == nullptr) {
        scale_from = &now.window->pack_def();
    }
    ghost_k_ = ase::math::min(scale_from->drawn_box(thing_.length_cm), max_box_) /
               ase::math::max(drawing->view_width(), drawing->view_height());

    judge(now);
    return true;
}

void DragLayer::judge(const DragLayerFrame& now) {
    verdict_ = FitCheckFrame();
    if (!moved()) {
        return;
    }
    verdict_.finger = finger_;
    verdict_.ghost_top = (finger_ - grip_ * static_cast<float>(ghost_k_ / taken_k_)).y;
    if (target_.place == nullptr) {
        return;
    }
    // The stow bridge judges the place the finger stood over when it was asked; a place entered this frame
    // gets its verdict on the next one, and until then the frame carries none (where stays empty).
    const StowAnswer* answer = now.answer;
    if (answer != nullptr && answer->key == thing_.key && same_place(target_.place, answer->place) &&
        answer->rack == target_.rack) {
        verdict_.where = target_.where;
        verdict_.code = answer->code;
        verdict_.values = answer->values;
    }
}

bool DragLayer::draw(UiPen& canvas, const ::godot::Transform2D& frame, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.drag_layer.unbound";
        return false;
    }
    if (!moved()) {
        return true;
    }
    const UiItemSprite* drawing = sprite(thing_.art, failed);
    if (drawing == nullptr) {
        return false;
    }
    const ::godot::Vector2 corner = finger_ - grip_ * static_cast<float>(ghost_k_ / taken_k_);
    // artSvg(D,o,...): the ghost is the thing itself, the can with its water and the trough with its sap.
    UiItemSpritePlace place;
    place.level = thing_.level;
    place.opacity = ghost_alpha_;
    place.silhouette = true;
    place.shade = shade_;
    place.spread = shadow_blur_;
    drawing->draw_scaled(canvas, frame, corner + ::godot::Vector2(0.0f, static_cast<float>(shadow_dy_)), ghost_k_,
                         place);
    place.silhouette = false;
    drawing->draw_scaled(canvas, frame, corner, ghost_k_, place);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    if (!verdict_.where.empty() && !fit_.draw(canvas, frame, verdict_, failed)) {
        return false;
    }
    return true;
}

bool DragLayer::free_spot(const DragLayerFrame& now, ::godot::Vector2& spot, std::string& failed) {
    const ContainerDef& pack = now.window->pack_def();
    const UiItemSprite* drawing = sprite(thing_.art, failed);
    if (drawing == nullptr) {
        return false;
    }
    const double k = scale_in(pack, *drawing, thing_.length_cm);
    const double w = drawing->view_width() * k;
    const double h = drawing->view_height() * k;
    for (double y = spot_inset_; y < pack.content_height() - h; y += spot_step_) {
        for (double x = spot_inset_; x < pack.content_width() - w; x += spot_step_) {
            bool taken = false;
            if (now.pack_things != nullptr) {
                for (const BagThing& other : *now.pack_things) {
                    if (other.key == thing_.key) {
                        continue;
                    }
                    const UiItemSprite* other_drawing = sprite(other.art, failed);
                    if (other_drawing == nullptr) {
                        return false;
                    }
                    const double other_k = scale_in(pack, *other_drawing, other.length_cm);
                    const double other_w = other_drawing->view_width() * other_k;
                    const double other_h = other_drawing->view_height() * other_k;
                    if (x < other.at.x + other_w && x + w > other.at.x && y < other.at.y + other_h &&
                        y + h > other.at.y) {
                        taken = true;
                        break;
                    }
                }
            }
            if (!taken) {
                spot = ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
                return true;
            }
        }
    }
    // No free cell: the mockup lays the thing at a random spot inside the inset.
    const double reach_x = pack.content_width() - w - 2.0 * spot_inset_;
    const double reach_y = pack.content_height() - h - 2.0 * spot_inset_;
    spot = ::godot::Vector2(static_cast<float>(spot_inset_ + ase::math::random_float() * reach_x),
                            static_cast<float>(spot_inset_ + ase::math::random_float() * reach_y));
    return true;
}

bool DragLayer::release(const DragLayerFrame& now, Toast& toast, double clock, DragRelease& out,
                        std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.drag_layer.unbound";
        return false;
    }
    if (!holding_) {
        failed = "hud.drag_layer.empty";
        return false;
    }
    out = DragRelease();
    out.key = thing_.key;
    if (!moved_) {
        out.kind = GODOT_HUD_DRAG_TAP;
        holding_ = false;
        return true;
    }
    if (!aim(now, failed)) {
        return false;
    }
    // The drop takes the verdict on the place it aims at now (m.html 821, fits at the release point): the stow
    // bridge answered that place before the release, a port's answer for another place leaves it empty.
    judge(now);
    if (target_.place != nullptr && FitCheck::allowed(verdict_)) {
        out.target = target_;
        if (target_.automatic && !free_spot(now, out.target.spot, failed)) {
            return false;
        }
        // dropItem: the hand returns through equip() before the toast, every other place toasts where it went.
        if (std::string(target_.place) != GODOT_HUD_PLACE_HAND) {
            std::string item;
            std::string where;
            if (!word_line(*theme_, std::string(GODOT_HUD_WORD_ITEM_HEAD) + thing_.id + GODOT_HUD_WORD_ITEM_TAIL,
                           item, failed) ||
                !word_line(*theme_, target_.where, where, failed) ||
                !toast.post(GODOT_HUD_WORD_DROP_AT, {HudPortValue{"item", item}, HudPortValue{"where", where}},
                            GODOT_HUD_COLOUR_DROP, clock, failed)) {
                return false;
            }
        }
        out.kind = GODOT_HUD_DRAG_SEND;
    } else if (target_.place != nullptr && FitCheck::refused(verdict_)) {
        if (!toast.post(std::string(GODOT_HUD_WORD_FIT) + verdict_.code, verdict_.values, GODOT_HUD_COLOUR_REFUSED,
                        clock, failed)) {
            return false;
        }
        out.kind = GODOT_HUD_DRAG_REFUSED;
    } else {
        out.kind = GODOT_HUD_DRAG_HELD;
    }
    holding_ = false;
    moved_ = false;
    target_ = DragTarget();
    verdict_ = FitCheckFrame();
    return true;
}

}  // namespace ase::adp::godot
