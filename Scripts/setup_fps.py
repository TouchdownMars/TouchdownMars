# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""First-person setup for L_TD_Outpost: player start, collision on the NASA and Tripo meshes (complex as simple),
and Inspect_* tags so the helmet names what the astronaut looks at (text.csv keys).
Run after setup_tripo.py (editor, PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/setup_fps.py", encoding="utf-8").read())
"""

import unreal

ROOT = "/Game/TouchdownMars"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Outpost")
log = []

# Collision: walkable base, no walking through habitats, vehicles or crates.
count = 0
for folder in (f"{ROOT}/NASA", f"{ROOT}/Tripo"):
    for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            body = asset.get_editor_property("body_setup")
            if body:
                body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
                unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
                count += 1
log.append(f"collision: complex-as-simple on {count} meshes")

TAGS = {
    "TD_Art_Habitat": "Inspect_Habitat", "TD_Tripo_Greenhouse": "Inspect_Greenhouse",
    "TD_Art_ShelterMound": "Inspect_Shelter", "TD_Tripo_ShelterHatch": "Inspect_Shelter",
    "TD_Tripo_FissionPower": "Inspect_Fission", "TD_Tripo_AscentVehicle": "Inspect_Ascent",
    "TD_Prop_SEV": "Inspect_Rover", "TD_Prop_RASSOR": "Inspect_Robot", "TD_Tripo_CargoCrates": "Inspect_Cargo",
}
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    tag = TAGS.get(label) or ("Inspect_Dish" if label.startswith("TD_Prop_Dish") else None)
    if tag:
        tags = [t for t in actor.get_editor_property("tags") if not str(t).startswith("Inspect_")]
        actor.set_editor_property("tags", tags + [unreal.Name(tag)])
        log.append(f"{label}: {tag}")
    if label == "TD_PlayerStart":
        actors.destroy_actor(actor)

# Player start: on the pad in front of the base, looking at the habitat.
x, y = -950.0, -150.0
sample = unreal.LandscapeService.get_height_at_location("TD_Terrain", x, y)
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(x, y, (sample.height if sample.valid else 0.0) + 100.0),
                                      unreal.Rotator(pitch=0, yaw=5, roll=0))
start.set_actor_label("TD_PlayerStart")
log.append("player start at (-950, -150)")

les.save_current_level()
print("\n".join(log))
