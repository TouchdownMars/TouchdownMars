# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Build the Mars dust VFX: one lit dust sprite material and four lightweight Niagara systems in
/Game/TouchdownMars/VFX, driven at runtime by UTDDustFXSubsystem.

  NS_TD_DustDrift     thin fine dust carried past the player by the daytime east-southeast wind
  NS_TD_DustStorm     dense blowing dust around the player in a dust-storm phase (gusts above 20 m/s)
  NS_TD_DustStormFar  huge slow dust sheets between the player and the horizon in a storm
  NS_TD_DustDevil     a tall rising dust column for the far-away dust devils

Each system is a copy of Epic's FountainLightweight template; its stateless modules are set through
UTDEditorFXLibrary (exported-text import), because Python cannot reach Niagara emitters directly.
Speeds are scaled from research_notes/Mars VFX/vfx_research.md (winds 4 to 8 m/s mid-sol, storm gusts
about 22 m/s, devils 20 to 100 m wide); counts, sizes and colours are look values. Re-runnable.
Run headless:  UnrealEditor-Cmd <uproject> -run=pythonscript -script=<ProjectDir>/Scripts/make_dust_vfx.py
"""

import math

import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
L = unreal.TDEditorFXLibrary
FOLDER = "/Game/TouchdownMars/VFX"
TEMPLATE = "/Niagara/DefaultAssets/Templates/Systems/FountainLightweight"
CLOUD_TEX = "/Engine/EngineVolumetrics/Fogsheet/Materials/T_EV_FogSheet_CloudTex_01"
log = []


# ---------------------------------------------------------------- material

def make_material(name="M_TD_DustSprite", lighting=unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL):
    path = f"{FOLDER}/{name}"
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    # Near dust: lit through the translucency volume (darkens in shadow, glows toward the sun). Far dust (devils,
    # storm sheets) lies beyond the translucency volume, so it is lit per pixel by the sun instead.
    mat.set_editor_property("translucency_lighting_mode", lighting)
    mat.set_editor_property("used_with_niagara_sprites", True)
    mat.set_editor_property("two_sided", True)

    def node(cls, x, y):
        return MEL.create_material_expression(mat, cls, x, y)

    pcolor = node(unreal.MaterialExpressionParticleColor, -900, -200)
    uv = node(unreal.MaterialExpressionTextureCoordinate, -1300, 200)
    half = node(unreal.MaterialExpressionConstant2Vector, -1300, 320)
    half.set_editor_property("r", 0.5)
    half.set_editor_property("g", 0.5)
    sub = node(unreal.MaterialExpressionSubtract, -1100, 250)
    MEL.connect_material_expressions(uv, "", sub, "A")
    MEL.connect_material_expressions(half, "", sub, "B")
    length = node(unreal.MaterialExpressionLength, -950, 250)
    MEL.connect_material_expressions(sub, "", length, "")
    two = node(unreal.MaterialExpressionConstant, -950, 360)
    two.set_editor_property("r", 2.0)
    mul2 = node(unreal.MaterialExpressionMultiply, -800, 250)
    MEL.connect_material_expressions(length, "", mul2, "A")
    MEL.connect_material_expressions(two, "", mul2, "B")
    inv = node(unreal.MaterialExpressionOneMinus, -650, 250)
    MEL.connect_material_expressions(mul2, "", inv, "")
    sat = node(unreal.MaterialExpressionSaturate, -520, 250)
    MEL.connect_material_expressions(inv, "", sat, "")
    expo = node(unreal.MaterialExpressionConstant, -520, 360)
    expo.set_editor_property("r", 1.6)
    radial = node(unreal.MaterialExpressionPower, -380, 250)
    MEL.connect_material_expressions(sat, "", radial, "Base")
    MEL.connect_material_expressions(expo, "", radial, "Exponent")

    tex = node(unreal.MaterialExpressionTextureSample, -800, 0)
    tex.set_editor_property("texture", unreal.load_asset(CLOUD_TEX))
    cloud = node(unreal.MaterialExpressionMultiply, -380, 60)
    MEL.connect_material_expressions(tex, "R", cloud, "A")
    MEL.connect_material_expressions(radial, "", cloud, "B")
    alpha = node(unreal.MaterialExpressionMultiply, -220, 60)
    MEL.connect_material_expressions(cloud, "", alpha, "A")
    MEL.connect_material_expressions(pcolor, "A", alpha, "B")
    fade = node(unreal.MaterialExpressionDepthFade, -60, 60)
    fade.set_editor_property("fade_distance_default", 250.0)
    MEL.connect_material_expressions(alpha, "", fade, "Opacity")

    MEL.connect_material_property(pcolor, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(fade, "", unreal.MaterialProperty.MP_OPACITY)
    rough = node(unreal.MaterialExpressionConstant, -200, -60)
    rough.set_editor_property("r", 1.0)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(mat)
    EAL.save_asset(path, only_if_is_dirty=False)
    log.append(f"material {path}")
    return mat


# ---------------------------------------------------------------- distribution text helpers

def f(v):
    return f"{float(v):.6f}"


def fconst(v):
    return f"(Min={f(v)},Max={f(v)},Mode=UniformConstant,ChannelConstantsAndRanges=({f(v)}))"


def frange(a, b):
    return f"(Min={f(a)},Max={f(b)},Mode=UniformRange,ChannelConstantsAndRanges=({f(a)},{f(b)}))"


def v3(x, y, z):
    return f"(X={f(x)},Y={f(y)},Z={f(z)})"


def v3const(x, y, z):
    return f"(Min={v3(x, y, z)},Max={v3(x, y, z)},Mode=NonUniformConstant,ChannelConstantsAndRanges=({f(x)},{f(y)},{f(z)}))"


def v3range(lo, hi):
    return (f"(Min={v3(*lo)},Max={v3(*hi)},Mode=NonUniformRange,"
            f"ChannelConstantsAndRanges=({f(lo[0])},{f(lo[1])},{f(lo[2])},{f(hi[0])},{f(hi[1])},{f(hi[2])}))")


def v2uniform(a, b):
    return f"(Min=(X={f(a)},Y={f(a)}),Max=(X={f(b)},Y={f(b)}),Mode=UniformRange,ChannelConstantsAndRanges=({f(a)},{f(b)}))"


def rgba(c):
    return f"(R={f(c[0])},G={f(c[1])},B={f(c[2])},A={f(c[3])})"


def color_range(lo, hi):
    return f"(Values=({rgba(lo)},{rgba(hi)}),ValuesTimeRange=(X=0.000000,Y=1.000000),Mode=NonUniformRange)"


def curve(keys):
    return "(Keys=(" + ",".join(f"(Time={f(t)},Value={f(v)})" for t, v in keys) + "))"


def sample(keys, t):
    for (t0, v0), (t1, v1) in zip(keys, keys[1:]):
        if t0 <= t <= t1:
            return v0 + (v1 - v0) * (t - t0) / max(1e-6, t1 - t0)
    return keys[-1][1]


def alpha_curve(keys, n=32):
    """Colour scale over life: white, alpha from keys (fade in and out). Baked LUT plus the editable curves."""
    values = ",".join(rgba((1, 1, 1, sample(keys, i / (n - 1)))) for i in range(n))
    one = curve([(0.0, 1.0)])
    return (f"(Values=({values}),ValuesTimeRange=(X=0.000000,Y=1.000000),Mode=NonUniformCurve,"
            f"ChannelCurves=({one},{one},{one},{curve(keys)}))")


def size_curve(keys, n=32):
    """Sprite size scale over life (uniform X and Y)."""
    values = ",".join(f"(X={f(sample(keys, i / (n - 1)))},Y={f(sample(keys, i / (n - 1)))})" for i in range(n))
    return (f"(Values=({values}),ValuesTimeRange=(X=0.000000,Y=1.000000),Mode=NonUniformCurve,"
            f"ChannelCurves=({curve(keys)},{curve(keys)}))")


# ---------------------------------------------------------------- systems

def module(emitter, cls_name):
    i = 0
    while True:
        m = L.get_array_object(emitter, "Modules", i)
        if not m:
            raise RuntimeError(f"module {cls_name} not found")
        if m.get_class().get_name() == cls_name:
            return m
        i += 1


def setp(obj, name, value):
    if not L.set_property_text(obj, name, value):
        raise RuntimeError(f"{obj.get_class().get_name()}.{name} rejected: {value[:120]}")


def enable(m, on=True):
    setp(m, "bModuleEnabled", "True" if on else "False")


def make_system(name, spec, material, far_material):
    path = f"{FOLDER}/{name}"
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    # Duplicate from the loaded template (a headless run has not scanned plugin content into the registry).
    system = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(name, FOLDER, unreal.load_asset(TEMPLATE))
    em = L.get_lightweight_emitter(system, 0)
    b = spec["bounds"]
    setp(em, "FixedBounds", f"(Min={v3(-b[0], -b[1], -b[2])},Max={v3(b[0], b[1], b[3])},IsValid=True)")
    if "burst" in spec:
        # One puff: a single burst, the emitter runs once and the component frees itself.
        n = spec["burst"]
        setp(em, "EmitterState", "(LoopBehavior=Once,LoopDuration=" + fconst(2.5) + ",bEnableDistanceCulling=False,bEnableVisibilityCulling=False)")
        setp(em, "SpawnInfos", f"((Type=Burst,SpawnTime=0.000000,Amount=(Min={n},Max={n},Mode=UniformConstant)))")
    else:
        setp(em, "EmitterState", "(LoopBehavior=Infinite,LoopDuration=" + fconst(5.0) + ",bEnableDistanceCulling=False,bEnableVisibilityCulling=False)")
        setp(em, "SpawnInfos", f"((Type=Rate,Rate={fconst(spec['rate'])}))")

    init = module(em, "NiagaraStatelessModule_InitializeParticle")
    setp(init, "LifetimeDistribution", frange(*spec["life"]))
    setp(init, "ColorDistribution", color_range(*spec["color"]))
    setp(init, "SpriteSizeDistribution", v2uniform(*spec["size"]))
    setp(init, "SpriteRotationDistribution", frange(0, 360))
    setp(init, "MassDistribution", frange(0.8, 1.2))

    shape = module(em, "NiagaraStatelessModule_ShapeLocation")
    if spec["shape"][0] == "box":
        _, sx, sy, sz = spec["shape"]
        setp(shape, "ShapePrimitive", "Box")
        setp(shape, "BoxSize", v3const(sx, sy, sz))
    else:
        _, radius, height = spec["shape"]
        setp(shape, "ShapePrimitive", "Cylinder")
        setp(shape, "CylinderRadius", frange(radius * 0.2, radius))
        setp(shape, "CylinderHeight", fconst(height))
        setp(shape, "CylinderHeightMidpoint", fconst(0.0))

    vel = module(em, "NiagaraStatelessModule_AddVelocity")
    setp(vel, "VelocityType", "Linear")
    setp(vel, "LinearVelocityDistribution", v3range(*spec["velocity"]))
    setp(vel, "CoordinateSpace", "Local")

    grav = module(em, "NiagaraStatelessModule_GravityForce")
    setp(grav, "GravityDistribution", v3const(0, 0, spec.get("gravity", 0.0)))
    enable(grav, spec.get("gravity", 0.0) != 0.0)
    drag = module(em, "NiagaraStatelessModule_Drag")
    setp(drag, "DragDistribution", fconst(spec.get("drag", 0.1)))
    curl = module(em, "NiagaraStatelessModule_CurlNoiseForce")
    setp(curl, "NoiseStrength", f(spec["curl"][0]))
    setp(curl, "NoiseFrequency", f(spec["curl"][1]))
    enable(curl, True)

    scol = module(em, "NiagaraStatelessModule_ScaleColor")
    setp(scol, "ScaleDistribution", alpha_curve(spec["fade"]))
    enable(scol, True)
    ssize = module(em, "NiagaraStatelessModule_ScaleSpriteSize")
    setp(ssize, "ScaleDistribution", size_curve(spec.get("grow", [(0.0, 1.0), (1.0, 1.4)])))
    enable(ssize, True)
    rot = module(em, "NiagaraStatelessModule_SpriteRotationRate")
    enable(rot, False)
    stretch = module(em, "NiagaraStatelessModule_ScaleSpriteSizeBySpeed")
    if "stretch" in spec:
        # Sand grains as streaks: the sprite's long axis follows the velocity and grows with speed.
        threshold, factor = spec["stretch"]
        setp(stretch, "VelocityThreshold", fconst(threshold))
        setp(stretch, "MinScaleFactor", f"(Min=(X=1.000000,Y=1.000000),Max=(X=1.000000,Y=1.000000),Mode=NonUniformConstant,ChannelConstantsAndRanges=(1.000000,1.000000))")
        setp(stretch, "MaxScaleFactor", f"(Min=(X=1.000000,Y={f(factor)}),Max=(X=1.000000,Y={f(factor)}),Mode=NonUniformConstant,ChannelConstantsAndRanges=(1.000000,{f(factor)}))")
    enable(stretch, "stretch" in spec)

    ren = L.get_array_object(em, "RendererProperties", 0)
    chosen = far_material if spec.get("far") else material
    setp(ren, "Material", f"/Script/Engine.Material'{chosen.get_path_name()}'")
    setp(ren, "bCastShadows", "False")
    setp(ren, "SortMode", "ViewDepth")
    setp(ren, "bMotionBlurEnabled", "False")
    setp(ren, "Alignment", "VelocityAligned" if "stretch" in spec else "Automatic")

    L.refresh_system(system)
    EAL.save_asset(path, only_if_is_dirty=False)
    log.append(f"system {path}: rate {spec.get('rate', spec.get('burst'))}, life {spec['life']}")
    return system


DUST = (0.50, 0.33, 0.21)  # Mars dust, linear albedo look value (butterscotch)
DARK = (0.38, 0.24, 0.15)

SPECS = {
    # Fine dust riding the 4 to 8 m/s mid-sol wind (Lemmon 2022 GRL, section 3.3): slow, sparse, near the ground.
    "NS_TD_DustDrift": dict(
        rate=45, life=(7.0, 11.0), size=(120, 380), shape=("box", 6000, 6000, 260),
        velocity=((400, -60, -10), (750, 60, 35)), curl=(80, 20), drag=0.05,
        color=((*DARK, 0.08), (*DUST, 0.18)), fade=[(0.0, 0.0), (0.25, 1.0), (0.7, 1.0), (1.0, 0.0)],
        grow=[(0.0, 0.8), (1.0, 1.8)], bounds=(9000, 9000, 600, 1500)),
    # Storm gusts above 20 m/s (Lemmon 2022 GRL, sol 313): dense, fast, higher up, the air itself turns to dust.
    "NS_TD_DustStorm": dict(
        rate=1400, life=(2.5, 4.0), size=(250, 900), shape=("box", 5000, 5000, 900),
        velocity=((1500, -200, -40), (2300, 200, 120)), curl=(350, 25), drag=0.05,
        color=((*DARK, 0.22), (*DUST, 0.45)), fade=[(0.0, 0.0), (0.15, 1.0), (0.75, 1.0), (1.0, 0.0)],
        grow=[(0.0, 0.7), (1.0, 2.0)], bounds=(11000, 11000, 600, 2500)),
    # Dust sheets crossing the plain in a storm: very large, slow fade, keeps the distance rolling.
    "NS_TD_DustStormFar": dict(
        rate=160, life=(7.0, 10.0), size=(2500, 6000), shape=("box", 30000, 30000, 2500),
        velocity=((1100, -150, -20), (1700, 150, 80)), curl=(150, 4), drag=0.02,
        color=((*DARK, 0.12), (*DUST, 0.25)), fade=[(0.0, 0.0), (0.3, 1.0), (0.7, 1.0), (1.0, 0.0)],
        grow=[(0.0, 0.8), (1.0, 1.5)], bounds=(40000, 40000, 3000, 9000), far=True),
    # Saltating sand in a storm: grains hop low and fast along the ground and fall back under Mars gravity
    # (3.73 m/s2, NSSDC fact sheet); millimetre grains moved at Jezero in the Jan 2022 storm (Lemmon 2022 GRL, 3.5).
    "NS_TD_SandSkim": dict(
        rate=1600, life=(0.7, 1.3), size=(6, 16), shape=("box", 3000, 3000, 60),
        velocity=((1900, -150, 30), (2800, 150, 140)), curl=(60, 30), drag=0.0, gravity=-373.0,
        color=((0.30, 0.18, 0.11, 0.35), (0.45, 0.29, 0.18, 0.65)), fade=[(0.0, 0.0), (0.1, 1.0), (0.8, 1.0), (1.0, 0.0)],
        grow=[(0.0, 1.0), (1.0, 1.0)], stretch=(800, 9.0), bounds=(6000, 6000, 300, 600)),
    # Boot and tool puff: grains fly out low and fall back under Mars gravity (3.73 m/s2, NSSDC), the fine dust
    # lingers as a small cloud that spreads (Lemmon 2022 JGR: lifted dust recirculates in thin air).
    "NS_TD_StepPuff": dict(
        burst=10, rate=0, life=(1.2, 2.2), size=(14, 40), shape=("box", 30, 30, 4),
        velocity=((-70, -70, 15), (70, 70, 80)), curl=(20, 40), drag=0.6, gravity=-373.0,
        color=((*DARK, 0.25), (*DUST, 0.45)), fade=[(0.0, 0.0), (0.08, 1.0), (0.5, 0.7), (1.0, 0.0)],
        grow=[(0.0, 0.7), (1.0, 2.6)], bounds=(400, 400, 50, 300)),
    # Dust devil column: tens of metres wide at the base, rising and widening (Hueso 2023, Toledo 2023 sizes).
    "NS_TD_DustDevil": dict(
        rate=180, life=(9.0, 13.0), size=(1600, 3600), shape=("cyl", 2600, 200),
        velocity=((-150, -150, 2200), (150, 150, 3400)), curl=(700, 6), drag=0.02,
        color=((0.62, 0.43, 0.29, 0.22), (0.74, 0.53, 0.37, 0.40)), fade=[(0.0, 0.0), (0.1, 1.0), (0.6, 0.8), (1.0, 0.0)],
        grow=[(0.0, 0.6), (1.0, 3.5)], bounds=(15000, 15000, 500, 50000), far=True),
}

if not EAL.does_directory_exist(FOLDER):
    EAL.make_directory(FOLDER)
material = make_material()
far_material = make_material("M_TD_DustSpriteFar", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
for name, spec in SPECS.items():
    make_system(name, spec, material, far_material)
print("\n".join(log))
print("make_dust_vfx: done")
