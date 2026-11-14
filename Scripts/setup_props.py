# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Replace greybox props in L_TD_Outpost with NASA 3D Resources models (imported by import_nasa_props.py,
textures stripped, plain team material). Run after setup_cameras.py (editor, PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/setup_props.py", encoding="utf-8").read())
Rerunnable: removes TD_Prop_* first.
"""

import unreal

ROOT = "/Game/TouchdownMars"
NASA = f"{ROOT}/NASA"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Outpost")
log = []

for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith("TD_Prop_"):
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


def ground(x, y):
    sample = unreal.LandscapeService.get_height_at_location("TD_Terrain", x, y)
    return sample.height if sample.valid else 0.0


def first_mesh(folder):
    for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            return asset
    raise RuntimeError(f"no static mesh in {folder}")


def place(label, meshes, x, y, yaw, scale, material=None):
    """Spawn one actor per mesh at the same transform, then sit the group on the terrain."""
    group = []
    for index, mesh in enumerate(meshes):
        actor = actors.spawn_actor_from_object(mesh, unreal.Vector(x, y, 0), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
        actor.set_actor_label(f"{label}" if len(meshes) == 1 else f"{label}_{index}")
        actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
        actor.set_folder_path("Props")
        if material:
            actor.static_mesh_component.set_material(0, material)
        group.append(actor)
    bottom = min(a.get_actor_bounds(False)[0].z - a.get_actor_bounds(False)[1].z for a in group)
    top = max(a.get_actor_bounds(False)[0].z + a.get_actor_bounds(False)[1].z for a in group)
    lift = ground(x, y) - bottom
    for actor in group:
        here = actor.get_actor_location()
        actor.set_actor_location(unreal.Vector(here.x, here.y, here.z + lift), False, False)
    log.append(f"{label}: {len(group)} part(s), height {(top - bottom) / 100:.1f} m at ({x}, {y})")
    return group


# Pressurized rover (NASA Space Exploration Vehicle) instead of the greybox rover block.
hide("TD_Rover")
place("TD_Prop_SEV", [first_mesh(f"{NASA}/SEV")], 1150, -650, 125, 1.0)

# RASSOR excavation robot by the cargo lander (the PowerHookup card's "let the robot try again").
place("TD_Prop_RASSOR", [first_mesh(f"{NASA}/RASSOR")], 3420, -1620, 150, 1.0)

# Communication dish: NASA Tall Dish scaled to a 4 m base antenna, on the spot of the greybox mast.
dish_parts = [unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets(f"{NASA}/TallDish", recursive=True)
              if isinstance(unreal.load_asset(p), unreal.StaticMesh)]
hide("TD_Antenna")
hide("TD_AntennaDish")
dish = place("TD_Prop_Dish", dish_parts, 380, 520, 200, 0.0026)

les.save_current_level()
print("\n".join(log))
