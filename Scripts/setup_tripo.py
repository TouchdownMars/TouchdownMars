# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Dress L_TD_Outpost with the Tripo AI-generated models (imported by import_tripo_assets.py, Nanite on):
ascent vehicle, fission power unit, greenhouse dome, plant rack, cargo crates, shelter hatch, and the rigged
astronaut for the three crew members (animations: idle, look around, walk, lift, greet, wave goodbye).
The greybox stand-ins stay in the level, hidden, so the game logic (lights, energy lines, stage points) is unchanged.
Run after setup_cameras.py and setup_props.py (editor, PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/setup_tripo.py", encoding="utf-8").read())
"""

import unreal

ROOT = "/Game/TouchdownMars"
TRIPO = f"{ROOT}/Tripo"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Outpost")
log = []

for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith("TD_Tripo_"):
        actors.destroy_actor(actor)


def find(label):
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    raise RuntimeError(f"{label} not found")


def hide(label):
    actor = find(label)
    actor.set_actor_hidden_in_game(True)
    actor.set_is_temporarily_hidden_in_editor(True)
    actor.set_actor_enable_collision(False)
    return actor


def ground(x, y):
    sample = unreal.LandscapeService.get_height_at_location("TD_Terrain", x, y)
    return sample.height if sample.valid else 0.0


def mesh(name):
    for path in unreal.EditorAssetLibrary.list_assets(f"{TRIPO}/{name}", recursive=True):
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            return asset
    raise RuntimeError(f"no Tripo mesh for {name}")


def place(label, static_mesh, x, y, yaw, height_cm):
    """Spawn, scale to the wanted height, sit on the terrain."""
    actor = actors.spawn_actor_from_object(static_mesh, unreal.Vector(x, y, 0), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    actor.set_actor_label(label)
    actor.set_folder_path("Tripo")
    box = static_mesh.get_bounding_box()
    scale = height_cm / max(1.0, box.max.z - box.min.z)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    origin, extent = actor.get_actor_bounds(False)
    here = actor.get_actor_location()
    actor.set_actor_location(unreal.Vector(here.x, here.y, here.z + ground(x, y) - (origin.z - extent.z)), False, False)
    log.append(f"{label}: {extent.x * 2 / 100:.1f} x {extent.y * 2 / 100:.1f} x {extent.z * 2 / 100:.1f} m at ({x}, {y})")
    return actor


# Ascent vehicle instead of the greybox cone and legs (S02 background, S07 departure).
hide("TD_Lander")
hide("TD_LanderLegs")
place("TD_Tripo_AscentVehicle", mesh("AscentVehicle"), 4000, -1800, 0, 900)

# Fission power unit: hide the greybox cylinder but keep the actor (its energy lines follow the cells).
unit = find("TD_FissionUnit")
unit.get_component_by_class(unreal.StaticMeshComponent).set_editor_property("visible", False)
here = unit.get_actor_location()
place("TD_Tripo_FissionPower", mesh("FissionPower"), here.x, here.y, 0, 350)

# Greenhouse dome (6.5 m wide) instead of the white sphere.
hide("TD_Art_GreenhouseDome")
dome_mesh = mesh("Greenhouse")
box = dome_mesh.get_bounding_box()
place("TD_Tripo_Greenhouse", dome_mesh, 100, -1150, 210, 650 * (box.max.z - box.min.z) / max(1.0, box.max.x - box.min.x))

# Cargo crates by the habitat, shelter hatch at the shelter door.
hide("TD_CargoA")
hide("TD_CargoB")
place("TD_Tripo_CargoCrates", mesh("CargoCrates"), -300, -440, 200, 150)
hide("TD_Art_ShelterDoor")
place("TD_Tripo_ShelterHatch", mesh("ShelterHatch"), -215, 1100, 90, 220)

# Plant rack: the logic actor keeps its stress tint; the Tripo rack replaces the placeholder shelves.
rack = find("TD_PlantRack")
rack.set_editor_property("rack_mesh", mesh("PlantRack"))
rack.set_editor_property("rack_height", 190.0)

# Crew: rigged Tripo astronaut with animations, tinted by system color in game.
anims = {}
skel = None
for path in unreal.EditorAssetLibrary.list_assets(f"{TRIPO}/Astronaut", recursive=True):
    asset = unreal.load_asset(path)
    if isinstance(asset, unreal.SkeletalMesh):
        skel = asset
    elif isinstance(asset, unreal.AnimSequence):
        for key in ("standing_relax", "look_around", "walk", "lift_heavy", "greet_01", "wave_goodbye_01"):
            if asset.get_name().endswith(key):
                anims[key] = asset
def srgb(hex_value):
    """design.md system color (sRGB hex) to linear, mixed half with white for a light suit tint."""
    channels = [int(hex_value[i:i + 2], 16) / 255.0 for i in (0, 2, 4)]
    linear = [((c + 0.055) / 1.055) ** 2.4 if c > 0.04045 else c / 12.92 for c in channels]
    return unreal.LinearColor(*[0.5 + 0.5 * c for c in linear], 1.0)


def suit_material(name, hex_value):
    """Plain suit material in the system color: keeps the crew color coding and drops any printed patch."""
    path = f"{ROOT}/Materials/{name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, f"{ROOT}/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi = unreal.load_asset(path)
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", srgb(hex_value))
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    return mi


SUITS = {"TD_Crew_FlightEngineer": suit_material("MI_TD_Suit_Air", "3A8DDE"),
         "TD_Crew_ScienceOfficer": suit_material("MI_TD_Suit_Greenhouse", "3FB27F"),
         "TD_Crew_HealthOfficer": suit_material("MI_TD_Suit_Shelter", "F07F2E")}
for label in ("TD_Crew_FlightEngineer", "TD_Crew_ScienceOfficer", "TD_Crew_HealthOfficer"):
    crew = find(label)
    crew.set_editor_property("suit_mesh", skel)
    crew.set_editor_property("suit_material", SUITS[label])
    crew.set_editor_property("suit_height", 180.0)
    crew.set_editor_property("idle_anim", anims.get("standing_relax"))
    crew.set_editor_property("worried_anim", anims.get("look_around"))
    crew.set_editor_property("walk_anim", anims.get("walk"))
    crew.set_editor_property("work_anim", anims.get("lift_heavy"))
    crew.set_editor_property("celebrate_anim", anims.get("greet_01"))
    crew.set_editor_property("farewell_anim", anims.get("wave_goodbye_01"))
log.append(f"crew suit {skel.get_name() if skel else None}, animations {sorted(anims)}")

les.save_current_level()
print("\n".join(log))
