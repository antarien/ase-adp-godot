/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD PLACES
 *
 * @file        godot_vivarium_hud_place.cpp
 * @brief       VivariumView - the places a tap reaches the HUD through, logged as lines whenever they change
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5 and 06.11: after each drawn frame (godot_vivarium_hud.cpp:
 *              VivariumView::draw_hud) the view names, in logical pixels, every place the window acceptance taps
 *              or holds: the debug switch on the sky band, the handle or the fold and every chip of the open
 *              debug drawer, the system glyph at the foot of the rail and the ring of the mode sys (Reiniciar),
 *              and the stand of the birch beside its trunk, which a tap selects the tree through (ACT_TREE opens
 *              the mode focus, whose edge the view logs as "hud mode=focus place=N"), and the play chip of the
 *              trail, which a tap leaves the mode arrive through (the edge "hud mode=rest"), the hand's verb chips, the cut
 *              marks of the mode prune, the rows of the mode rack, the markers of the signs, the signs on the rail,
 *              the event rows of the trail and the things on the ground. The lines are written only
 *              when the set changes and close with "debug place end count=N". The HUD is the one source of
 *              these places: each comes from the same rect its hit() asks (godot_hud_compose_place.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.01.00013
 */

#include <ase/adp/godot/godot_hud_compose_place.hpp>
#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The part a chip answers as: the word of its act and its index, UINT32_MAX where the act carries none. */
const char* place_part(const DebugLayerHit& act, uint32_t& index) {
    index = UINT32_MAX;
    if (act.handle) {
        return "handle";
    }
    if (act.fold) {
        return "fold";
    }
    if (act.tab != GODOT_HUD_DEBUG_NONE) {
        index = act.tab;
        return "tab";
    }
    if (act.button != GODOT_HUD_DEBUG_NONE) {
        index = act.button;
        return "button";
    }
    if (act.jump != GODOT_HUD_DEBUG_NONE) {
        index = act.jump;
        return "jump";
    }
    if (act.step != GODOT_HUD_DEBUG_NONE) {
        index = act.step;
        return "step";
    }
    if (act.go) {
        return "go";
    }
    if (act.now) {
        return "now";
    }
    if (act.field != GODOT_DATE_PART_NONE) {
        index = act.field;  // the GODOT_DATE_PART_* the cell picks
        return "field";
    }
    if (act.key != GODOT_HUD_DEBUG_NONE) {
        index = act.key;
        return "key";
    }
    if (act.clear) {
        return "clear";
    }
    if (act.relief != GODOT_HUD_DEBUG_NONE) {
        index = act.relief;
        return "relief";
    }
    return "none";
}

/** One place line: the part, its index where it has one, the key of a verb or a thing where it has one, the rect
 *  in logical pixels. */
std::string place_line(const char* part, uint32_t index, const std::string& key, const ::godot::Transform2D& frame,
                       const ::godot::Rect2& rect) {
    const ::godot::Vector2 from = frame.xform(rect.position);
    const ::godot::Vector2 to = frame.xform(rect.position + rect.size);
    std::string line = std::string("debug place part=") + part;
    if (index != UINT32_MAX) {
        line += " index=" + std::to_string(index);
    }
    if (!key.empty()) {
        line += " key=" + key;
    }
    line += " x=" + std::to_string(static_cast<int64_t>(from.x)) + " y=" + std::to_string(static_cast<int64_t>(from.y)) +
            " w=" + std::to_string(static_cast<int64_t>(to.x - from.x)) +
            " h=" + std::to_string(static_cast<int64_t>(to.y - from.y));
    return line;
}

}  // namespace

