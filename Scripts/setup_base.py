# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Grow the Jezero outpost into a connected base: command hub, pressurised corridors, life-support module,
science lab, airlock, ISRU plant, landing zones, logistics yard, power yard with cable trays, relay antenna,
beacon lights and module labels. Design-dependent parts carry DesignShow_<OptionId>_<N> tags; the game shows
them when the flown design has at least N of that option (UTDMissionSubsystem::RefreshDesignScene).

Run after setup_cameras.py, setup_props.py, setup_tripo.py and before setup_fps.py (editor, PIE stopped), or headless:
    UnrealEditor-Cmd.exe TouchdownMars.uproject -run=pythonscript -script=<abs path>/Scripts/setup_base.py -unattended
Rerunnable: removes TD_Base_* first and re-shows rocks it hid before. Units are centimetres; the hub (NASA
Habitat Demonstration Unit) is at the origin, the outpost's front faces -X (player start).
Positions and sizes are [DESIGNER VALUE] layout choices, not NASA measurements.
"""

import math

import unreal

ROOT = "/Game/TouchdownMars"
MAT = f"{ROOT}/Materials"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Outpost")
log = []

# ------------------------------------------------------------------ reset
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith("TD_Base_"):
        actors.destroy_actor(actor)
    elif "HiddenByBase" in [str(t) for t in actor.get_editor_property("tags")]:
        actor.set_actor_hidden_in_game(False)
        actor.set_is_temporarily_hidden_in_editor(False)
        actor.set_actor_enable_collision(True)
        actor.set_editor_property("tags", [t for t in actor.get_editor_property("tags") if str(t) != "HiddenByBase"])


def material(name, color, roughness=None):
    path = f"{MAT}/{name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT, unreal.MaterialInstanceConstant,
                                                                unreal.MaterialInstanceConstantFactoryNew())
    mi = unreal.load_asset(path)
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    return mi


HULL = material("MI_TD_Base_Hull", (0.62, 0.62, 0.60))
RIB = material("MI_TD_Base_Rib", (0.32, 0.33, 0.35))
DARK = material("MI_TD_Base_Dark", (0.05, 0.05, 0.06))
PAD = material("MI_TD_Base_Pad", (0.11, 0.08, 0.065))
TANK = material("MI_TD_Base_Tank", (0.55, 0.58, 0.63))
SOLAR = material("MI_TD_Base_Solar", (0.015, 0.03, 0.09))
ACCENT = material("MI_TD_Base_Accent", (0.80, 0.33, 0.07))
WATER = material("MI_TD_Base_WaterTank", (0.70, 0.72, 0.74))
REGOLITH = material("MI_TD_Base_Regolith", (0.36, 0.17, 0.08))

SHAPES = {name: unreal.load_asset(f"/Engine/BasicShapes/{name}") for name in ("Cube", "Cylinder", "Sphere", "Cone", "Plane")}


def ground(x, y):
    try:
        sample = unreal.LandscapeService.get_height_at_location("TD_Terrain", x, y)
        if sample.valid:
            return sample.height
    except Exception:
        pass
    return 0.0


def spawn(label, mesh, location, rotation=(0, 0, 0), scale=(1, 1, 1), mi=None, folder="Base", tags=(), inspect=None):
    # spawn_actor_from_object returns None in headless commandlet runs; spawn the class and set the mesh instead.
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location),
                                          unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_label(f"TD_Base_{label}")
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if mi:
        actor.static_mesh_component.set_material(0, mi)
    actor.set_folder_path(folder)
    names = [unreal.Name(t) for t in tags]
    if inspect:
        names.append(unreal.Name(inspect))
    if names:
        actor.set_editor_property("tags", names)
    return actor


def box(label, x, y, z, sx, sy, sz, mi, yaw=0, **kw):
    """Cube with its size in centimetres, centre at x, y and bottom at ground + z."""
    return spawn(label, SHAPES["Cube"], (x, y, ground(x, y) + z + sz / 2), (0, yaw, 0), (sx / 100, sy / 100, sz / 100), mi, **kw)


def tube(label, a, b, radius, z, mi=HULL, ribs=True, supports=True, **kw):
    """Horizontal cylinder from a to b (x, y), axis height z above the ground at its middle."""
    (ax, ay), (bx, by) = a, b
    length = math.hypot(bx - ax, by - ay)
    yaw = math.degrees(math.atan2(by - ay, bx - ax))
    mx, my = (ax + bx) / 2, (ay + by) / 2
    gz = ground(mx, my) + z
    parts = [spawn(label, SHAPES["Cylinder"], (mx, my, gz), (90, yaw, 0), (radius / 50, radius / 50, length / 100), mi, **kw)]
    steps = int(length // 300)
    for i in range(1, steps + 1) if ribs else ():
        t = i / (steps + 1)
        px, py = ax + (bx - ax) * t, ay + (by - ay) * t
        parts.append(spawn(f"{label}_Rib{i}", SHAPES["Cylinder"], (px, py, gz), (90, yaw, 0),
                           ((radius + 12) / 50, (radius + 12) / 50, 0.25), RIB, folder=kw.get("folder", "Base")))
        if supports:
            parts.append(box(f"{label}_Leg{i}", px, py, 0, 40, 2 * radius * 0.6, z - radius + 5, RIB, yaw))
    return parts


def label_text(label, text, x, y, z, yaw=180, size=34, color=(0.95, 0.93, 0.88)):
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, ground(x, y) + z), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    actor.set_actor_label(f"TD_Base_Label_{label}")
    comp = actor.text_render
    comp.set_editor_property("text", text)
    comp.set_editor_property("world_size", size)
    comp.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
    comp.set_editor_property("text_render_color", unreal.Color(int(color[2] * 255), int(color[1] * 255), int(color[0] * 255), 255))
    actor.set_folder_path("Base/Labels")
    return actor


def beacon(label, x, y, z=320, color=(1.0, 0.72, 0.42), intensity=30.0, radius=900.0):
    pole = box(f"Beacon_{label}", x, y, 0, 14, 14, z, RIB, folder="Base/Lights")
    # No point light: in daylight the pools of light read as fake; the lamp is an accent-coloured marker.
    spawn(f"BeaconLamp_{label}", SHAPES["Sphere"], (x, y, ground(x, y) + z + 10), scale=(0.2, 0.2, 0.2), mi=ACCENT, folder="Base/Lights")
    return pole


def mesh(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"missing mesh {path}")
    return asset


def sit(actor):
    """Move a mesh actor so its bounds' bottom sits on the terrain."""
    origin, extent = actor.get_actor_bounds(False)
    loc = actor.get_actor_location()
    lift = ground(loc.x, loc.y) - (origin.z - extent.z)
    actor.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + lift), False, False)
    return actor


