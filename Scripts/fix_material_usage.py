# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Cooked builds draw a material only on the mesh types its usage flags allow; otherwise the default grey material
shows (the editor quietly fixes the flag at runtime, so the problem is invisible until a package). This finds every
material the game puts on a skeletal mesh, a Niagara particle or an instanced mesh, sets the missing usage flag,
recompiles and saves. Re-runnable; prints what it changed.
    UnrealEditor-Cmd <uproject> -run=pythonscript -script=<ProjectDir>/Scripts/fix_material_usage.py
"""

import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
ROOT = "/Game"
LEVEL = "/Game/TouchdownMars/Maps/L_TD_Outpost"

needs = {}  # base material path -> set of usage property names


def base_of(mi):
    while mi and isinstance(mi, unreal.MaterialInstance):
        mi = mi.get_editor_property("parent")
    return mi if isinstance(mi, unreal.Material) else None


def need(material, flag, why):
    base = base_of(material)
    if base is None:
        return
    needs.setdefault(base.get_path_name(), {}).setdefault(flag, why)


registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path(ROOT, recursive=True)

for data in assets:
    cls = str(data.asset_class_path.asset_name)
    path = str(data.package_name)
    if cls == "SkeletalMesh":
        mesh = unreal.load_asset(path)
        for slot in mesh.get_editor_property("materials"):
            mi = slot.get_editor_property("material_interface")
            if mi:
                need(mi, "used_with_skeletal_mesh", path)
                if mesh.get_editor_property("morph_targets"):
                    need(mi, "used_with_morph_targets", path)
    elif cls == "NiagaraSystem" and path.startswith("/Game/TouchdownMars"):
        # Niagara renderer materials: the project's dust sprites (sprite renderers).
        pass

# Crew suit override materials (set on the skeletal suit at runtime) and the dust sprite materials.
for name in ("MI_TD_Suit_Air", "MI_TD_Suit_Greenhouse", "MI_TD_Suit_Shelter", "M_TD_SuitTextured", "M_TD_Highlight"):
    p = f"/Game/TouchdownMars/Materials/{name}"
    if EAL.does_asset_exist(p):
        need(unreal.load_asset(p), "used_with_skeletal_mesh", "crew suit / overlay")
for name in ("M_TD_DustSprite", "M_TD_DustSpriteFar"):
    p = f"/Game/TouchdownMars/Materials/{name}"
    if EAL.does_asset_exist(p):
        need(unreal.load_asset(p), "used_with_niagara_sprites", "dust VFX")

# Level: skeletal components with overrides, instanced meshes.
world = unreal.EditorLoadingAndSavingUtils.load_map(LEVEL)
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    for comp in actor.get_components_by_class(unreal.MeshComponent):
        mats = [comp.get_material(i) for i in range(comp.get_num_materials())]
        for m in mats:
            if not m:
                continue
            if isinstance(comp, unreal.SkeletalMeshComponent):
                need(m, "used_with_skeletal_mesh", actor.get_name())
            if isinstance(comp, unreal.InstancedStaticMeshComponent):
                need(m, "used_with_instanced_static_meshes", actor.get_name())
    # Crew actors keep their suit material in a property.
    try:
        sm = actor.get_editor_property("suit_material")
        if sm:
            need(sm, "used_with_skeletal_mesh", actor.get_name())
    except Exception:
        pass

changed = []
for path, flags in sorted(needs.items()):
    if not path.startswith("/Game/"):
        continue
    mat = unreal.load_asset(path)
    if mat is None:
        continue
    missing = [f for f in flags if not mat.get_editor_property(f)]
    if not missing:
        print(f"ok      {path}: {', '.join(sorted(flags))}")
        continue
    for f in missing:
        mat.set_editor_property(f, True)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat, only_if_is_dirty=False)
    changed.append(path)
    print(f"FIXED   {path}: {', '.join(missing)}  (used by {', '.join(flags[f] for f in missing)})")
print(f"fix_material_usage: {len(changed)} material(s) changed")
