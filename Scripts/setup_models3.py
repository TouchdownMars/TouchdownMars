# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Model swaps, round 3 (15 Nov 2026): the last greybox the player can see becomes Tripo models, and leftover
greybox pieces that float with nothing around them are hidden. Run after setup_models2.py (re-runnable):
    exec(open(r"<ProjectDir>/Scripts/setup_models3.py", encoding="utf-8").read())

- Solar field: the 24 grey panel boxes and their legs become ground-mounted solar array tables, tilted to the
  south (Jezero is north of the equator; +Y is south in this level).
- ISRU to life-support pipe: the grey cylinder, its rings and legs become an insulated pipe run on trestles.
- MOXIE-type oxygen unit next to the ISRU plant: the grey box becomes the compact oxygen unit.
- Leftovers: the corridor rings and legs from the first greybox (the tubes are Tripo models since round 2), the
  airlock legs and the water-wall bands stood alone in the sand; they are hidden.
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


def hide(actor):
    actor.set_actor_hidden_in_game(True)
    actor.set_actor_enable_collision(False)
    actor.set_is_temporarily_hidden_in_editor(True)


# Re-runnable: remove the round-3 actors and show their placeholders again (tag M3From_<label>;<label>...).
for actor in list(eas.get_all_level_actors()):
    if actor.get_actor_label().startswith("TD_M3_"):
        for t in [str(t) for t in actor.tags]:
            if t.startswith("M3From_"):
                for label in t[len("M3From_"):].split(";"):
                    old = by_label(label)
                    if old:
                        old.set_actor_hidden_in_game(False)
                        old.set_actor_enable_collision(True)
                        old.set_is_temporarily_hidden_in_editor(False)
        eas.destroy_actor(actor)


def mesh_info(name):
    mesh = unreal.load_asset(TRIPO.format(name))
    if not mesh:
        return None, None, None
    box = mesh.get_bounding_box()
    return mesh, box, box.max - box.min


def spawn(name, label, x, y, z, yaw, scale, replaces):
    mesh, box, size = mesh_info(name)
    if not mesh:
        log.append(f"skip {label}: no mesh {name}")
        return None
    centre_x, centre_y = (box.max.x + box.min.x) * 0.5, (box.max.y + box.min.y) * 0.5
    r = math.radians(yaw)
    ox, oy = -centre_x * scale, -centre_y * scale
    dx, dy = ox * math.cos(r) - oy * math.sin(r), ox * math.sin(r) + oy * math.cos(r)
    new = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x + dx, y + dy, z - box.min.z * scale),
                                     unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0))
    new.set_actor_label(label)
    new.set_folder_path("Models3")
    new.static_mesh_component.set_static_mesh(mesh)
    new.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    tags = []
    for old_label in replaces:
        old = by_label(old_label)
        if old:
            tags += [t for t in old.tags if str(t) not in [str(n) for n in tags]]
            old.tags = []
            hide(old)
    new.tags = tags + [unreal.Name("M3From_" + ";".join(replaces))]
    return new


# Solar field: one table per grey panel, 3.4 m wide so the rows (2.6 m apart) keep a walkway, long side
# east-west, facing south.
mesh, box, size = mesh_info("SolarArray")
if mesh:
    long_x = size.x >= size.y
    s = 340.0 / max(size.x, size.y)
    count = 0
    for field in (1, 2, 3):
        for row in range(4):
            for col in range(2):
                panel = f"TD_Base_Solar{field}_{row}_{col}"
                leg = f"TD_Base_SolarLeg{field}_{row}_{col}"
                old = by_label(panel)
                if not old:
                    continue
                o = old.get_actor_location()
                yaw = 0.0 if long_x else 90.0
                spawn("SolarArray", f"TD_M3_Solar{field}_{row}_{col}", o.x, o.y, ground(o.x, o.y, 0.0) - 3.0, yaw, s, [panel, leg])
                count += 1
    log.append(f"solar tables: {count}, scale {s:.3f}, long axis {'X' if long_x else 'Y'}")

# ISRU -> ECLSS pipe: segments of the pipe run laid end to end along the grey cylinder's axis, the pipes at
# about waist height (segment scale from the run's length, so the trestles keep their shape).
pipe = by_label("TD_Base_Pipe_ISRU_ECLSS")
mesh, box, size = mesh_info("PipeRun")
if pipe and mesh:
    axis = pipe.get_actor_rotation().get_up_vector()
    axis = unreal.Vector(axis.x, axis.y, 0.0).normal()
    length = 100.0 * pipe.get_actor_scale3d().z
    seg_len = max(size.x, size.y)
    count = max(1, round(length / (seg_len * 3.4)))
    s = length / count / seg_len
    yaw = math.degrees(math.atan2(axis.y, axis.x)) + (0.0 if size.x >= size.y else 90.0)
    o = pipe.get_actor_location()
    parts = ["TD_Base_Pipe_ISRU_ECLSS"] + [f"TD_Base_Pipe_ISRU_ECLSS_{k}{i}" for k in ("Rib", "Leg") for i in (1, 2, 3)]
    for i in range(count):
        t = (i + 0.5) / count - 0.5
        x, y = o.x + axis.x * length * t, o.y + axis.y * length * t
        spawn("PipeRun", f"TD_M3_PipeISRU{i}", x, y, ground(x, y, 0.0) - 3.0, yaw, s, parts if i == 0 else [])
    log.append(f"pipe run: length {length:.0f} cm in {count} segments, yaw {yaw:.0f}, scale {s:.3f}")

