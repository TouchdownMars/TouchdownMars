# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Import extra NASA 3D Resources props (github.com/nasa/NASA-3D-Resources) for the outpost and strip their
textures and materials (no insignia, no flags; plain team materials instead). One model per run keeps the
editor's Python call short:
    MODEL = "SEV"; exec(open(r"<ProjectDir>/Scripts/import_nasa_props.py", encoding="utf-8").read())
Source files live in D:/NASA/_kaynaklar/nasa3d (not in the repo).
"""

import os

import unreal

SOURCE = r"D:/NASA/_kaynaklar/nasa3d"
DEST = "/Game/TouchdownMars/NASA"
MAT = "/Game/TouchdownMars/Materials"
PROPS = {
    "SEV": ["Space Exploration Vehicle.glb"],
    "RASSOR": ["Regolith Advanced Surface Systems Operations Robot (RASSOR).glb"],
    "TallDish": ["Tall Dish (dish).glb", "Tall Dish (post).glb", "Tall Dish (turntable).glb"],
    # Hand tools and the relay radome (15 Nov 2026), each downloaded into its own folder; *_static.glb is the
    # same file without its armature (Scripts/glb_strip_skin.py).
    "Hammer": ["Hammer/Hammer_static.glb"],
    "Wrench": ["Wrench/Wrench_static.glb"],
    "PistolGripTool": ["PistolGripTool/PistolGripTool.glb"],
    "Radome": ["Radome/Radome_static.glb"],
}
# Plain colour per model (default white). Hand tools read as steel; NASA's EVA power tool is white.
COLOURS = {"Hammer": ("MI_TD_Plain_Steel", (0.20, 0.21, 0.22)), "Wrench": ("MI_TD_Plain_Steel", (0.20, 0.21, 0.22))}


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


name = globals().get("MODEL", "SEV")
white = plain(*COLOURS.get(name, ("MI_TD_Plain_White", (0.72, 0.72, 0.70))))
folder = f"{DEST}/{name}"
if unreal.EditorAssetLibrary.does_directory_exist(folder):
    unreal.EditorAssetLibrary.delete_directory(folder)
tasks = []
for file_name in PROPS[name]:
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SOURCE, file_name)
    task.destination_path = folder
    task.automated = True
    task.save = True
    task.replace_existing = True
    # Some NASA GLBs carry a node hierarchy that Interchange reads as a skeleton: force static meshes.
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    common = pipeline.get_editor_property("common_meshes_properties")
    common.set_editor_property("force_all_mesh_as_type", unreal.InterchangeForceMeshType.IFMT_STATIC_MESH)
    task.set_editor_property("options", pipeline)
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
log = []
for asset_path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
    asset = unreal.load_asset(asset_path)
    if isinstance(asset, unreal.StaticMesh):
        for index in range(len(asset.get_editor_property("static_materials"))):
            asset.set_material(index, white)
        unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
        size = asset.get_bounding_box().max - asset.get_bounding_box().min
        log.append(f"{asset_path}: {size.x:.0f} x {size.y:.0f} x {size.z:.0f} cm")
    elif isinstance(asset, unreal.SkeletalMesh):
        log.append(f"{asset_path}: SKELETAL (not used)")
for asset_path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
    asset = unreal.load_asset(asset_path)
    if isinstance(asset, (unreal.Texture, unreal.MaterialInterface)):
        unreal.EditorAssetLibrary.delete_asset(asset_path)
print(f"{name}:\n" + "\n".join(log))