HDU_B = mesh(f"{ROOT}/NASA/HabitatA/Habitat_Demonstration_Unit_part_1/StaticMeshes/Habitat_Demonstration_Unit_part_1")  # same module as the hub
FISSION = mesh(f"{ROOT}/Tripo/FissionPower/TD_Tripo_FissionPower/StaticMeshes/TD_Tripo_FissionPower")
DOME = mesh(f"{ROOT}/Tripo/Greenhouse/TD_Tripo_Greenhouse/StaticMeshes/TD_Tripo_Greenhouse")
CRATES = mesh(f"{ROOT}/Tripo/CargoCrates/TD_Tripo_CargoCrates/StaticMeshes/TD_Tripo_CargoCrates")
HATCH = mesh(f"{ROOT}/Tripo/ShelterHatch/TD_Tripo_ShelterHatch/StaticMeshes/TD_Tripo_ShelterHatch")

# The habitat module walks with complex collision (as the hub).
body = HDU_B.get_editor_property("body_setup")
if body:
    body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    unreal.EditorAssetLibrary.save_loaded_asset(HDU_B, only_if_is_dirty=False)

footprint = []  # (x, y, r) circles that must be free of rocks


def keep_clear(x, y, r):
    footprint.append((x, y, r))


# ------------------------------------------------------------------ B1 command hub (existing habitat at 0, 0)
keep_clear(0, 0, 520)
label_text("Hub", "B1  COMMAND", -470, 0, 250, size=28)
# Pressurised corridors from the hub to the greenhouse (-Y) and the shelter (+Y).
tube("Corridor_Greenhouse", (40, -360), (90, -800), 100, 140, inspect="Inspect_Corridor")
tube("Corridor_Shelter", (40, 360), (110, 820), 100, 140, inspect="Inspect_Corridor")

