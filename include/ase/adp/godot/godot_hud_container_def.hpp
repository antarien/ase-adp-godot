#pragma once

/**
 * ASE GODOT ADAPTER - HUD CONTAINER DEF
 *
 * @file        godot_hud_container_def.hpp
 * @brief       One container of the care view: its drawing side from SHA, its physical side from the inventory port
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, unit ContainerDef (the mockup's CAPS entry).
 *              BagWindow holds one for the leather pack, the pouch window one for the linen pouch, and FitCheck
 *              asks it what a thing must not exceed.
 *
 *              TWO SIDES, TWO SOURCES.
 *                drawing   the area a thing may lie in (hud_<family>_content_w/h), the units per centimetre a
 *                          thing is drawn at (hud_<family>_per_cm) and the smallest drawn size
 *                          (hud_<family>_item_min), tokens of design_tokens.json; <family> is the token family
 *                          of the window that draws the container (bag, pouch)
 *                physical  the opening a thing must pass (its longest piece in cm), the room in litres, the
 *                          weight it carries in kg and the asset of the container item: state of ase-inventory,
 *                          delivered on flora.carry.v1
 *
 *              THE RECORD COMES FROM THE BRIDGE. flora.carry.v1 does not exist yet (register N0451 under carrier
 *              N0448). Its bridge VivariumCarryBridge (godot_vivarium_hud_carry_frame.cpp) writes the mockup's
 *              CAPS from SHA (hud_bridge_container_<id>_*) into every frame and marks the gap in its own source;
 *              the window draws from physical() and never asks whether the port delivered. A frame that arrives
 *              without a record is a fault of the bridge: carry() logs it once on that edge and keeps the
 *              last record, so the picture stays whole. backend_open() and gap() are read by the bridge alone
 *              (VivariumView::draw_hud names the id with the record's fields through open_gap), never by a window.
 *
 *              The drawn size of a thing (the mockup's ik): its true length at per_cm, never below item_min,
 *              as the box UiItemSprite fits it into.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <string>

namespace ase::adp::godot {

/** The physical side of one container as flora.carry.v1 delivers it. */
struct ContainerDefCarry {
    double opening_cm = 0.0;  // the longest piece that passes the opening
    double room_l = 0.0;      // the room inside
    double carry_kg = 0.0;    // the weight it carries
    std::string asset;        // the asset of the container item, shown at perception 3 (texts.hud.mouth {asset})
};

/** One container of the care view. */
class ContainerDef {
public:
    /** Binds the drawing side of the token family `family` (bag, pouch); false with the missing token in `failed`. */
    bool bind(const UiTheme& theme, const std::string& family, std::string& failed);

    /**
     * Takes this frame's record of the container from the carry bridge; nullptr when the bridge handed none, and
     * then the last record stands. Entering that state logs gap() once.
     */
    void carry(const ContainerDefCarry* record);

    /** No record arrived this frame: the bridge handed none. */
    bool backend_open() const { return !measured_; }

    /** The register entry of the missing port. */
    const char* gap() const;

    /** The physical side: the record of this frame, or the last one while the bridge hands none. */
    const ContainerDefCarry& physical() const { return physical_; }

    double content_width() const { return content_w_; }
    double content_height() const { return content_h_; }

    /** The side of the box a thing of `length_cm` is drawn in (the mockup's ik times the drawing's box). */
    double drawn_box(double length_cm) const;

private:
    std::string family_;
    double content_w_ = 0.0;   // hud_<family>_content_w
    double content_h_ = 0.0;   // hud_<family>_content_h
    double per_cm_ = 0.0;      // hud_<family>_per_cm
    double item_min_ = 0.0;    // hud_<family>_item_min
    ContainerDefCarry physical_;
    bool measured_ = false;    // a record arrived this frame
    bool logged_ = false;      // the gap was logged since the last record
};

}  // namespace ase::adp::godot
