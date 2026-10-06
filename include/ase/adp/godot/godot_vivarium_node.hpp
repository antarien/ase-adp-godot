#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW
 *
 * @file        godot_vivarium_node.hpp
 * @brief       VivariumView - instance data and behaviour of the Godot class AseVivariumView
 * @description The demo frontend of PLAN_ASE_VIVARIUM_PHASE_01_INTEG: four patches seen from
 *              above, the actions Regar, Pausar/Continuar and Reiniciar, a header with the build
 *              identity. One view owns exactly ONE GodotHostResourceManager and through it one
 *              KernelEmbeddedHost - no global App, no singleton. Godot draws and takes input; the
 *              simulation stays in the ASE registry of that host.
 *
 *              COMPOSED, NOT DERIVED: an object of the Godot class AseVivariumView is an engine
 *              Node2D. godot_registration.cpp registers that class through the GDExtension class
 *              interface and gives every such object one VivariumView as its instance data. The
 *              view draws and reads input through node_, the godot-cpp handle of exactly that
 *              object - no C++ type of this adapter derives from a Godot type.
 *
 *              WHAT THE VIEW NEVER DOES: it computes no growth, writes no simulation value back
 *              and shows no optimistic number. Every drawn value is the copy the port reported
 *              after the last tick; the stage colour is the projection of the port's own flags.
 *
 *              THE EDITOR STAYS SIMULATION-FREE (V0): ready() returns when Engine::is_editor_hint,
 *              so importing or editing the scene starts no host and loads no plugin.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-05
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/**
 * Where things stand on the logical 720 x 1280 surface and how values are scaled for drawing -
 * read from res://config/vivarium_start.json, never written into the code.
 */
struct VivariumLayout {
    ::godot::Vector2 size;
    double           biomass_full = 0.0;   // cover drawn full at this biomass
    double           moisture_full = 0.0;  // moisture bar drawn full at this moisture
    float            header_height = 0.0f;
    ::godot::Vector2 title_at;
    ::godot::Vector2 build_at;
    ::godot::Vector2 info_at;
    float            info_line = 0.0f;
    ::godot::Vector2 note_at;
    std::vector<::godot::Rect2> patch_rects;  // same order as VivariumStartConfig::patches
    ::godot::Rect2   water_rect;
    ::godot::Rect2   pause_rect;
    ::godot::Rect2   reset_rect;
};

/** Every text the view draws, Brazilian Portuguese, from the same configuration. */
struct VivariumTexts {
    ::godot::String title;
    ::godot::String selection;
    ::godot::String selection_none;
    ::godot::String area;
    ::godot::String simulation;
    ::godot::String ticks;
    ::godot::String biomass;
    ::godot::String moisture;
    ::godot::String age;
    ::godot::String stage_seed;
    ::godot::String stage_sprout;
    ::godot::String stage_mature;
    ::godot::String stage_dead;
    ::godot::String paused;
    ::godot::String restart_note;
    ::godot::String error;
    ::godot::String water;
    ::godot::String pause;
    ::godot::String resume;
    ::godot::String reset;
    ::godot::String separator;   // between the parts of one line, e.g. "área 2 · Broto"
};

/**
 * @brief The Vivarium view: one Godot object, one embedded ASE host, four patches.
 *
 * Godot calls the six entry points below through the class binding in godot_registration.cpp:
 * ready (_ready), process (_process), draw (_draw), unhandled_input (_unhandled_input),
 * exit_tree (_exit_tree) and notification (every notification of the object).
 *
 * Lifecycle (PLAN 01.3): ready boots the host through the resource manager; process advances it
 * with dt capped and reads the snapshot; notification keeps the pause reasons apart (user,
 * application, focus) and drops the first delta after a resume; exit_tree and Reiniciar run the
 * same teardown. Input: one press per physical tap, ScreenTouch or the left mouse button, mapped
 * onto the logical surface through make_input_local.
 */
class VivariumView {
public:
    /** node: the godot-cpp handle of the engine object this view is the instance data of. */
    explicit VivariumView(::godot::Node2D* node);
    ~VivariumView();

    VivariumView(const VivariumView&) = delete;
    VivariumView& operator=(const VivariumView&) = delete;

    void ready();
    void process(double delta);
    void draw();
    void unhandled_input(const ::godot::Ref<::godot::InputEvent>& event);
    void exit_tree();
    void notification(int32_t what);

private:
    bool load_config();
    bool load_host_config(const ::godot::Dictionary& root);
    bool load_layout(const ::godot::Dictionary& root);
    bool load_texts(const ::godot::Dictionary& root);
    bool fail_config(const char* key);

    void boot_host();
    void restart();
    void tap(const ::godot::Vector2& at);
    void select(uint32_t object_id);
    void water();
    void toggle_pause();
    void follow_stages();
    void follow_water();
    void drain_log();
    void note(const ::godot::String& line);

    [[nodiscard]] bool can_water() const;
    [[nodiscard]] bool ticking() const;
    [[nodiscard]] uint8_t stage_of(uint32_t object_id) const;
    [[nodiscard]] ::godot::String stage_text(uint8_t stage) const;
    [[nodiscard]] ::godot::String boot_line() const;
    [[nodiscard]] ::godot::String measure(uint32_t object_id) const;

    void draw_patch(const ::godot::Rect2& rect, uint32_t object_id);
    void draw_button(const ::godot::Rect2& rect, const ::godot::String& label, bool enabled);
    void draw_info();
    void draw_failure();

    ::godot::Node2D*         node_ = nullptr;      // the engine object; Godot owns it, it owns this view
    GodotHostResourceManager resources_;
    VivariumStartConfig      start_;
    VivariumLayout           layout_;
    VivariumTexts            texts_;
    ::godot::Dictionary      identity_;        // build.identity of the staged bundle
    ::godot::String          build_line_;      // drawn under the title
    int32_t                  build_font_size_ = GODOT_FONT_SMALL;  // fitted to the width at boot
    ::godot::String          failure_text_;    // not empty: the error panel stands instead of the patches
    std::vector<std::string> log_lines_;       // drained once per frame and printed
    std::vector<uint8_t>     stages_;          // last drawn stage per configured patch
    uint32_t                 selected_id_ = GODOT_NO_OBJECT;
    uint32_t                 water_pending_id_ = GODOT_NO_OBJECT;  // watered, its step not yet seen
    double                   water_pending_from_ = 0.0;            // its moisture when watered
    double                   water_pending_age_ = 0.0;             // its age when watered
    bool                     configured_ = false;
    bool                     user_paused_ = false;
    bool                     app_paused_ = false;
    bool                     focus_lost_ = false;
    bool                     drop_next_delta_ = false;
};

}  // namespace ase::adp::godot