# ------------------------------------------------------------------ B2 life support (ECLSS) and B3 science lab
HDU_SCALE = (0.022, 0.022, 0.022)  # the NASA Habitat Demonstration Unit mesh is authored about 40x too large (hub uses 0.025)
eclss = sit(spawn("ECLSS", HDU_B, (1650, 250, 0), (0, 90, 0), HDU_SCALE, inspect="Inspect_ECLSS"))
keep_clear(1650, 250, 560)
label_text("ECLSS", "B2  LIFE SUPPORT", 1650 - 450, 250, 330)
tube("Corridor_ECLSS", (380, 40), (1500, 220), 100, 140, inspect="Inspect_Corridor")

lab = sit(spawn("Lab", HDU_B, (1750, 1750, 0), (0, 0, 0), HDU_SCALE, inspect="Inspect_Lab"))
keep_clear(1750, 1750, 560)
label_text("Lab", "B3  SCIENCE LAB", 1750 - 450, 1750, 330)
tube("Corridor_Lab", (1680, 500), (1740, 1450), 100, 140, inspect="Inspect_Corridor")

# ------------------------------------------------------------------ B4 airlock and suitports by the rover
airlock_parts = tube("Airlock", (1700, -1050), (2350, -1050), 190, 230, mi=HULL, inspect="Inspect_Airlock")
for end_x in (1700, 2350):
    spawn(f"AirlockCap_{end_x}", SHAPES["Sphere"], (end_x, -1050, ground(end_x, -1050) + 230), scale=(1.2, 3.8, 3.8), mi=HULL,
          inspect="Inspect_Airlock")
keep_clear(2025, -1050, 420)
tube("Corridor_Airlock", (1700, 0), (1760, -900), 100, 140, inspect="Inspect_Corridor")
spawn("AirlockHatch", HATCH, (2025, -1250, ground(2025, -1250)), (0, 90, 0), (0.8, 0.8, 0.8), inspect="Inspect_Airlock")
label_text("Airlock", "B4  AIRLOCK", 2025, -1300, 470, yaw=-90)

# ------------------------------------------------------------------ ISRU plant (oxygen and water from Mars)
box("ISRU_Plant", 3300, 950, 0, 520, 360, 300, HULL, inspect="Inspect_ISRU")
for i, (tx, ty) in enumerate(((3050, 1300), (3550, 1300))):
    tube(f"ISRU_Tank{i}", (tx - 220, ty), (tx + 220, ty), 110, 150, mi=TANK, ribs=False, inspect="Inspect_ISRU")
keep_clear(3300, 1050, 520)
label_text("ISRU", "ISRU PLANT", 3300 - 270, 950, 360)
# Pipe rack from the ISRU plant to life support.
tube("Pipe_ISRU_ECLSS", (3040, 900), (2190, 330), 18, 120, mi=TANK, ribs=True, supports=True, inspect="Inspect_Pipe")
box("MOXIE_Unit", 3600, 650, 0, 120, 90, 90, ACCENT, tags=("DesignShow_Moxie_1",), inspect="Inspect_MOXIE")

# ------------------------------------------------------------------ landing zones and logistics
spawn("LZ1_Pad", SHAPES["Cylinder"], (4000, -1800, ground(4000, -1800) - 2), scale=(18, 18, 0.06), mi=PAD, inspect="Inspect_LandingPad")
label_text("LZ1", "LZ-1  ASCENT", 4000 - 950, -1800, 40, size=90)
spawn("LZ2_Pad", SHAPES["Cylinder"], (5600, -2600, ground(5600, -2600) - 2), scale=(18, 18, 0.06), mi=PAD, inspect="Inspect_LandingPad")
label_text("LZ2", "LZ-2  CARGO", 5600 - 950, -2600, 40, size=90)
keep_clear(4000, -1800, 950)
keep_clear(5600, -2600, 950)
for i, (cx, cy, yaw) in enumerate(((5300, -2200, 10), (5450, -2200, 0), (5800, -2950, 40), (3000, -1350, 20))):
    sit(spawn(f"Logistics_Crates{i}", CRATES, (cx, cy, 0), (0, yaw, 0), (1.2, 1.2, 1.2), inspect="Inspect_Cargo"))
