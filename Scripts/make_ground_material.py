# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Phase 11: M_TD_MarsGround, a procedural regolith material for the Mars Trek landscape (no external textures).
Two world-space noise scales blend three regolith tones; slopes get the darker tone so the measured relief reads.
Megascans surfaces can replace it later (team import into Content/Fab/). Run inside the editor, PIE stopped.
    exec(open(r"<ProjectDir>/Scripts/make_ground_material.py", encoding="utf-8").read())
"""

import unreal

PATH = "/Game/TouchdownMars/Materials"
NAME = "M_TD_MarsGround"
MEL = unreal.MaterialEditingLibrary
full = f"{PATH}/{NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())


def expr(cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def color(rgb, x, y):
    node = expr(unreal.MaterialExpressionConstant3Vector, x, y)
    node.set_editor_property("constant", unreal.LinearColor(*rgb, 1.0))
    return node


def noise(scale, x, y):
    node = expr(unreal.MaterialExpressionNoise, x, y)
    node.set_editor_property("scale", scale)
    node.set_editor_property("levels", 4)
    node.set_editor_property("output_min", 0.0)
    node.set_editor_property("output_max", 1.0)
    node.set_editor_property("noise_function", unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_ALU)
    return node


# Regolith tones (linear): dusty orange, dark basaltic brown, pale dust.
dusty = color((0.32, 0.13, 0.055), -900, -300)
dark = color((0.12, 0.055, 0.03), -900, -150)
pale = color((0.45, 0.24, 0.12), -900, 0)
macro = noise(0.0008, -900, 150)     # metre-scale patches
micro = noise(0.02, -900, 300)       # grain

lerp1 = expr(unreal.MaterialExpressionLinearInterpolate, -600, -250)
MEL.connect_material_expressions(dusty, "", lerp1, "A")
MEL.connect_material_expressions(pale, "", lerp1, "B")
MEL.connect_material_expressions(macro, "", lerp1, "Alpha")

# Slope mask from the world normal: flat = 1, steep = 0.
normal = expr(unreal.MaterialExpressionVertexNormalWS, -900, 450)
mask = expr(unreal.MaterialExpressionComponentMask, -750, 450)
mask.set_editor_property("r", False)
mask.set_editor_property("g", False)
mask.set_editor_property("b", True)
MEL.connect_material_expressions(normal, "", mask, "")
power = expr(unreal.MaterialExpressionPower, -600, 450)
MEL.connect_material_expressions(mask, "", power, "Base")
exponent = expr(unreal.MaterialExpressionConstant, -750, 550)
exponent.set_editor_property("r", 8.0)
MEL.connect_material_expressions(exponent, "", power, "Exp")

lerp2 = expr(unreal.MaterialExpressionLinearInterpolate, -400, -100)
MEL.connect_material_expressions(dark, "", lerp2, "A")
MEL.connect_material_expressions(lerp1, "", lerp2, "B")
MEL.connect_material_expressions(power, "", lerp2, "Alpha")

grain = expr(unreal.MaterialExpressionMultiply, -200, 0)
MEL.connect_material_expressions(lerp2, "", grain, "A")
grain_amount = expr(unreal.MaterialExpressionLinearInterpolate, -400, 250)
low = expr(unreal.MaterialExpressionConstant, -600, 250)
low.set_editor_property("r", 0.82)
high = expr(unreal.MaterialExpressionConstant, -600, 320)
high.set_editor_property("r", 1.1)
MEL.connect_material_expressions(low, "", grain_amount, "A")
MEL.connect_material_expressions(high, "", grain_amount, "B")
MEL.connect_material_expressions(micro, "", grain_amount, "Alpha")
MEL.connect_material_expressions(grain_amount, "", grain, "B")

rough = expr(unreal.MaterialExpressionConstant, -200, 200)
rough.set_editor_property("r", 0.95)
MEL.connect_material_property(grain, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(full, only_if_is_dirty=False)

unreal.LandscapeService.set_landscape_material("TD_Terrain", full)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("material:", full, "nodes:", len(MEL.get_material_expressions(mat)) if hasattr(MEL, "get_material_expressions") else "?")
