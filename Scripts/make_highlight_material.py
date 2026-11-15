# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""M_TD_Highlight: the interaction highlight drawn over whatever the player looks at (inspectable parts, crew).
A warm rim (Fresnel) that breathes slowly, used as a mesh overlay material by ATDAstronautCharacter, so the object
keeps its own look and only its silhouette glows. Colour is the design accent family (look value). Re-runnable:
    UnrealEditor-Cmd <uproject> -run=pythonscript -script=<ProjectDir>/Scripts/make_highlight_material.py
"""

import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
FOLDER = "/Game/TouchdownMars/Materials"
NAME = "M_TD_Highlight"
path = f"{FOLDER}/{NAME}"
if EAL.does_asset_exist(path):
    EAL.delete_asset(path)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("used_with_skeletal_mesh", True)
mat.set_editor_property("used_with_static_lighting", False)


def node(cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


fres = node(unreal.MaterialExpressionFresnel, -700, 0)
fres.set_editor_property("exponent", 2.5)
fres.set_editor_property("base_reflect_fraction", 0.04)
time = node(unreal.MaterialExpressionTime, -900, 200)
speed = node(unreal.MaterialExpressionConstant, -900, 300)
speed.set_editor_property("r", 3.0)
tmul = node(unreal.MaterialExpressionMultiply, -750, 220)
MEL.connect_material_expressions(time, "", tmul, "A")
MEL.connect_material_expressions(speed, "", tmul, "B")
sine = node(unreal.MaterialExpressionSine, -600, 220)
MEL.connect_material_expressions(tmul, "", sine, "")
# pulse = 0.75 + 0.25 * sin(3 t)
quarter = node(unreal.MaterialExpressionConstant, -600, 320)
quarter.set_editor_property("r", 0.25)
pmul = node(unreal.MaterialExpressionMultiply, -450, 240)
MEL.connect_material_expressions(sine, "", pmul, "A")
MEL.connect_material_expressions(quarter, "", pmul, "B")
base = node(unreal.MaterialExpressionConstant, -450, 340)
base.set_editor_property("r", 0.75)
pulse = node(unreal.MaterialExpressionAdd, -300, 260)
MEL.connect_material_expressions(pmul, "", pulse, "A")
MEL.connect_material_expressions(base, "", pulse, "B")
rim = node(unreal.MaterialExpressionMultiply, -300, 60)
MEL.connect_material_expressions(fres, "", rim, "A")
MEL.connect_material_expressions(pulse, "", rim, "B")

color = node(unreal.MaterialExpressionConstant3Vector, -300, -140)
color.set_editor_property("constant", unreal.LinearColor(1.0, 0.72, 0.38, 1.0))
glow = node(unreal.MaterialExpressionConstant, -300, -40)
glow.set_editor_property("r", 6.0)
cmul = node(unreal.MaterialExpressionMultiply, -150, -120)
MEL.connect_material_expressions(color, "", cmul, "A")
MEL.connect_material_expressions(glow, "", cmul, "B")
emis = node(unreal.MaterialExpressionMultiply, 0, -40)
MEL.connect_material_expressions(cmul, "", emis, "A")
MEL.connect_material_expressions(rim, "", emis, "B")
op_scale = node(unreal.MaterialExpressionConstant, -150, 160)
op_scale.set_editor_property("r", 0.85)
opac = node(unreal.MaterialExpressionMultiply, 0, 100)
MEL.connect_material_expressions(rim, "", opac, "A")
MEL.connect_material_expressions(op_scale, "", opac, "B")

MEL.connect_material_property(emis, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(opac, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(mat)
EAL.save_asset(path, only_if_is_dirty=False)
print("make_highlight_material: done")
