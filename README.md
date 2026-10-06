# ASE Adp Godot

> **ASE Adp Godot** `00.00.01.00001` — Adapter (3rd-party isolation, consumed by L5 clients) | Status: seed

Godot 4.5 GDExtension adapter of the Antares Vivarium client (`clients/avt-client-android`).
It is the only place that registers a Godot class and holds the GDExtension entry. The
simulation stays in ASE: one embedded host, one registry, the vegetation plugin loaded
dynamically — Godot draws and takes input.

No C++ type of the adapter derives from a Godot type, and the entry keeps C++ linkage. The class
`AseVivariumView` is registered through the GDExtension class interface: its objects are engine
`Node2D` objects, each carrying one `VivariumView` as instance data, which draws and reads input
through the godot-cpp handle of its object.

## Parts

| Part | Content |
|------|---------|
| `GodotHostResourceManager` | One `KernelEmbeddedHost`: load the plugin from the bundle, check the port, configure the four patches, start, tick with `dt` capped, read the snapshot, stop. No Godot type — tested without Godot. |
| `VivariumView` | Instance data of an `AseVivariumView` object: reads `res://config/vivarium_start.json`, drives the manager from `_process`, draws patches, selection and the actions Regar / Pausar / Reiniciar, takes touch and mouse taps. |
| `vivarium_library_init` | GDExtension entry `ase::adp::godot::vivarium_library_init`: registers `AseVivariumView` at the scene level. Its exported (mangled) name is read from the built library by the client's `scripts/stage_native.py` and written into `avt.gdextension`. |

## Build

Built inside the embedded closure of the client, never on its own:

```bash
ase avt -B
```

The adapter binds `godot::cpp` (godot-cpp `godot-4.5-stable`) and `ase::kernel-embedded`; it never
links the vegetation plugin. Plan: `docs/ase-docs/tech/clients/plans/vivarium/PLAN_ASE_VIVARIUM_PHASE_01_INTEG.md`.
