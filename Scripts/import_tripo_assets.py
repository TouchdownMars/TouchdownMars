# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Import the Tripo AI-generated models (D:/NASA/_kaynaklar/tripo, not in the repo) into
/Game/TouchdownMars/Tripo. Static props get Nanite (Tripo meshes are ~2 M triangles); the rigged astronaut FBX
keeps its skeleton and six animations. AI-generated: every asset is listed in sources.csv (bIsAIGenerated=True).
Run headless (no 30 s limit):
    UnrealEditor-Cmd.exe TouchdownMars.uproject -run=pythonscript -script=<ProjectDir>/Scripts/import_tripo_assets.py
"""

import glob
import os

import unreal

SOURCE = r"D:/NASA/_kaynaklar/tripo"
DEST = "/Game/TouchdownMars/Tripo"
log = []


def report(message):
    print(message)
    unreal.log(message)


def import_file(path, folder):
    if unreal.EditorAssetLibrary.does_directory_exist(folder):
        unreal.EditorAssetLibrary.delete_directory(folder)
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = folder
    task.automated = True
    task.save = True
    task.replace_existing = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])


for glb in sorted(glob.glob(os.path.join(SOURCE, "TD_Tripo_*.glb"))):
    name = os.path.splitext(os.path.basename(glb))[0].replace("TD_Tripo_", "")
    folder = f"{DEST}/{name}"
    import_file(glb, folder)
    for asset_path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
        asset = unreal.load_asset(asset_path)
        if isinstance(asset, unreal.StaticMesh):
            nanite = asset.get_editor_property("nanite_settings")
            nanite.set_editor_property("enabled", True)
            asset.set_editor_property("nanite_settings", nanite)
            unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
            box = asset.get_bounding_box()
            size = box.max - box.min
            report(f"[tripo] {name}: {asset_path} {size.x:.0f} x {size.y:.0f} x {size.z:.0f} cm, Nanite on")

fbx = glob.glob(os.path.join(SOURCE, "astronaut", "*.fbx"))
if fbx:
    folder = f"{DEST}/Astronaut"
    import_file(fbx[0], folder)
    for asset_path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
        asset = unreal.load_asset(asset_path)
        kind = asset.get_class().get_name()
        extra = ""
        if isinstance(asset, unreal.AnimSequence):
            extra = f" length {asset.get_editor_property('sequence_length'):.2f} s" if hasattr(asset, "get_editor_property") else ""
        if kind in ("SkeletalMesh", "Skeleton", "AnimSequence", "PhysicsAsset"):
            report(f"[tripo] Astronaut: {kind} {asset_path}{extra}")
report("[tripo] IMPORT DONE")
