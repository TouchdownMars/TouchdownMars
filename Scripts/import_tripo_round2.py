# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Import the round-2 Tripo AI models (D:/NASA/_kaynaklar/tripo/round2, not in the repo; generated 15 Nov 2026 on
the team's Tripo Studio account, HD v3.1, Ultra off, 2K PBR, de-lit) into /Game/TouchdownMars/Tripo/<Name>.
Each mesh gets Nanite and is reported with its size so setup_models2.py can scale it. AI-generated: every model
has a sources.csv row with bIsAIGenerated=True. Run in the editor or headless:
    exec(open(r"<ProjectDir>/Scripts/import_tripo_round2.py", encoding="utf-8").read())
"""

import glob
import os

import unreal

SOURCE = r"D:/NASA/_kaynaklar/tripo/round2"
DEST = "/Game/TouchdownMars/Tripo"
ONLY = globals().get("ONLY")   # optional list of names to (re)import
log = []

for glb in sorted(glob.glob(os.path.join(SOURCE, "TD_Tripo_*.glb"))):
    name = os.path.splitext(os.path.basename(glb))[0].replace("TD_Tripo_", "")
    if ONLY and name not in ONLY:
        continue
    folder = f"{DEST}/{name}"
    if unreal.EditorAssetLibrary.does_directory_exist(folder):
        unreal.EditorAssetLibrary.delete_directory(folder)
    task = unreal.AssetImportTask()
    task.filename = glb
    task.destination_path = folder
    task.automated = True
    task.save = True
    task.replace_existing = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    for asset_path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
        asset = unreal.load_asset(asset_path)
        if isinstance(asset, unreal.StaticMesh):
            nanite = asset.get_editor_property("nanite_settings")
            nanite.set_editor_property("enabled", True)
            asset.set_editor_property("nanite_settings", nanite)
            unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
            box = asset.get_bounding_box()
            size = box.max - box.min
            log.append(f"{name}: {asset_path.split('.')[0]} size {size.x:.0f} x {size.y:.0f} x {size.z:.0f}, "
                       f"min z {box.min.z:.0f}, tris {asset.get_num_triangles(0)}")
        elif isinstance(asset, unreal.SkeletalMesh):
            log.append(f"{name}: SKELETAL {asset_path} (strip with glb_strip_skin.py)")
print("\n".join(log))
