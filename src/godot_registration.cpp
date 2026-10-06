/**
 * ASE GODOT ADAPTER - GDEXTENSION REGISTRATION
 *
 * @file        godot_registration.cpp
 * @brief       Entry function vivarium_library_init and the class binding of AseVivariumView
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.2. The entry hands initializer and terminator
 *              to godot::GDExtensionBinding::InitObject; the initializer registers the class
 *              AseVivariumView at MODULE_INITIALIZATION_LEVEL_SCENE, the terminator removes it at
 *              the same level.
 *
 *              THE CLASS IS REGISTERED THROUGH THE GDEXTENSION CLASS INTERFACE.
 *              classdb_register_extension_class5 takes the class name, the engine class its
 *              objects are built from (Node2D) and a table of plain functions: create builds the
 *              engine object and attaches one VivariumView to it as its instance data, free
 *              destroys that view, view_callback answers which engine callbacks the view takes
 *              over, and every notification of the object is passed on. No C++ type derives from
 *              a Godot type - the view holds its engine object, it is not one.
 *
 *              THE VIEW LIVES IN GODOT'S ALLOCATOR AND BELONGS TO ITS ENGINE OBJECT: create
 *              allocates it with memnew, Godot calls free exactly once when it deletes the object,
 *              and free releases the view with memdelete. Nothing else holds or frees it.
 *
 *              THE TERMINATOR REMOVES THE CLASS ITSELF: godot-cpp's binding unregisters only the
 *              classes registered through its own ClassDB, and this one is not among them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-05
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_registration.hpp>
#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>

#include <gdextension_interface.h>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/method_ptrcall.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

VivariumView* view_of(GDExtensionClassInstancePtr instance) {
    return static_cast<VivariumView*>(instance);
}

// ── the engine callbacks the view takes over, called by Godot with ptrcall arguments ──────

void call_ready(GDExtensionClassInstancePtr instance, const GDExtensionConstTypePtr* args,
                GDExtensionTypePtr result) {
    (void)args;
    (void)result;
    view_of(instance)->ready();
}

void call_process(GDExtensionClassInstancePtr instance, const GDExtensionConstTypePtr* args,
                  GDExtensionTypePtr result) {
    (void)result;
    view_of(instance)->process(::godot::PtrToArg<double>::convert(args[0]));
}

void call_draw(GDExtensionClassInstancePtr instance, const GDExtensionConstTypePtr* args,
               GDExtensionTypePtr result) {
    (void)args;
    (void)result;
    view_of(instance)->draw();
}

void call_unhandled_input(GDExtensionClassInstancePtr instance, const GDExtensionConstTypePtr* args,
                          GDExtensionTypePtr result) {
    (void)result;
    view_of(instance)->unhandled_input(
        ::godot::PtrToArg<const ::godot::Ref<::godot::InputEvent>&>::convert(args[0]));
}

void call_exit_tree(GDExtensionClassInstancePtr instance, const GDExtensionConstTypePtr* args,
                    GDExtensionTypePtr result) {
    (void)args;
    (void)result;
    view_of(instance)->exit_tree();
}

bool asks_for(const ::godot::StringName& name, uint32_t hash, const char* callback,
              uint32_t callback_hash) {
    return hash == callback_hash && name == ::godot::StringName(callback);
}

/**
 * Godot asks once per callback name and signature hash. A name the view does not take over, or
 * a hash of another signature, is answered with nullptr and stays the engine class's own.
 */
GDExtensionClassCallVirtual view_callback(void* class_userdata, GDExtensionConstStringNamePtr name,
                                          uint32_t hash) {
    (void)class_userdata;
    const ::godot::StringName& asked = *static_cast<const ::godot::StringName*>(name);
    if (asks_for(asked, hash, GODOT_HOOK_READY, GODOT_HOOK_READY_HASH)) {
        return &call_ready;
    }
    if (asks_for(asked, hash, GODOT_HOOK_PROCESS, GODOT_HOOK_PROCESS_HASH)) {
        return &call_process;
    }
    if (asks_for(asked, hash, GODOT_HOOK_DRAW, GODOT_HOOK_DRAW_HASH)) {
        return &call_draw;
    }
    if (asks_for(asked, hash, GODOT_HOOK_INPUT, GODOT_HOOK_INPUT_HASH)) {
        return &call_unhandled_input;
    }
    if (asks_for(asked, hash, GODOT_HOOK_EXIT, GODOT_HOOK_EXIT_HASH)) {
        return &call_exit_tree;
    }
    return nullptr;
}

