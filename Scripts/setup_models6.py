# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Round 6: the pressurised corridors become rows of the rigid Tripo tunnel segment (TD_Tripo_CorridorSegment,
AI-generated, sources.csv) instead of one round-2 CorridorTube stretched up to 4x along its length.
Each corridor keeps its axis, diameter and length from its placeholder cylinder (TD_Base_Corridor_<name>); the
segment count is the length divided by one segment at that diameter, so the stretch stays within about 15 %.
The design and ops tags of the corridor move onto its segments (RefreshDesignScene shows and hides them), and
the round-2 tube, ring and saddle placeholders are hidden. Re-runnable. Run after setup_models2.py:
    exec(open(r"<ProjectDir>/Scripts/setup_models6.py", encoding="utf-8").read())
"""

import math

import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if les.get_current_level().get_outer().get_name() != "L_TD_Outpost":
    les.load_level("/Game/TouchdownMars/Maps/L_TD_Outpost")

MESH = "/Game/TouchdownMars/Tripo/CorridorSegment/TD_Tripo_CorridorSegment/StaticMeshes/TD_Tripo_CorridorSegment"
CORRIDORS = ("Airlock", "ECLSS", "Greenhouse", "Greenhouse2", "Lab", "Shelter")
log = []


def actors():
    return eas.get_all_level_actors()


def by_label(label):
    return next((a for a in actors() if a.get_actor_label() == label), None)


def hide(actor):
    actor.set_actor_hidden_in_game(True)
    actor.set_actor_enable_collision(False)
    actor.set_is_temporarily_hidden_in_editor(True)


# Re-runnable: remove old segments, give the round-2 tubes their tags back.
for a in actors():
    if a.get_actor_label().startswith("TD_M6_Seg_"):
        a.destroy_actor()
for a in actors():
    tags = [str(t) for t in a.tags]
    kept = next((t for t in tags if t.startswith("M6Tags=")), None)
    if kept is not None:
        a.tags = [unreal.Name(t) for t in kept[len("M6Tags="):].split(";") if t]

mesh = unreal.load_asset(MESH)
if not mesh:
    raise RuntimeError("CorridorSegment mesh missing: import it first")
box = mesh.get_bounding_box()
size = box.max - box.min
long_is_x = size.x >= size.y
mesh_len = size.x if long_is_x else size.y
mesh_wid = size.y if long_is_x else size.x
tube_centre_z = box.max.z - mesh_wid * 0.5          # the tube is round: its top minus half its width
mesh_mid = ((box.max.x + box.min.x) * 0.5, (box.max.y + box.min.y) * 0.5)

for name in CORRIDORS:
    ph = by_label(f"TD_Base_Corridor_{name}")
    tube = by_label(f"TD_M2_Corridor_{name}")
    if not ph:
        log.append(f"skip {name}: no placeholder")
        continue
    scale3 = ph.get_actor_scale3d()
    axis = ph.get_actor_rotation().get_up_vector()
    length = 100.0 * scale3.z
    diameter = 100.0 * scale3.x
    origin = ph.get_actor_location()
    yaw = math.degrees(math.atan2(axis.y, axis.x))
    s_round = diameter / mesh_wid
    count = max(1, round(length / (mesh_len * s_round)))
    s_len = (length / count) / mesh_len
    mesh_yaw = yaw if long_is_x else yaw - 90.0
    # Tags that show or hide this corridor (design / ops); the round-2 tube carries them now.
    source = tube if tube else ph
    tags = [t for t in (str(x) for x in source.tags) if not t.startswith("M2From_") and not t.startswith("M6")]
    r = math.radians(mesh_yaw)
    for i in range(count):
        along = -length * 0.5 + (i + 0.5) * length / count
        cx, cy = origin.x + axis.x * along, origin.y + axis.y * along
        # Move the mesh centre (not its pivot) onto the axis point.
        ox, oy = -mesh_mid[0] * (s_len if long_is_x else s_round), -mesh_mid[1] * (s_round if long_is_x else s_len)
        dx, dy = ox * math.cos(r) - oy * math.sin(r), ox * math.sin(r) + oy * math.cos(r)
        z = origin.z - tube_centre_z * s_round
        seg = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx + dx, cy + dy, z),
                                         unreal.Rotator(pitch=0.0, yaw=mesh_yaw, roll=0.0))
        seg.set_actor_label(f"TD_M6_Seg_{name}_{i}")
        seg.set_folder_path("Models6")
        seg.static_mesh_component.set_static_mesh(mesh)
        seg.set_actor_scale3d(unreal.Vector(s_len, s_round, s_round) if long_is_x else unreal.Vector(s_round, s_len, s_round))
        seg.tags = [unreal.Name(t) for t in tags] + [unreal.Name(f"M6From_{name}")]
        if ph.is_hidden_ed() and tube and tube.is_hidden_ed():
            seg.set_is_temporarily_hidden_in_editor(True)
    # The round-2 tube, rings and saddles go away; keep their tags so a re-run can give them back.
    for part in [a for a in actors() if a.get_actor_label() == f"TD_M2_Corridor_{name}"
                 or a.get_actor_label().startswith(f"TD_Base_Corridor_{name}_Rib")
                 or a.get_actor_label().startswith(f"TD_Base_Corridor_{name}_Leg")]:
        old_tags = [str(t) for t in part.tags]
        part.tags = [unreal.Name("M6Tags=" + ";".join(old_tags))]
        hide(part)
    log.append(f"{name}: {count} segments, {length:.0f} cm long, diameter {diameter:.0f}, stretch {s_len / s_round:.2f}, tags {tags}")

les.save_current_level()
print("\n".join(log))
