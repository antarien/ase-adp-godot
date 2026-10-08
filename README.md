# ASE Adp Godot

> **ASE Adp Godot** `00.00.01.00001` — Adapter (3rd-party isolation, consumed by L5 clients) | Status: seed

Godot 4.5 GDExtension adapter of the Antares Vivarium client (`clients/avt-client-android`).
It is the only place that registers a Godot class and holds the GDExtension entry. The
simulation stays in ASE: one embedded host and one registry. The host loads the unit set around
the plugin `ase-pl-flora` by its manifests (`ase-hub`, `ase-time`, `ase-calendar`,
`ase-ephemeris`, `ase-hydro`, `ase-plant`, `ase-sdk`, `ase-pl-flora`). Godot draws and takes input.

No C++ type of the adapter derives from a Godot type, and the entry keeps C++ linkage. The class
`AseVivariumView` is registered through the GDExtension class interface: its objects are engine
`Node2D` objects, each carrying one `VivariumView` as instance data, which draws and reads input
through the godot-cpp handle of its object.

## Parts

| Part | Content |
|------|---------|
| `GodotHostResourceManager` | One `KernelEmbeddedHost`. It reads the bundle index, names the data root, loads every unit with its manifest (`load_units`) and checks the ports `flora.patch.v1` and `flora.clock.v1`. Then it starts the host, hands it the real time of every frame (`advance`: a long absence is caught up in ordinary ticks, there is no pause), reads both snapshots, submits a pour as the request `irrigate` and stops. It also runs the self-test table. No Godot type — tested without Godot. |
| `VivariumView` | Instance data of an `AseVivariumView` object. It reads `res://config/vivarium_start.json` and stages the data files (inside an APK they are copied to `user://bundle` first). It drives the manager from `_process` and draws the four places with coverage, stage, condition and soil water, the selection and the actions Regar and Reiniciar. It takes touch and mouse taps and logs every event as one `[AVT]` line. |
| `vivarium_library_init` | GDExtension entry `ase::adp::godot::vivarium_library_init`: registers `AseVivariumView` at the scene level. Its exported (mangled) name is read from the built library by the client's `scripts/stage_native.py` and written into `avt.gdextension`. |

## Build

Built inside the embedded closure of the client, never on its own:

```bash
ase avt -B
```

The adapter binds `godot::cpp` (godot-cpp `godot-4.5-stable`) and `ase::kernel-embedded`. It never
links a unit of the set: the host loads them at run time. Plan: `docs/ase-docs/tech/clients/plans/vivarium/PLAN_ASE_VIVARIUM_PHASE_01_INTEG.md`.
