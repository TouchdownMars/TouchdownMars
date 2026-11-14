# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Pebbles and small rocks on the Jezero crater floor around the outpost, as instanced foliage.
Meshes: free Quixel Megascans from Fab (Stones and Pebbles Pack, Desert Western Pile Rock Small 01), imported by
VibeUE FabService into /Game/Fab/Free (git-ignored; rows in sources.csv). Foliage types live there too.

Density follows the shape of the Golombek rock model (cumulative rock area falls off fast with size; Golombek
et al. 2021), with k = 0.07 near the base (research_notes/Mars gokyuzu ve zemin/references.md); the counts are
designer values capped for frame rate. Seeded, so the layout is the same every run. The base footprint, the
pads and the paths stay clear. Run inside the editor after setup_far_terrain.py (PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/scatter_pebbles.py", encoding="utf-8").read())
"""

import math
import random

import unreal

FS = unreal.FoliageService
EAL = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if les.get_current_level().get_outer().get_name() != "L_TD_Outpost":
    les.load_level("/Game/TouchdownMars/Maps/L_TD_Outpost")

PACK = "/Game/Fab/Free/Stones_and_Pebbles_Pack/ukhkbediw_tier_1/StaticMeshes"
PILE = "/Game/Fab/Free/Desert_Western_Pile_Rock_Small_01/ul4lehiga_tier_1/StaticMeshes/ul4lehiga_tier_1"
TYPES = "/Game/Fab/Free/TD_Foliage"

# Same keep-clear zones as scatter_rocks.py (cm): base footprint and paths, lander, fission unit, console.
CLEAR_RECT = (-900.0, 900.0, -1700.0, 1700.0)
CLEAR_CIRCLES = [((4000.0, -1800.0), 700.0), ((6000.0, 700.0), 500.0), ((-480.0, 0.0), 600.0), ((-1180.0, -560.0), 250.0)]


def is_clear(x, y):
    if CLEAR_RECT[0] < x < CLEAR_RECT[1] and CLEAR_RECT[2] < y < CLEAR_RECT[3]:
        return False
    return all(math.hypot(x - cx, y - cy) > r for (cx, cy), r in CLEAR_CIRCLES)


pebbles = [p for p in EAL.list_assets(PACK, recursive=False) if isinstance(unreal.load_asset(p), unreal.StaticMesh)]
if not pebbles or not EAL.does_asset_exist(PILE):
    raise RuntimeError("import the Fab pebble and rock packs first")


def size_cm(path):
    box = unreal.load_asset(path).get_bounding_box()
    d = box.max - box.min
    return max(d.x, d.y)


sizes = {p.split(".")[0]: size_cm(p) for p in pebbles}
print("pack sizes (cm):", {k.split("/")[-1]: round(v) for k, v in sizes.items()})


def foliage_type(mesh, name, smin, smax, cull):
    path = f"{TYPES}/{name}"
    if not EAL.does_asset_exist(path):
        FS.create_foliage_type(mesh, TYPES, name, smin, smax, True, 30.0, 35.0, cull)
    return path


rng = random.Random(15112026)
report = []
# Pebbles: 1 to 12 cm across, dense near the base where the player looks down, gone by 60 m.
for i, (mesh, size) in enumerate(sorted(sizes.items())):
    target = rng.uniform(4.0, 12.0)
    scale = max(0.05, target / max(size, 1.0))
    ft = foliage_type(mesh, f"FT_TD_Pebble_{i:02d}", scale * 0.5, scale * 1.2, 6000.0)
    FS.remove_all_foliage_of_type(ft)
    points = []
    while len(points) < 450:
        r = 1200.0 + 9000.0 * math.sqrt(rng.random())   # area-uniform between 12 and 102 m from the centre
        a = rng.uniform(0.0, 2.0 * math.pi)
        x, y = r * math.cos(a), r * math.sin(a)
        if is_clear(x, y):
            points.append(unreal.Vector(x, y, 5000.0))
    res = FS.add_foliage_instances(ft, points, 1.0, 1.0, True, True, True)
    report.append(f"pebble {i}: {res.instances_added} added, {res.instances_rejected} rejected")

# Small rocks, 20 to 60 cm: fewer, out to 250 m (most raised rocks at Jezero are under 30 cm, Bell et al. 2022).
rock_size = size_cm(PILE)
rock_scale = 40.0 / max(rock_size, 1.0)
ft = foliage_type(PILE, "FT_TD_SmallRock", rock_scale * 0.5, rock_scale * 1.5, 25000.0)
FS.remove_all_foliage_of_type(ft)
points = []
while len(points) < 260:
    r = 1500.0 + 23500.0 * math.sqrt(rng.random())
    a = rng.uniform(0.0, 2.0 * math.pi)
    x, y = r * math.cos(a), r * math.sin(a)
    if is_clear(x, y):
        points.append(unreal.Vector(x, y, 5000.0))
res = FS.add_foliage_instances(ft, points, 1.0, 1.0, True, True, True)
report.append(f"small rocks: {res.instances_added} added, {res.instances_rejected} rejected (mesh {round(rock_size)} cm)")

les.save_current_level()
print("\n".join(report))
