# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Model swaps, round 4 (15 Nov 2026): the launch pads get vehicles shaped like NASA's human Mars architecture
concepts instead of a generic rocket. Run after setup_models3.py (re-runnable):
    exec(open(r"<ProjectDir>/Scripts/setup_models4.py", encoding="utf-8").read())

- LZ-1: the crew's ascent vehicle standing on its descent lander (vertical crew cabin on a methane/oxygen stage,
  on a broad legged lander; NASA MAV studies, Polsgrove et al. 2015-2019, NTRS 20170003391 and 20190001229).
  The existing actor (with its Inspect_Ascent tag) keeps its place and gets the new mesh.
- LZ-2: an uncrewed cargo lander with its payload deck (the cargo that "landed before you").
Sizes are look values ([DESIGNER VALUE]); no NASA number is shown.
"""

import math

import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if les.get_current_level().get_outer().get_name() != "L_TD_Outpost":
    les.load_level("/Game/TouchdownMars/Maps/L_TD_Outpost")
TRIPO = "/Game/TouchdownMars/Tripo/{0}/TD_Tripo_{0}/StaticMeshes/TD_Tripo_{0}"
log = []


def by_label(label):
    return next((a for a in eas.get_all_level_actors() if a.get_actor_label() == label), None)


def ground(x, y, fallback):
    hit = unreal.LandscapeService.get_height_at_location("TD_Terrain", x, y)
    return hit.height if hit.valid else fallback


def stand(actor, mesh, x, y, height, yaw):
    """Put the mesh on the ground at (x, y), centred on its footprint, `height` cm tall."""
    box = mesh.get_bounding_box()
    size = box.max - box.min
    s = height / size.z
    cx, cy = (box.max.x + box.min.x) * 0.5 * s, (box.max.y + box.min.y) * 0.5 * s
    r = math.radians(yaw)
    dx, dy = -(cx * math.cos(r) - cy * math.sin(r)), -(cx * math.sin(r) + cy * math.cos(r))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(s, s, s))
    actor.set_actor_rotation(unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0), False)
    actor.set_actor_location(unreal.Vector(x + dx, y + dy, ground(x, y, 0.0) - 5.0 - box.min.z * s), False, False)
    return size * s


# LZ-1: ascent vehicle on its lander, 11 m tall.
mav = unreal.load_asset(TRIPO.format("MarsAscentVehicle"))
ascent = by_label("TD_Tripo_AscentVehicle")
if mav and ascent:
    size = stand(ascent, mav, 4000.0, -1800.0, 1100.0, 0.0)
    log.append(f"LZ-1 ascent vehicle: {size.x:.0f} x {size.y:.0f} x {size.z:.0f} cm, tags {[str(t) for t in ascent.tags]}")

# LZ-2: cargo lander, 6 m tall, its long side along the pad's approach road.
cargo_mesh = unreal.load_asset(TRIPO.format("CargoLander"))
if cargo_mesh:
    cargo = by_label("TD_M4_CargoLander")
    if not cargo:
        cargo = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(5600.0, -2600.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
        cargo.set_actor_label("TD_M4_CargoLander")
        cargo.set_folder_path("Models4")
    size = stand(cargo, cargo_mesh, 5600.0, -2600.0, 600.0, 20.0)
    log.append(f"LZ-2 cargo lander: {size.x:.0f} x {size.y:.0f} x {size.z:.0f} cm")

les.save_current_level()
print("setup_models4:\n" + "\n".join(log))
