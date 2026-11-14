# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Far terrain TD_TerrainFar around L_TD_Outpost (run inside the editor, PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/setup_far_terrain.py", encoding="utf-8").read())
Needs Data/Terrain/TD_Jezero_Far_1009.png + .json from Scripts/make_far_terrain.py. Uses the near terrain's
material, has no collision (nobody walks there) and casts no dynamic shadow of its own beyond the far field.
"""

import json
import os

import unreal

ROOT = "/Game/TouchdownMars"
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
TERRAIN = os.path.join(PROJECT, "Data", "Terrain")
LS = unreal.LandscapeService
LABEL = "TD_TerrainFar"

info = json.load(open(os.path.join(TERRAIN, "TD_Jezero_Far_1009.json"), encoding="utf-8"))
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if les.get_current_level().get_outer().get_name() != "L_TD_Outpost":
    les.load_level(f"{ROOT}/Maps/L_TD_Outpost")

near = None
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() == LABEL:
        actors.destroy_actor(actor)
    elif actor.get_actor_label() == "TD_Terrain":
        near = actor

loc, scale = info["ue_location"], info["ue_scale"]
result = LS.create_landscape(unreal.Vector(*loc), unreal.Rotator(0, 0, 0), unreal.Vector(*scale),
                             sections_per_component=2, quads_per_section=63, component_count_x=8, component_count_y=8,
                             landscape_label=LABEL)
print("create:", result.success, result.error_message)
imported = LS.import_heightmap(LABEL, os.path.join(TERRAIN, "TD_Jezero_Far_1009.png"))
print("import:", imported.success, imported.resolution, imported.error_message)

far = next(a for a in actors.get_all_level_actors() if a.get_actor_label() == LABEL)
if near:
    far.set_editor_property("landscape_material", near.get_editor_property("landscape_material"))
LS.set_landscape_collision(LABEL, False)
for edge in ((5000.0, 0.0), (0.0, -5000.0), (-5000.0, 3000.0)):
    sample = LS.get_height_at_location(LABEL, edge[0], edge[1])
    print("far height at", edge, round(sample.height), "cm" if sample.valid else "invalid")
les.save_current_level()
print("far terrain ok")
