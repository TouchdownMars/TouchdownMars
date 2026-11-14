# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Phase 11: scatter the free Megascans rocks (Fab, imported into /Game/Fab/Free, git-ignored) around the outpost.
Seeded, so the layout is the same every run. The base pad (radius 35 m around the base centre) stays clear and
every rock sits on the Mars Trek terrain. Run inside the editor after setup_terrain.py (PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/scatter_rocks.py", encoding="utf-8").read())
"""

import math
import random

import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/TouchdownMars/Maps/L_TD_Outpost")
for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith("TD_Rock_"):
        actors.destroy_actor(actor)

meshes = [unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets("/Game/Fab/Free", recursive=True)
          if isinstance(unreal.load_asset(p), unreal.StaticMesh)]
rng = random.Random(14112026)
# Keep clear: the base footprint and paths, the lander and the fission unit (cm).
CLEAR_RECT = (-900.0, 900.0, -1700.0, 1700.0)          # x min, x max, y min, y max
CLEAR_CIRCLES = [((4000.0, -1800.0), 700.0), ((6000.0, 700.0), 500.0), ((-480.0, 0.0), 600.0)]


def is_clear(x, y):
    if CLEAR_RECT[0] < x < CLEAR_RECT[1] and CLEAR_RECT[2] < y < CLEAR_RECT[3]:
        return False
    return all(math.hypot(x - cx, y - cy) > r for (cx, cy), r in CLEAR_CIRCLES)


placed = 0
attempts = 0
while placed < 90 and attempts < 4000:
    attempts += 1
    x = rng.uniform(-2500, 9000)
    y = rng.uniform(-5000, 4500)
    if not is_clear(x, y):
        continue
    sample = unreal.LandscapeService.get_height_at_location("TD_Terrain", x, y)
    if not sample.valid:
        continue
    mesh = rng.choice(meshes)
    distance = math.hypot(x, y)
    scale = rng.uniform(0.35, 1.0) if distance < 3000 else rng.uniform(0.8, 2.4)
    actor = actors.spawn_actor_from_object(mesh, unreal.Vector(x, y, sample.height - 8 * scale),
                                           unreal.Rotator(pitch=rng.uniform(-6, 6), yaw=rng.uniform(0, 360), roll=rng.uniform(-6, 6)))
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    actor.set_actor_label(f"TD_Rock_{placed:02d}")
    actor.set_folder_path("Rocks")
    placed += 1

les.save_current_level()
print(f"meshes: {[m.get_name() for m in meshes]}\nplaced {placed} rocks in {attempts} tries")