# Oxygen unit: as tall as the grey box (90 cm), on the ground.
moxie = by_label("TD_Base_MOXIE_Unit")
mesh, box, size = mesh_info("MoxieUnit")
if moxie and mesh:
    o, e = moxie.get_actor_bounds(False)
    s = 2.0 * e.z / size.z * 1.25
    spawn("MoxieUnit", "TD_M3_Moxie", o.x, o.y, ground(o.x, o.y, 0.0) - 2.0, moxie.get_actor_rotation().yaw, s, ["TD_Base_MOXIE_Unit"])
    log.append(f"oxygen unit: scale {s:.3f}")

# Leftover greybox that stands alone in the sand.
leftovers = [a for a in eas.get_all_level_actors()
             if a.get_actor_label().startswith(("TD_Base_Corridor_", "TD_Base_Airlock_Leg", "TD_Base_WaterWallBand"))]
def owner_of(label):
    """The model that stands where this leftover was; it inherits the leftover's show tags (the mission scene
    shows and hides actors by tag at runtime, so a tagged leftover would come back)."""
    if label.startswith("TD_Base_Corridor_"):
        for name in ("Greenhouse2", "Greenhouse", "Airlock", "ECLSS", "Lab", "Shelter"):
            if label.startswith(f"TD_Base_Corridor_{name}"):
                return by_label(f"TD_M2_Corridor_{name}")
    if label.startswith("TD_Base_Airlock_Leg"):
        return by_label("TD_M2_Airlock")
    if label.startswith("TD_Base_WaterWallBand"):
        return by_label(f"TD_M2_WaterWall{label[-1]}")
    return None


moved = 0
for a in leftovers:
    kept = [str(t) for t in a.tags if str(t).startswith(("DesignShow_", "OpsShow_", "Inspect_"))]
    owner = owner_of(a.get_actor_label())
    if kept and owner:
        have = [str(t) for t in owner.tags]
        owner.tags = list(owner.tags) + [unreal.Name(t) for t in kept if t not in have]
        moved += 1
    a.tags = []
    hide(a)
log.append(f"leftover tags moved to their models: {moved}")
log.append(f"hidden leftovers: {len(leftovers)}")

# Docking collars: every corridor tube meets a module through a collar (a flanged bulkhead ring), never a raw
# tube mouth (pressurized joints are visible hardware; Skyrim kit-building talk, GDC 2013: joint pieces at every
# connection). Run fit_corridor_tubes.py first so the tube ends sit on the module walls.
mesh, box, size = mesh_info("CorridorEndCap")
collars = 0
if mesh:
    cap_axis_x = size.x >= size.y            # the bulkhead's axis is its longer horizontal side
    cap_dia = min(size.x, size.y)
    for tube in [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith("TD_M2_Corridor_")]:
        comp = tube.static_mesh_component
        tb = comp.get_editor_property("static_mesh").get_bounding_box()
        ext = tb.max - tb.min
        local = unreal.Vector(1, 0, 0) if ext.x >= ext.y else unreal.Vector(0, 1, 0)
        xf = comp.get_world_transform()
        centre = xf.transform_location((tb.max + tb.min) * 0.5)
        axis = xf.transform_direction(local)
        axis = unreal.Vector(axis.x, axis.y, 0.0).normal()
        scale = tube.get_actor_scale3d()
        half = 0.5 * max(ext.x, ext.y) * (scale.x if local.x else scale.y)
        tube_dia = min(ext.y if local.x else ext.x, ext.z) * (scale.y if local.x else scale.x)
        s = tube_dia * 1.18 / cap_dia
        for sign in (1.0, -1.0):
            d = axis * sign
            end = centre + d * (half - 30.0)
            yaw = math.degrees(math.atan2(d.y, d.x)) + (0.0 if cap_axis_x else 90.0)
            # Centre the collar on the tube axis: bottom of the collar = tube centre height - half its height.
            z = centre.z - 0.5 * size.z * s
            spawn("CorridorEndCap", f"TD_M3_Collar_{tube.get_actor_label()[15:]}_{'a' if sign > 0 else 'b'}",
                  end.x, end.y, z, yaw, s, [])
            collars += 1
log.append(f"docking collars: {collars}")

# Fission units: a regolith berm between the reactors and the base (real surface reactors sit far from the
# habitat and use regolith as shielding; DRA 5.0 surface power). The cable trays pass under it.
mound = unreal.load_asset(TRIPO.format("ShelterMound"))
if mound:
    mb = mound.get_bounding_box()
    msz = mb.max - mb.min
    for i, (bx, by) in enumerate(((5600.0, 700.0), (5600.0, 1150.0))):
        z = ground(bx, by, 0.0) - 25.0
        berm = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(bx, by, z - mb.min.z * 200.0 / msz.z),
                                          unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
        berm.set_actor_label(f"TD_M3_ReactorBerm{i}")
        berm.set_folder_path("Models3")
        berm.static_mesh_component.set_static_mesh(mound)
        berm.set_actor_scale3d(unreal.Vector(320.0 / msz.x, 640.0 / msz.y, 200.0 / msz.z))
        berm.tags = [unreal.Name("M3From_")]
    log.append("reactor berms: 2")
    # The POWER YARD sign stood where the berms are now: lift it above them, on the base side.
    sign = by_label("TD_Base_Label_Power")
    if sign:
        sign.set_actor_location(unreal.Vector(5380.0, 925.0, ground(5380.0, 925.0, 0.0) + 260.0), False, False)
        log.append("power yard sign above the berm")

# Floating 3D text: the module names (B1 COMMAND, LIFE SUPPORT, ISRU PLANT, POWER YARD ...) hung in the air over
# the base. The HUD names what the player looks at, so the world text goes.
labels = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.TextRenderActor)]
for a in labels:
    hide(a)
log.append(f"floating text labels hidden: {len(labels)}")

les.save_current_level()
print("setup_models3:\n" + "\n".join(log))
