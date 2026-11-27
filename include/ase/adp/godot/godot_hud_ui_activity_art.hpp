#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI ACTIVITY ART
 *
 * @file        godot_hud_ui_activity_art.hpp
 * @brief       Reads every activity symbol of SHA's activity_art.json once and answers lookups by name
 * @description MIG_ASE_SKILL_RSM B7, PLAN_ASE_VIVARIUM_PHASE_06_HUD sections "Item-Art: die Zeichnung jedes
 *              Dings" and "Herkunft: SHA ist die Quelle". The symbols (picareta, vagoneta, pa_monte, guincho,
 *              veio, monte, toras, tabuas, cavalete, mata, arvore_seca, machado_toco, serra_tracador) stand in
 *              sha-web-styles/src/activity-art.json in the format of item-art.json and travel the way it does:
 *              generate-item-art.cjs activity writes console/generated/activity_art.json, the SHA cascade
 *              carries it, avt-stage puts it into res://config/, and hud.distribution.activities of the start
 *              configuration names it.
 *
 *              READ LIKE THE ITEMS, NO FALLBACK. load reads the file the start configuration names and refuses
 *              the configuration where the name, the file or its section activities or places is missing or
 *              unreadable; a symbol that stands in no file is never drawn from a stand-in. art answers the entry
 *              of a symbol (vw, vh, art) in the format UiItemSprite draws, and false for a name the file does not
 *              carry. place answers the symbol a tool of hud.tools carries (section places, tool.<name> → symbol,
 *              written by generate-item-art.cjs from the field place of activity-art.json).
 *
 *              A SEPARATE UNIT, NOT A LINE OF UiTheme: godot_hud_ui_theme.hpp is included by more than 100 own
 *              units of the build and takes no new line (header ratchet, operator decision 2026-11-21).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/variant/dictionary.hpp>

#include <string>

namespace ase::adp::godot {

/** The activity symbols of activity_art.json, read once from the file hud.distribution.activities names. */
class UiActivityArt {
public:
    /** Reads the file named by hud.distribution.activities of `root`; false with the failing key in `failed`. */
    bool load(const ::godot::Dictionary& root, std::string& failed);

    /** The drawing of the symbol `name` (vw, vh, art) in the format of item_art.json; false for an unknown name. */
    bool art(const std::string& name, ::godot::Dictionary& out) const;

    /** The drawing of the symbol whose place is `place` (tool.<name> of hud.tools); false where no symbol stands on it. */
    bool place(const std::string& place, ::godot::Dictionary& out) const;

private:
    ::godot::Dictionary activities_;  // activity_art.json: activities
    ::godot::Dictionary places_;      // activity_art.json: places, place → symbol
};

}  // namespace ase::adp::godot