void VivariumView::note_hud_places() {
    // The places a tap reaches the debug layer, Reiniciar and the tree through, in logical pixels: the switch on
    // the band, the handle or the fold and every chip of the open drawer; the system glyph at the foot of the
    // rail, which a long press opens the mode sys with, and in that mode the ring held for Reiniciar; the stand
    // of the birch beside its trunk, which a tap selects the tree through while the mode lays the tree, and the
    // trunk itself while the mode lays it (sap and plug, `trunk`), which a tap bores or pegs; the play
    // chip of the trail, which a tap leaves the mode arrive through while the trail stands; the sky band's rect,
    // which a tap opens and closes the sky sheet through, and while the sheet stands every hourglass chip (`glass
    // key=<factor>`), which a tap sets the pace through; the bag icon's disc, which a tap opens the pack
    // through; the box round the thing in hand, which a hold pours a can through, its stow button and its verb chips (`verb
    // key=<verb>`); every chip of the fan in focus, `rack` where the tool has no item and `take` where it has one;
    // in the mode prune every cut mark (`cut index=<internode>`), in the mode rack every row of the tool place
    // (`row key=<item>`, the item id, since the thing keys k<N> name no tool); while the markers show, every
    // marker's tap target (`sign index=<sign>`), which hit() asks before the tree, so the tree's stand ends where
    // one begins; in the mode focus the square round the ring (`focus`), the one way back to rest, since a tap on
    // the birch there selects it again and opens no edge; while the rail shows, the target of each of its signs
    // (`rail index=<sign>`), which opens that sign; in the mode arrive each event row of the trail (`event
    // index=<sign>`, no index where the row rests the view); and while the mode lays the world's hits, the padded
    // box of each thing on the ground (`ground key=<item>`), which a tap picks up; in the mode water the middle of
    // the brook's line (`brook`), which a tap fills the can of the hand at. The view is the one source of
    // these places; the window acceptance taps and holds where these lines name them.
    ::godot::Rect2 debug_switch;
    std::vector<DebugLayerChip> chips;
    std::string failed;
    if (!VivariumHudPlaces::debug_places(hud_, hud_now_, debug_switch, chips, failed)) {
        fail_hud("debug places " + failed);
        return;
    }
    ::godot::Rect2 foot;
    ::godot::Rect2 ring;
    ::godot::Rect2 close;
    bool foot_hidden = true;
    bool ring_hidden = true;
    VivariumHudPlaces::system_places(hud_, foot, foot_hidden, ring, ring_hidden, close);
    ::godot::Rect2 tree;
    bool tree_hidden = true;
    VivariumHudPlaces::tree_place(hud_, hud_now_, tree, tree_hidden);
    ::godot::Rect2 trunk;
    bool trunk_hidden = true;
    VivariumHudPlaces::trunk_place(hud_, hud_now_, trunk, trunk_hidden);
    ::godot::Rect2 play;
    bool play_hidden = true;
    VivariumHudPlaces::arrive_place(hud_, play, play_hidden);
    ::godot::Rect2 sky;
    bool sky_hidden = true;
    VivariumHudPlaces::band_place(hud_, sky, sky_hidden);
    ::godot::Rect2 bag;
    bool bag_hidden = true;
    HandThingPlace hand;
    bool hand_hidden = true;
    VivariumHudPlaces::bottom_places(hud_, bag, bag_hidden, hand, hand_hidden);
    // The places a drag is carried to on the bottom band, while it shows: the hand's drop square and both holsters.
    // DragLayer::aim asks them at the finger less cover.hand, so they stand that far down in the middle band's frame.
    std::vector<std::pair<std::string, ::godot::Rect2>> drops;
    if (!bag_hidden) {
        drops.emplace_back(GODOT_HUD_PLACE_HAND, hud_.drag().hand_drop());
        hud_.belt().holster_places(drops);
        for (std::pair<std::string, ::godot::Rect2>& drop : drops) {
            drop.second.position.y += static_cast<float>(hud_now_.cover.hand);
        }
    }
    std::vector<ActionFanPlace> fan;
    VivariumHudPlaces::fan_places(hud_, fan);
    ::godot::Rect2 ring_focus;
    bool focus_hidden = true;
    VivariumHudPlaces::focus_place(hud_, ring_focus, focus_hidden);
    std::vector<PruneMarkPlace> cuts;
    std::vector<RackToolPlace> rows;
    VivariumHudPlaces::mode_places(hud_, cuts, rows);
    std::vector<SignMarkerPlace> signs;
    VivariumHudPlaces::sign_places(hud_, signs);
    std::vector<SignMarkerPlace> rail;
    VivariumHudPlaces::rail_places(hud_, rail);
    std::vector<SignMarkerPlace> events;
    VivariumHudPlaces::event_places(hud_, events);
    std::vector<GroundThingPlace> ground;
    VivariumHudPlaces::ground_places(hud_, ground);
    std::vector<BagThingPlace> pack;
    std::vector<BagThingPlace> pouch;
    ::godot::Rect2 pack_close;
    ::godot::Rect2 pouch_close;
    bool pack_hidden = true;
    bool pouch_hidden = true;
    VivariumHudPlaces::window_places(hud_, pack, pouch, pack_close, pack_hidden, pouch_close, pouch_hidden);
    ::godot::Rect2 pack_grip;
    ::godot::Rect2 pouch_grip;
    bool pack_grip_hidden = true;
    bool pouch_grip_hidden = true;
    VivariumHudPlaces::grip_places(hud_, pack_grip, pack_grip_hidden, pouch_grip, pouch_grip_hidden);
    ::godot::Rect2 tool_rack;
    const bool rack_shown = VivariumHudPlaces::rack_place(hud_, tool_rack);
    const std::string none;
    std::vector<std::string> lines;
    lines.push_back(place_line("switch", UINT32_MAX, none, hud_frame_, debug_switch));
    if (!foot_hidden) {
        lines.push_back(place_line("system", UINT32_MAX, none, hud_frame_, foot));
    }
    if (!ring_hidden) {
        lines.push_back(place_line("ring", UINT32_MAX, none, hud_frame_, ring));
        lines.push_back(place_line("close", UINT32_MAX, none, hud_frame_, close));  // the one way out of the mode sys
    }
    if (!tree_hidden) {
        lines.push_back(place_line("tree", UINT32_MAX, none, hud_frame_, tree));
    }
    if (!trunk_hidden) {
        lines.push_back(place_line("trunk", UINT32_MAX, none, hud_frame_, trunk));  // a tap here bores or pegs it
    }
    if (!play_hidden) {
        lines.push_back(place_line("play", UINT32_MAX, none, hud_frame_, play));
    }
    if (!sky_hidden) {
        lines.push_back(place_line("sky", UINT32_MAX, none, hud_frame_, sky));
    }
    if (!bag_hidden) {
        lines.push_back(place_line("bag", UINT32_MAX, none, hud_frame_, bag));
    }
    if (!hand_hidden) {
        lines.push_back(place_line("hand", UINT32_MAX, none, hud_frame_, hand.box));  // a hold here pours a can in hand
        lines.push_back(place_line("stow", UINT32_MAX, none, hud_frame_, hand.stow));  // a tap here puts it away
        for (const std::pair<std::string, ::godot::Rect2>& verb : hand.verbs) {
            lines.push_back(place_line("verb", UINT32_MAX, verb.first, hud_frame_, verb.second));  // chooses the verb
        }
    }
    for (const std::pair<std::string, ::godot::Rect2>& drop : drops) {
        lines.push_back(place_line("drop", UINT32_MAX, drop.first, hud_frame_, drop.second));  // a drag carried here
    }
    for (const ActionFanPlace& chip : fan) {
        lines.push_back(place_line(chip.act.rack ? "rack" : "take", chip.act.tool, none, hud_frame_, chip.rect));
    }
    if (!focus_hidden) {
        lines.push_back(place_line("focus", UINT32_MAX, none, hud_frame_, ring_focus));  // a tap here goes to rest
    }
    for (const PruneMarkPlace& cut : cuts) {
        lines.push_back(place_line("cut", cut.internode, none, hud_frame_, cut.rect));  // a tap here cuts it
    }
    for (const RackToolPlace& row : rows) {
        lines.push_back(place_line("row", UINT32_MAX, row.id, hud_frame_, row.hit));  // a tap here equips it
    }
    for (const SignMarkerPlace& sign : signs) {
        lines.push_back(place_line("sign", sign.sign, none, hud_frame_, sign.rect));  // a tap here opens the sign
    }
    for (const SignMarkerPlace& sign : rail) {
        lines.push_back(place_line("rail", sign.sign, none, hud_frame_, sign.rect));  // a tap opens the rail's sign
    }
    for (const SignMarkerPlace& event : events) {
        lines.push_back(place_line("event", event.sign, none, hud_frame_, event.rect));  // opens its sign or rests
    }
    // The things on the ground by their item id, as the windows name theirs: a tap picks the thing up.
    for (const GroundThingPlace& place : ground) {
        for (const VivariumHudThing& thing : hud_now_.things) {
            if (thing.key == place.key) {
                lines.push_back(place_line("ground", UINT32_MAX, thing.id, hud_frame_, place.hit));
            }
        }
    }
    // In the mode water a point of the first brook's bed under the middle of its line over the drawn surface
    // (`brook`), the point a tap on the ground asks brookTap at (ACT_GROUND sends ASK_BROOK there).
    // THE MIDDLE OF THE DRAWN LINE IS NO POINT OF THE BED. The line joins the ground under the brook's two ends, and
    // the bed between them is cut below both; the camera's ray through that middle meets the ground beside the
    // brook (run 20261113-v12-brook: "the tap lies 3.750000 m from brook", reach 1.6 m). So the middle is carried
    // back to its share of the world's line - the screen share s over the ends' depths wa and wb is the world share
    // s * wa / (wb * (1 - s) + s * wa) - and the ground there is placed again, the bed the brook runs in.
    // A brook column without a landing pixel: no place line (the set's count names place lines only, and the
    // acceptance takes exactly that many), but part of the set's change, so it is written once on its change.
    std::string unplaced;
    const ::godot::PackedVector2Array& brook = hud_now_.signs.brook;
    const std::vector<VivariumStreamView>& streams = resources_.get_streams();
    ::godot::Camera3D* camera = node_->get_viewport()->get_camera_3d();
    if (hud_.mode() == GODOT_HUD_MODE_WATER && brook.size() >= 2 && !streams.empty() && camera != nullptr) {
        const VivariumStreamView& stream = streams.front();
        const ::godot::Vector2 middle = (brook[0] + brook[brook.size() - 1]) * GODOT_HALF;
        double head_y = 0.0;
        double tail_y = 0.0;
        ::godot::Vector2 head;
        ::godot::Vector2 tail;
        if (hud_ground_.height(stream.x, stream.z, head_y) && hud_ground_.height(stream.end_x, stream.end_z, tail_y) &&
            hud_ground_.surface(hud_anchor_, stream.x, stream.z, 0.0, head) &&
            hud_ground_.surface(hud_anchor_, stream.end_x, stream.end_z, 0.0, tail) && head != tail) {
            const ::godot::Transform3D eye = camera->get_global_transform();
            const float wa = -eye.xform_inv(::godot::Vector3(static_cast<float>(stream.x), static_cast<float>(head_y),
                                                             static_cast<float>(stream.z))).z;
            const float wb = -eye.xform_inv(::godot::Vector3(static_cast<float>(stream.end_x),
                                                             static_cast<float>(tail_y),
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
            // (the nearest projected sample within pick_px_, a later one winning a tie), and it is the middle of
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
            if (span > 0.0f && hud_ground_.surface(hud_anchor_, at_x, at_z, stream.depth_m, face) &&
                hud_ground_.surface(hud_anchor_, at_x - run_z * across, at_z + run_x * across, 0.0, before) &&
                hud_ground_.surface(hud_anchor_, at_x + run_z * across, at_z - run_x * across, 0.0, behind)) {
                const ::godot::Vector2 seen_face = hud_frame_.xform(face);
                const float seen_before = hud_frame_.xform(before).y;
                const float seen_behind = hud_frame_.xform(behind).y;
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
                for (const VivariumTerrainView& sample : resources_.get_terrain()) {
                    const ::godot::Vector3 place(static_cast<float>(sample.x), static_cast<float>(sample.height),
                                                 static_cast<float>(sample.z));
                    ::godot::Vector2 framed;
                    if (!hud_anchor_.place(place, framed)) {
                        continue;
                    }
                    const ::godot::Vector2 seen = hud_frame_.xform(framed);
                    if (ase::math::abs(seen.x - column_x) > pick_px_) {
                        continue;
                    }
                    for (size_t k = 0; k < count; ++k) {
                        const float px =
                            seen.distance_to(::godot::Vector2(column_x, top + static_cast<float>(k)));
                        if (px <= pick_px_ && (best_px[k] < 0.0f || px <= best_px[k])) {
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
                if (run_length > 0) {
                    const size_t middle_k = run_from + run_length / 2;
                    // place_line cuts toward zero, so the rect stands half a pixel inside the tapped one and the
                    // round trip through the frame's inverse cannot fall to the pixel before it.
                    const ::godot::Vector2 tap(column_x + GODOT_HALF, top + static_cast<float>(middle_k) + GODOT_HALF);
                    lines.push_back(place_line("brook", UINT32_MAX, none, hud_frame_,
                                               ::godot::Rect2(hud_frame_.affine_inverse().xform(tap),
                                                              ::godot::Vector2())) +
                                    " pick=" + std::to_string(best_x[middle_k]) + "," +
                                    std::to_string(best_z[middle_k]) + " run=" + std::to_string(run_length) + "/" +
                                    std::to_string(count) + " half=" + std::to_string(stream.half_m));
                } else {
                    unplaced = "debug brook none column x=" + std::to_string(static_cast<int64_t>(column_x)) +
                               " y=" + std::to_string(static_cast<int64_t>(top)) + ".." +
                               std::to_string(static_cast<int64_t>(foot)) + " lands no pixel within half=" +
                               std::to_string(stream.half_m) + " of the brook";
                }
            }
        }
    }
    // The open windows: each thing by its item id (the thing keys k<N> name no item), a tap chooses it, a double
    // tap equips it or opens the pouch; the crosses close the pack and the pouch.
    for (const std::pair<const char*, const std::vector<BagThingPlace>*> window :
         {std::pair<const char*, const std::vector<BagThingPlace>*>("pack", &pack),
          std::pair<const char*, const std::vector<BagThingPlace>*>("pouch", &pouch)}) {
        for (const BagThingPlace& place : *window.second) {
            for (const VivariumHudThing& thing : hud_now_.things) {
                if (thing.key == place.key) {
                    lines.push_back(place_line(window.first, UINT32_MAX, thing.id, hud_frame_, place.box));
                }
            }
        }
    }
    if (!pack_hidden) {
        lines.push_back(place_line("packclose", UINT32_MAX, none, hud_frame_, pack_close));  // a tap closes the pack
    }
    if (!pouch_hidden) {
        lines.push_back(place_line("pouchclose", UINT32_MAX, none, hud_frame_, pouch_close));  // closes the pouch
    }
    if (!pack_grip_hidden) {
        lines.push_back(place_line("packgrip", UINT32_MAX, none, hud_frame_, pack_grip));  // a drag moves the pack
    }
    if (!pouch_grip_hidden) {
        lines.push_back(place_line("pouchgrip", UINT32_MAX, none, hud_frame_, pouch_grip));  // a drag moves the pouch
    }
    if (rack_shown) {
        lines.push_back(place_line("toolrack", UINT32_MAX, none, hud_frame_, tool_rack));  // a tap opens the tool place
    }
    for (const DebugLayerChip& chip : chips) {
        uint32_t index = UINT32_MAX;
        const char* part = place_part(chip.act, index);
        lines.push_back(place_line(part, index, none, hud_frame_, chip.rect));
    }
    std::string joined;
    for (const std::string& line : lines) {
        joined += line;
        joined += '\n';
    }
    joined += unplaced;
    if (joined == hud_places_) {
        return;
    }
    hud_places_ = joined;
    if (!unplaced.empty()) {
        note(::godot::String::utf8(unplaced.c_str()));
    }
    for (const std::string& line : lines) {
        note(::godot::String::utf8(line.c_str()));
    }
    note(::godot::String::utf8(("debug place end count=" + std::to_string(lines.size())).c_str()));
}

}  // namespace ase::adp::godot
