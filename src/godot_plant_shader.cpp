/**
 * ASE GODOT ADAPTER - THE SHADERS OF THE CARE VIEW
 *
 * @file        godot_plant_shader.cpp
 * @brief       Loads a shader of the care view's 3D scene from its file below res://shaders/
 * @description See godot_plant_shader.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_plant_shader.hpp>

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ase::adp::godot {

::godot::Ref<::godot::Shader> load_plant_shader(const char* file) {
    // ResourceLoader runs the shader preprocessor over the file: an #include of a block below res://shaders/ is
    // set in at its place before the shader compiles. That is how plant_stand_in, plant_organ and plant_bark take
    // the swing of a branch - the uniforms of sway_uniforms.gdshaderinc in their head, the swing of
    // sway_swing.gdshaderinc in their vertex() - so no unit splices shader text at run time. The loaded shader
    // stays in the resource cache: every material of the same file shares one shader.
    const ::godot::Ref<::godot::Shader> shader = ::godot::ResourceLoader::get_singleton()->load(file, "Shader");
    if (shader.is_null()) {
        ::godot::UtilityFunctions::push_error(::godot::String("shader missing ") + file);
    }
    return shader;
}

}  // namespace ase::adp::godot
