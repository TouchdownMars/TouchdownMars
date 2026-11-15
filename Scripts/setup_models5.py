# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Round 6: the life-support (B2) and lab (B3) modules get new hard-surface Tripo habitat modules
(TD_Tripo_HabModule2 and HabModule3, AI-generated, sources.csv) in place of the round-2 HabModule. Each keeps its place, its
yaw and its height (the HDU-copy height setup_models2.py fitted, [DESIGNER VALUE]); the mesh aspect is kept.
Run after setup_models2.py, in the editor or headless:
    exec(open(r"<ProjectDir>/Scripts/setup_models5.py", encoding="utf-8").read())
Optional globals: YAW_OFFSET (degrees added to each module's yaw, to turn the hatch toward its corridor).
"""

import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if les.get_current_level().get_outer().get_name() != "L_TD_Outpost":
    les.load_level("/Game/TouchdownMars/Maps/L_TD_Outpost")

TRIPO = "/Game/TouchdownMars/Tripo/{0}/TD_Tripo_{0}/StaticMeshes/TD_Tripo_{0}"
# B2 life support: boxy hard-shell module (HabModule2); B3 lab: two-storey cylinder on legs (HabModule3).
MODULES = {"TD_M2_ECLSS": "HabModule2", "TD_M2_Lab": "HabModule3"}
YAW_OFFSET = globals().get("YAW_OFFSET", {"TD_M2_ECLSS": 0.0, "TD_M2_Lab": 0.0})
log = []
for label, name in MODULES.items():
    mesh = unreal.load_asset(TRIPO.format(name))
    actor = next((a for a in eas.get_all_level_actors() if a.get_actor_label() == label), None)
    if not actor or not mesh:
        log.append(f"skip {label}: actor {bool(actor)}, mesh {bool(mesh)}")
        continue
    comp = actor.static_mesh_component
    old = comp.static_mesh
    ob = old.get_bounding_box()
    height = (ob.max.z - ob.min.z) * actor.get_actor_scale3d().z   # keep the module height
    nb = mesh.get_bounding_box()
    s = height / (nb.max.z - nb.min.z)
    loc = actor.get_actor_location()
    # The round-2 mesh may have been tagged with its base yaw; remember it once so YAW_OFFSET is re-runnable.
    base_tag = next((str(t) for t in actor.tags if str(t).startswith("M5BaseYaw_")), None)
    if base_tag is None:
        base_yaw = actor.get_actor_rotation().yaw
        actor.tags = list(actor.tags) + [unreal.Name(f"M5BaseYaw_{base_yaw:.1f}")]
    else:
        base_yaw = float(base_tag[len("M5BaseYaw_"):])
    comp.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(s, s, s))
    yaw = base_yaw + YAW_OFFSET.get(label, 0.0)
    actor.set_actor_rotation(unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0), False)
    # Bottom of the mesh on the old base height.
    actor.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z), False, False)
    o, e = actor.get_actor_bounds(False)
    log.append(f"{label}: {name} scale {s:.2f}, yaw {yaw:.0f}, size {2*e.x:.0f} x {2*e.y:.0f} x {2*e.z:.0f}, bottom {o.z - e.z:.0f}")
les.save_current_level()
print("\n".join(log))