void view_notification(GDExtensionClassInstancePtr instance, int32_t what, GDExtensionBool reversed) {
    (void)reversed;
    view_of(instance)->notification(what);
}

/**
 * One object of the class AseVivariumView: the engine Node2D, its godot-cpp handle and the
 * VivariumView attached as its instance data.
 */
GDExtensionObjectPtr create_view(void* class_userdata, GDExtensionBool notify_postinitialize) {
    (void)class_userdata;
    const ::godot::StringName engine_class(GODOT_VIEW_ENGINE_CLASS);
    const ::godot::StringName view_class(GODOT_VIEW_CLASS);
    GDExtensionObjectPtr object =
        ::godot::internal::gdextension_interface_classdb_construct_object2(engine_class._native_ptr());
    ::godot::Node2D* node =
        ::godot::Object::cast_to<::godot::Node2D>(::godot::internal::get_object_instance_binding(object));
    if (node == nullptr) {
        ::godot::UtilityFunctions::push_error(::godot::String(GODOT_VIEW_CLASS) + ": no " +
                                              GODOT_VIEW_ENGINE_CLASS + " object to attach the view to");
        if (object != nullptr) {
            ::godot::internal::gdextension_interface_object_destroy(object);
        }
        return nullptr;
    }
    ::godot::internal::gdextension_interface_object_set_instance(object, view_class._native_ptr(),
                                                                 memnew(VivariumView(node)));
    if (notify_postinitialize) {
        // classdb_construct_object2 leaves this notification to whoever completes the object.
        node->notification(::godot::Object::NOTIFICATION_POSTINITIALIZE);
    }
    return object;
}

void free_view(void* class_userdata, GDExtensionClassInstancePtr instance) {
    (void)class_userdata;
    ::godot::memdelete(view_of(instance));
}

}  // anonymous namespace

void initialize_vivarium_module(::godot::ModuleInitializationLevel level) {
    if (level != ::godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    // Every field not set here stays zero: instantiable, not a runtime-only class, no icon, no
    // properties, no reference counting, no hot reload.
    GDExtensionClassCreationInfo5 info = {};
    info.is_exposed = true;
    info.notification_func = &view_notification;
    info.create_instance_func = &create_view;
    info.free_instance_func = &free_view;
    info.get_virtual_func = &view_callback;

    const ::godot::StringName view_class(GODOT_VIEW_CLASS);
    const ::godot::StringName engine_class(GODOT_VIEW_ENGINE_CLASS);
    ::godot::internal::gdextension_interface_classdb_register_extension_class5(
        ::godot::internal::library, view_class._native_ptr(), engine_class._native_ptr(), &info);
}

void uninitialize_vivarium_module(::godot::ModuleInitializationLevel level) {
    if (level != ::godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    const ::godot::StringName view_class(GODOT_VIEW_CLASS);
    ::godot::internal::gdextension_interface_classdb_unregister_extension_class(::godot::internal::library,
                                                                               view_class._native_ptr());
}

GDExtensionBool GDE_EXPORT vivarium_library_init(GDExtensionInterfaceGetProcAddress get_proc_address,
                                                 GDExtensionClassLibraryPtr library,
                                                 GDExtensionInitialization* initialization) {
    ::godot::GDExtensionBinding::InitObject init(get_proc_address, library, initialization);
    init.register_initializer(initialize_vivarium_module);
    init.register_terminator(uninitialize_vivarium_module);
    init.set_minimum_library_initialization_level(::godot::MODULE_INITIALIZATION_LEVEL_SCENE);
    return init.init();
}

}  // namespace ase::adp::godot
