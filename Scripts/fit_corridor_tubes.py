# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Make every corridor tube actually meet the modules it links (no open tube mouths in the air).

For each Tripo corridor tube (TD_M2_Corridor_*), both ends are probed along the tube axis against the module
footprints. Where a module surface is found within REACH, that end is moved to sit EMBED cm inside it;
the tube is then stretched along its length and re-centred. Ends with nothing to meet are reported (a bulkhead cap
goes there). Module footprints are simple shapes (see module_shapes). DRY = True only prints the gaps. Run headless:
    UnrealEditor-Cmd <uproject> -run=pythonscript -script=<ProjectDir>/Scripts/fit_corridor_tubes.py
"""

import unreal

LEVEL = "/Game/TouchdownMars/Maps/L_TD_Outpost"
DRY = globals().get("DRY", False)
REACH = 700.0   # cm searched beyond each tube end
EMBED = 35.0    # cm the tube end sinks into the module wall

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if les.get_current_level().get_outer().get_name() != "L_TD_Outpost":
    les.load_level(LEVEL)
actors = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.StaticMeshActor)]
tubes = [a for a in actors if a.get_actor_label().startswith("TD_M2_Corridor_")]


def long_axis(comp):
    box = comp.get_editor_property("static_mesh").get_bounding_box()
    ext = box.max - box.min
    local = unreal.Vector(1, 0, 0) if ext.x >= ext.y else unreal.Vector(0, 1, 0)
    length = max(ext.x, ext.y)
    centre = (box.max + box.min) * 0.5
    return local, length, centre


# Module footprints at tube height (traces need a physics scene, which a commandlet world does not build).
# circle: (label, x, y, radius at tube height); box: (label, min x, max x, min y, max y). Radii from the meshes'
# bounds (domes and mounds narrowed to their width at the tube's height).
def module_shapes():
    shapes = []
    for a in actors:
        label = a.get_actor_label()
        o, e = a.get_actor_bounds(True)
        if label in ("TD_M2_ECLSS", "TD_M2_Lab"):
            shapes.append(("circle", label, o.x, o.y, min(e.x, e.y) * 0.98))
        elif label in ("TD_Tripo_Greenhouse", "TD_Base_Greenhouse2"):
            shapes.append(("circle", label, o.x, o.y, max(e.x, e.y) * 0.92))
        elif label == "TD_M2_ShelterMound":
            shapes.append(("circle", label, o.x, o.y, max(e.x, e.y) * 0.80))
        elif label == "TD_M2_Airlock":
            shapes.append(("box", label, o.x - e.x, o.x + e.x, o.y - e.y, o.y + e.y))
        elif label == "TD_Art_Habitat":
            # NASA HDU: its bounds include the attached side units; the round core (about 5.8 m across) is what the
            # corridors dock to. Centre and radius measured from a top-down capture of the level.
            shapes.append(("circle", label, -75.0, 0.0, 290.0))
    return shapes


SHAPES = None


def entry(origin, d, t_min, t_max):
    """First point where the ray origin + t d (t in [t_min, t_max]) enters a module footprint."""
    best = None
    for shape in SHAPES:
        if shape[0] == "circle":
            _, label, mx, my, r = shape
            fx, fy = origin.x - mx, origin.y - my
            b = fx * d.x + fy * d.y
            c = fx * fx + fy * fy - r * r
            disc = b * b - c
            if disc < 0:
                continue
            t = -b - disc ** 0.5
        else:
            _, label, x0, x1, y0, y1 = shape
            ts = []
            for (o, dd, lo, hi) in ((origin.x, d.x, x0, x1), (origin.y, d.y, y0, y1)):
                if abs(dd) < 1e-6:
                    if o < lo or o > hi:
                        ts = None
                        break
                    ts.append((-1e9, 1e9))
                else:
                    a1, a2 = (lo - o) / dd, (hi - o) / dd
                    ts.append((min(a1, a2), max(a1, a2)))
            if ts is None:
                continue
            t_in, t_out = max(ts[0][0], ts[1][0]), min(ts[0][1], ts[1][1])
            if t_in > t_out:
                continue
            t = t_in
        if t_min <= t <= t_max and (best is None or t < best[0]):
            best = (t, label)
    return best


SHAPES = module_shapes()
for shape in SHAPES:
    print("SHAPE", shape)
changed = 0
for tube in tubes:
    comp = tube.static_mesh_component
    local_axis, mesh_len, mesh_centre = long_axis(comp)
    xf = comp.get_world_transform()
    axis = xf.transform_direction(local_axis)
    axis = unreal.Vector(axis.x, axis.y, 0.0).normal()
    scale = comp.get_world_scale()
    axis_scale = scale.x if local_axis.x else scale.y
    length = mesh_len * axis_scale
    centre = xf.transform_location(mesh_centre)
    ends = []
    for sign in (1.0, -1.0):
        d = axis * sign
        end = centre + d * (length * 0.5)
        # Start a little inside the tube so a module the tube already sinks into is still found.
        found = entry(centre, d, length * 0.5 - 250.0, length * 0.5 + REACH)
        hit, label = (centre + d * found[0], found[1]) if found else (None, None)
        if hit is None:
            ends.append((end, None, None))
            print(f"OPEN   {tube.get_actor_label():28s} end {'+' if sign > 0 else '-'} at {end.x:.0f},{end.y:.0f}: nothing within {REACH:.0f} cm")
            continue
        gap = (hit - end).dot(d)
        target = hit + d * EMBED
        ends.append((end, target, label))
        print(f"{'GAP' if gap > -EMBED else 'OK':6s} {tube.get_actor_label():28s} end {'+' if sign > 0 else '-'} gap {gap:6.0f} cm to {label}")
    if DRY:
        continue
    new_a = ends[0][1] if ends[0][1] is not None else ends[0][0]
    new_b = ends[1][1] if ends[1][1] is not None else ends[1][0]
    new_len = (new_a - new_b).dot(axis)
    if abs(new_len - length) < 5.0:
        continue
    factor = new_len / length
    new_scale = unreal.Vector(scale.x * factor if local_axis.x else scale.x, scale.y * factor if local_axis.y else scale.y, scale.z)
    tube.set_actor_scale3d(new_scale)
    # Re-centre: move so the mesh centre lands on the midpoint of the new ends (keep height).
    mid = (new_a + new_b) * 0.5
    now = comp.get_world_transform().transform_location(mesh_centre)
    shift = unreal.Vector(mid.x - now.x, mid.y - now.y, 0.0)
    tube.set_actor_location(tube.get_actor_location() + shift, False, False)
    changed += 1
    print(f"FITTED {tube.get_actor_label():28s} length {length:.0f} -> {new_len:.0f} cm")

if not DRY and changed:
    les.save_current_level()
print(f"fit_corridor_tubes: {len(tubes)} tubes, {changed} fitted, dry={DRY}")
