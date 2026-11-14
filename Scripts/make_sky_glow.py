# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Blue glow around the sun near sunset (the Mars "blue sunset"), as a large additive sky shell.

UE's SkyAtmosphere has one Mie phase function for all colours, so it cannot make the dust scatter blue light
more strongly forward than red. On Mars that is what happens (Curiosity's sunset, PIA19400): the sky right
around a low sun is blue-grey while the rest stays butterscotch. This shell adds that glow: colour taken from
PIA19400 (#A9C3DF, research_notes/Mars gokyuzu ve zemin/references.md), shape and strength are designer values.
UTDSkySubsystem writes the sun direction and the glow strength (strong only when the sun is low, weak in dust
storms) into MPC_TD_Sky every frame. Run inside the editor, PIE stopped:
    exec(open(r"<ProjectDir>/Scripts/make_sky_glow.py", encoding="utf-8").read())
"""

import unreal

PATH = "/Game/TouchdownMars/Materials"
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Parameter collection the C++ writes into.
mpc_path = f"{PATH}/MPC_TD_Sky"
if EAL.does_asset_exist(mpc_path):
    mpc = unreal.load_asset(mpc_path)
else:
    mpc = tools.create_asset("MPC_TD_Sky", PATH, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
glow = unreal.CollectionScalarParameter()
glow.set_editor_property("parameter_name", "BlueGlow")
glow.set_editor_property("default_value", 0.0)
sun = unreal.CollectionVectorParameter()
sun.set_editor_property("parameter_name", "SunDir")
sun.set_editor_property("default_value", unreal.LinearColor(-0.7, 0.5, 0.1, 0.0))
mpc.set_editor_property("scalar_parameters", [glow])
mpc.set_editor_property("vector_parameters", [sun])
EAL.save_asset(mpc_path)

# Additive, unlit, no fog: it only adds light on top of the sky behind everything else.
mat_path = f"{PATH}/M_TD_SkyGlow"
if EAL.does_asset_exist(mat_path):
    mat = unreal.load_asset(mat_path)
    MEL.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset("M_TD_SkyGlow", PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("two_sided", True)
for prop, value in (("use_translucency_vertex_fog", False), ("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)):
    try:
        mat.set_editor_property(prop, value)
    except Exception as error:
        print("skip", prop, error)


def node(cls, x, y, **props):
    n = MEL.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


def link(a, b, pin="", out=""):
    MEL.connect_material_expressions(a, out, b, pin)


def const(v, x, y):
    if isinstance(v, tuple):
        return node(unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(v[0], v[1], v[2], 1.0))
    return node(unreal.MaterialExpressionConstant, x, y, r=v)


def op(cls, a, b, x, y):
    n = node(cls, x, y)
    link(a, n, "A")
    link(b, n, "B")
    return n


cam = node(unreal.MaterialExpressionCameraVectorWS, -1400, 0)          # pixel -> camera
view = op(unreal.MaterialExpressionMultiply, cam, const(-1.0, -1400, 100), -1250, 0)
sun_dir = node(unreal.MaterialExpressionCollectionParameter, -1400, 200, collection=mpc, parameter_name="SunDir")
sun_rgb = node(unreal.MaterialExpressionComponentMask, -1250, 200, r=True, g=True, b=True, a=False)
link(sun_dir, sun_rgb)
dot = node(unreal.MaterialExpressionDotProduct, -1100, 100)
link(view, dot, "A")
link(sun_rgb, dot, "B")
d = node(unreal.MaterialExpressionSaturate, -950, 100)
link(dot, d)
narrow = node(unreal.MaterialExpressionPower, -800, 50)
link(d, narrow, "Base")
link(const(140.0, -950, 0), narrow, "Exp")
wide = node(unreal.MaterialExpressionPower, -800, 200)
link(d, wide, "Base")
link(const(22.0, -950, 250), wide, "Exp")
shape = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, narrow, const(0.65, -700, 0), -650, 50),
           op(unreal.MaterialExpressionMultiply, wide, const(0.35, -700, 250), -650, 200), -500, 100)
# Fade out below the horizon so the ground never glows.
vz = node(unreal.MaterialExpressionComponentMask, -1100, 350, r=False, g=False, b=True, a=False)
link(view, vz)
horizon = node(unreal.MaterialExpressionSaturate, -800, 350)
link(op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionAdd, vz, const(0.03, -950, 420), -950, 350),
        const(25.0, -950, 480), -850, 350), horizon)
strength = node(unreal.MaterialExpressionCollectionParameter, -650, 450, collection=mpc, parameter_name="BlueGlow")
amount = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, shape, horizon, -400, 200), strength, -300, 300)
# #A9C3DF (PIA19400) is what the eye sees after the glow mixes with the butterscotch sky; the added light itself
# has to be a stronger blue to land there (designer value, matched by eye).
colour = const((0.16, 0.38, 1.0), -400, 450)
emissive = op(unreal.MaterialExpressionMultiply, colour, amount, -150, 350)
MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
EAL.save_asset(mat_path)

# The shell: a 50 km sphere around the base; the camera is always inside it.
if les.get_current_level().get_outer().get_name() != "L_TD_Outpost":
    les.load_level("/Game/TouchdownMars/Maps/L_TD_Outpost")
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() == "TD_SkyGlow":
        actors.destroy_actor(actor)
shell = actors.spawn_actor_from_object(unreal.load_asset("/Engine/BasicShapes/Sphere"), unreal.Vector(0, 0, 0))
shell.set_actor_label("TD_SkyGlow")
shell.set_actor_scale3d(unreal.Vector(100000.0, 100000.0, 100000.0))
comp = shell.static_mesh_component
comp.set_material(0, mat)
comp.set_cast_shadow(False)
comp.set_collision_profile_name("NoCollision")   # never block traces: ground checks run from high above
comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
comp.set_editor_property("affect_distance_field_lighting", False)
comp.set_editor_property("visible_in_reflection_captures", False)
comp.set_editor_property("visible_in_real_time_sky_captures", False)
les.save_current_level()
print("sky glow ok")
