# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Model swaps, round 2 (research_notes/Mars uessue modelleri/model_plan.md section 4.3). Run LAST, after
setup_fps.py, inside the editor (PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/setup_models2.py", encoding="utf-8").read())

- Relay antenna: the grey cone TD_Base_RelayDish becomes the NASA 3D Resources Radome on the relay mast
  (size [DESIGNER VALUE]: 1.5 m tall; the surface relay is a fixed-pointing design, DRA5 addendum Tbl 4-15).
- Rover: the grey box TD_Rover is deleted and its "OpsShow_OffloadRover" tag moves to the NASA SEV, so the
  real rover appears when the crew off-loads it and leaves while it is out on a traverse.
A swap copies every tag (DesignShow_, OpsShow_, Inspect_) to the new actor and removes them from the
placeholder, then hides it; otherwise the mission scene would show the grey shape again.
"""

import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if les.get_current_level().get_outer().get_name() != "L_TD_Outpost":
    les.load_level("/Game/TouchdownMars/Maps/L_TD_Outpost")


def by_label(label):
    return next((a for a in eas.get_all_level_actors() if a.get_actor_label() == label), None)


for actor in eas.get_all_level_actors():
    if actor.get_actor_label().startswith("TD_M2_"):
        eas.destroy_actor(actor)
log = []


def swap(placeholder_label, mesh_path, label, location, scale, yaw=0.0):
    old = by_label(placeholder_label)
    mesh = unreal.load_asset(mesh_path)
    if not old or not mesh:
        log.append(f"skip {label}: placeholder {bool(old)}, mesh {bool(mesh)}")
        return None
    new = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(0, yaw, 0))
    new.set_actor_label(label)
    new.set_folder_path("Models2")
    comp = new.static_mesh_component
    comp.set_static_mesh(mesh)
    new.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    new.tags = list(old.tags)
    old.tags = []
    old.set_actor_hidden_in_game(True)
    old.set_actor_enable_collision(False)
    old.set_is_temporarily_hidden_in_editor(True)
    log.append(f"{placeholder_label} -> {label}, tags {[str(t) for t in new.tags]}")
    return new


# Relay: radome base on the mast top (mast spans 0..650 cm).
mast = by_label("TD_Base_RelayMast")
top = mast.get_actor_location().z + mast.get_actor_bounds(False)[1].z if mast else 650.0
swap("TD_Base_RelayDish", "/Game/TouchdownMars/NASA/Radome/Radome_static/StaticMeshes/Radome_static",
     "TD_M2_Radome", (900.0, 1450.0, top), 150.0 / 2200.0)

# Rover: the NASA SEV takes the off-load tag; the grey box goes.
sev = by_label("TD_Prop_SEV")
box = by_label("TD_Rover")
if sev:
    tags = [str(t) for t in sev.tags]
    if "OpsShow_OffloadRover" not in tags:
        sev.tags = list(sev.tags) + [unreal.Name("OpsShow_OffloadRover")]
    log.append(f"TD_Prop_SEV tags: {[str(t) for t in sev.tags]}")
if box:
    eas.destroy_actor(box)
    log.append("TD_Rover box deleted")

les.save_current_level()
print("\n".join(log))
