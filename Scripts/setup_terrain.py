# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Phase 11: Mars Trek Jezero terrain as the L_TD_Outpost landscape (run inside the editor, PIE stopped).
    exec(open(r"<ProjectDir>/Scripts/setup_terrain.py", encoding="utf-8").read())
Needs Data/Terrain/TD_Jezero_1009.png + .json from Scripts/dem_to_heightmap.py and the level from setup_outpost.py.
Creates the landscape TD_Terrain (VibeUE LandscapeService), removes the flat greybox ground and puts the scene
props on the terrain surface.
"""

import json
import os

import unreal

ROOT = "/Game/TouchdownMars"
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
TERRAIN = os.path.join(PROJECT, "Data", "Terrain")
LS = unreal.LandscapeService
LABEL = "TD_Terrain"
log = []

info = json.load(open(os.path.join(TERRAIN, "TD_Jezero_1009.json"), encoding="utf-8"))
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Outpost")

for actor in actors.get_all_level_actors():
    if actor.get_actor_label() in (LABEL, "TD_Ground"):
        actors.destroy_actor(actor)

loc, scale = info["ue_location"], info["ue_scale"]
result = LS.create_landscape(unreal.Vector(*loc), unreal.Rotator(0, 0, 0), unreal.Vector(*scale),
                             sections_per_component=2, quads_per_section=63, component_count_x=8, component_count_y=8,
                             landscape_label=LABEL)
log.append(f"create: {result.success} {result.error_message}")
imported = LS.import_heightmap(LABEL, os.path.join(TERRAIN, "TD_Jezero_1009.png"))
log.append(f"import: {imported.success} {imported.resolution} {imported.error_message}")
LS.set_landscape_material(LABEL, f"{ROOT}/Materials/MI_TD_Greybox_Ground")

# Sit every free-standing prop on the terrain (the base pad is level at Z = 0 by construction).
GROUNDED = {"TD_Lander": 400, "TD_LanderLegs": 100, "TD_FissionUnit": 150, "TD_Rover": 60}
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label in GROUNDED:
        here = actor.get_actor_location()
        sample = LS.get_height_at_location(LABEL, here.x, here.y)
        if sample.valid:
            actor.set_actor_location(unreal.Vector(here.x, here.y, sample.height + GROUNDED[label]), False, False)
            log.append(f"{label}: ground {sample.height:.0f} cm")

centre = LS.get_height_at_location(LABEL, 1500, -500)
log.append(f"pad height at base centre: {centre.height:.1f} cm (expected ~0)")
log.append(f"relief in window: {info['relief_m']:.1f} m, z scale {scale[2]:.2f}")
les.save_current_level()
print("\n".join(log))
