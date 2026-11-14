# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Phase 11: import NASA 3D Resources models (github.com/nasa/NASA-3D-Resources) and strip their insignia.

The original textures carry the NASA insignia and flags; the game must not imply NASA endorsement, so every
material slot is replaced with a plain team material. Source files live in D:/NASA/_kaynaklar/nasa3d (not in the
repo); rerun after downloading them. Run inside the editor, PIE stopped:
    exec(open(r"<ProjectDir>/Scripts/import_nasa_models.py", encoding="utf-8").read())
"""

import os

import unreal

SOURCE = r"D:/NASA/_kaynaklar/nasa3d"
DEST = "/Game/TouchdownMars/NASA"
MAT = "/Game/TouchdownMars/Materials"
MODELS = {
    "Perseverance": "Mars_2020_Perseverance_Rover.glb",
    "HabitatA": "Habitat_Demonstration_Unit_part_1.glb",
    "HabitatB": "Habitat_Demonstration_Unit_part_2.glb",
    "Z2Suit": "Z2_Spacesuit.glb",
}
log = []


def plain(name, rgb):
    path = f"{MAT}/{name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi = unreal.load_asset(path)
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(*rgb, 1.0))
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    return mi


white = plain("MI_TD_Plain_White", (0.72, 0.72, 0.70))

for name, file_name in MODELS.items():
    folder = f"{DEST}/{name}"
    if unreal.EditorAssetLibrary.does_directory_exist(folder):
        unreal.EditorAssetLibrary.delete_directory(folder)
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SOURCE, file_name)
    task.destination_path = folder
    task.automated = True
    task.save = True
    task.replace_existing = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    meshes = []
    for asset_path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
        asset = unreal.load_asset(asset_path)
        if isinstance(asset, unreal.StaticMesh):
            for index in range(len(asset.get_editor_property("static_materials"))):
                asset.set_material(index, white)
            unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
            bounds = asset.get_bounding_box()
            size = bounds.max - bounds.min
            meshes.append(f"{asset.get_name()} {size.x:.0f}x{size.y:.0f}x{size.z:.0f}cm")
    # Drop the imported textures and materials that carry insignia.
    for asset_path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
        asset = unreal.load_asset(asset_path)
        if isinstance(asset, (unreal.Texture, unreal.MaterialInterface)):
            unreal.EditorAssetLibrary.delete_asset(asset_path)
    log.append(f"{name}: {'; '.join(meshes) or 'no static mesh'}")

print("\n".join(log))