# Spare parts: one stack for the kit, a second for full spares.
sit(spawn("Spares_A", CRATES, (2600, -1400, 0), (0, 15, 0), (1.0, 1.0, 1.0),
          tags=("DesignShow_SparesKit_1", "DesignShow_SparesFull_1"), inspect="Inspect_Spares"))
sit(spawn("Spares_B", CRATES, (2780, -1450, 0), (0, -20, 0), (1.0, 1.0, 1.0), tags=("DesignShow_SparesFull_1",), inspect="Inspect_Spares"))
# Graded road from the airlock to the landing zones.
for i, ((ax, ay), (bx, by)) in enumerate((((2400, -1250), (3300, -1650)), ((3300, -1650), (5200, -2450)))):
    box(f"Road{i}", (ax + bx) / 2, (ay + by) / 2, -3, math.hypot(bx - ax, by - ay), 380, 6, PAD,
        math.degrees(math.atan2(by - ay, bx - ax)), inspect="Inspect_Road")

# ------------------------------------------------------------------ power yard: fission units by design count
keep_clear(6200, 900, 650)
label_text("Power", "POWER YARD", 6200 - 650, 900, 60, size=90)
fission_slots = ((6000, 700), (6000, 1150), (6450, 700), (6450, 1150))
existing = [a for a in actors.get_all_level_actors() if a.get_actor_label() == "TD_Tripo_FissionPower"]
if existing:
    tags = [t for t in existing[0].get_editor_property("tags") if not str(t).startswith("DesignShow_")]
    existing[0].set_editor_property("tags", tags + [unreal.Name("DesignShow_Kilopower_1")])
for n, (fx, fy) in enumerate(fission_slots[1:], start=2):
    src = existing[0] if existing else None
    actor = spawn(f"Fission{n}", FISSION, (fx, fy, 0), (0, 0, 0), tuple(src.get_actor_scale3d().to_tuple()) if src else (1, 1, 1),
                  tags=(f"DesignShow_Kilopower_{n}",), inspect="Inspect_Fission")
    if src:
        mats = src.static_mesh_component.get_materials()
        for i, m in enumerate(mats):
            actor.static_mesh_component.set_material(i, m)
    sit(actor)
# Cable trays from the power yard to life support (low, on the ground).
cable = [(5850, 860), (4700, 1050), (3700, 700), (2200, 120)]
for i in range(len(cable) - 1):
    (ax, ay), (bx, by) = cable[i], cable[i + 1]
    length = math.hypot(bx - ax, by - ay)
    yaw = math.degrees(math.atan2(by - ay, bx - ax))
    box(f"CableTray{i}", (ax + bx) / 2, (ay + by) / 2, 0, length, 40, 14, DARK, yaw, inspect="Inspect_CableTray")

# ------------------------------------------------------------------ solar fields (by design count)
field_origin = ((2900, 2600), (3900, 2600), (4900, 2600))
for f, (ox, oy) in enumerate(field_origin, start=1):
    for row in range(4):
        for col in range(2):
            px, py = ox + col * 420, oy + row * 260
            spawn(f"Solar{f}_{row}_{col}", SHAPES["Cube"], (px, py, ground(px, py) + 120), (0, 0, 28), (3.8, 1.8, 0.04), SOLAR,
                  folder="Base/Solar", tags=(f"DesignShow_SolarField_{f}",), inspect="Inspect_SolarField")
            box(f"SolarLeg{f}_{row}_{col}", px, py, 0, 12, 12, 110, RIB, folder="Base/Solar", tags=(f"DesignShow_SolarField_{f}",))
    keep_clear(ox + 210, oy + 390, 650)

# ------------------------------------------------------------------ relay UHF antenna (relay orbiter picked)
box("RelayMast", 900, 1450, 0, 24, 24, 650, RIB, tags=("DesignShow_RelayOrbiter_1",), inspect="Inspect_RelayAntenna")
spawn("RelayDish", SHAPES["Cone"], (900, 1450, ground(900, 1450) + 680), (-50, 200, 0), (0.9, 0.9, 0.35), TANK,
      tags=("DesignShow_RelayOrbiter_1",), inspect="Inspect_RelayAntenna")
keep_clear(900, 1450, 150)

# ------------------------------------------------------------------ MEDA weather mast (instrument picked)
box("MEDA_Mast", 650, -600, 0, 10, 10, 260, RIB, tags=("DesignShow_Meda_1",), inspect="Inspect_MEDA")
box("MEDA_Boom", 650, -600, 240, 90, 6, 6, RIB, tags=("DesignShow_Meda_1",), inspect="Inspect_MEDA")

# ------------------------------------------------------------------ shelter options and second greenhouse
for i in range(6):
    a = math.radians(-60 + i * 24)
    tx, ty = 150 + 470 * math.cos(a), 1100 + 470 * math.sin(a)
    # Upright water tanks (2.4 m tall, 1.2 m across) with a dark band, ringing the sleeping side of the shelter.
    spawn(f"WaterWall{i}", SHAPES["Cylinder"], (tx, ty, ground(tx, ty) + 120), scale=(1.2, 1.2, 2.4), mi=WATER,
          tags=("DesignShow_ShelterWaterWall_1",), inspect="Inspect_WaterWall")
    spawn(f"WaterWallBand{i}", SHAPES["Cylinder"], (tx, ty, ground(tx, ty) + 170), scale=(1.24, 1.24, 0.12), mi=RIB,
          tags=("DesignShow_ShelterWaterWall_1",), inspect="Inspect_WaterWall")
spawn("RegolithBerm", SHAPES["Sphere"], (250, 1100, ground(250, 1100) - 120), scale=(8.6, 8.6, 5.2), mi=REGOLITH,
      tags=("DesignShow_ShelterRegolith_1",), inspect="Inspect_Shelter")
dome_src = [a for a in actors.get_all_level_actors() if a.get_actor_label() == "TD_Tripo_Greenhouse"]
gh2 = spawn("Greenhouse2", DOME, (100, -2300, 0), (0, 30, 0),
            tuple(dome_src[0].get_actor_scale3d().to_tuple()) if dome_src else (1, 1, 1),
            tags=("DesignShow_GreenhouseLarge_1",), inspect="Inspect_Greenhouse")
if dome_src:
    for i, m in enumerate(dome_src[0].static_mesh_component.get_materials()):
        gh2.static_mesh_component.set_material(i, m)
sit(gh2)
tube("Corridor_Greenhouse2", (100, -1500), (100, -1950), 100, 140, tags=("DesignShow_GreenhouseLarge_1",), inspect="Inspect_Corridor")
keep_clear(100, -2300, 500)

# ------------------------------------------------------------------ beacons along the base
for name, (bx, by) in {"HubW": (-650, -650), "HubE": (-650, 650), "Airlock": (2450, -1350), "LZ1a": (3100, -2500),
                       "LZ1b": (4900, -1100), "LZ2": (6500, -2600), "PowerA": (5600, 450), "PowerB": (6800, 1400),
                       "ISRU": (3900, 1150), "Lab": (2350, 2200)}.items():
    beacon(name, bx, by)

# ------------------------------------------------------------------ keep rocks out of the base
hidden = 0
for actor in actors.get_all_level_actors():
    if not actor.get_actor_label().startswith("TD_Rock_"):
        continue
    loc = actor.get_actor_location()
    _, ext = actor.get_actor_bounds(False)
    r = max(ext.x, ext.y)
    if any(math.hypot(loc.x - x, loc.y - y) < cr + r for x, y, cr in footprint):
        actor.set_actor_hidden_in_game(True)
        actor.set_is_temporarily_hidden_in_editor(True)
        actor.set_actor_enable_collision(False)
        actor.set_editor_property("tags", list(actor.get_editor_property("tags")) + [unreal.Name("HiddenByBase")])
        hidden += 1

count = len([a for a in actors.get_all_level_actors() if a.get_actor_label().startswith("TD_Base_")])
log.append(f"base actors: {count}, rocks hidden: {hidden}")
les.save_current_level()
print("\n".join(log))
unreal.log("[setup_base] " + " ".join(log))
