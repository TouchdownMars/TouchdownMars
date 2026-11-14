# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""M_TD_Regolith: the Jezero crater floor for TD_Terrain and TD_TerrainFar, from three free Quixel Megascans
surfaces (Fab, imported by VibeUE FabService into /Game/Fab/Free, git-ignored; listed in sources.csv):
Gravel on Soil (base), Dry Soil Ground (slopes and patches) and Dry Rippled Sand (wind ripples in sand patches).

No paint layers: world-space noise decides where sand lies, slope decides where the darker soil shows, and the
sand edge follows the texture heights. The Earth colours are pulled toward a dusty Mars tone (designer values,
matched by eye to Perseverance Mastcam-Z natural-colour views). Run inside the editor, PIE stopped:
    exec(open(r"<ProjectDir>/Scripts/make_regolith_material.py", encoding="utf-8").read())
"""

import unreal

PATH, NAME = "/Game/TouchdownMars/Materials", "M_TD_Regolith"
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
SETS = {
    "gravel": "/Game/Fab/Free/Gravel_on_Soil/okosdmp0_tier_1/Textures/T_okosdmp0_4K_",
    "soil": "/Game/Fab/Free/Dry_Soil_Ground/umsndibcw_tier_1/Textures/T_umsndibcw_4K_",
    "sand": "/Game/Fab/Free/Dry_Rippled_Sand/vl3jdcpg_tier_1/Textures/T_vl3jdcpg_4K_",
}
TILE_CM = {"gravel": 240.0, "soil": 420.0, "sand": 380.0}
MARS_TINT = (1.0, 0.76, 0.58)      # designer value
DUST = (0.36, 0.22, 0.14)          # linear dust tone laid over everything, designer value
DUST_AMOUNT = 0.30

for prefix in SETS.values():
    for suffix in ("B", "N", "ORM", "H"):
        if not EAL.does_asset_exist(prefix + suffix):
            raise RuntimeError(f"missing {prefix + suffix}: import the Fab surfaces first")

full = f"{PATH}/{NAME}"
if EAL.does_asset_exist(full):
    mat = unreal.load_asset(full)
    MEL.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())

X = [0]


def node(cls, x=None, y=0, **props):
    n = MEL.create_material_expression(mat, cls, -1800 + (x if x is not None else 0), y)
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


def lerp(a, b, alpha, x, y):
    n = node(unreal.MaterialExpressionLinearInterpolate, x, y)
    link(a, n, "A")
    link(b, n, "B")
    link(alpha, n, "Alpha")
    return n


world = node(unreal.MaterialExpressionWorldPosition, 0, 0)
xy = node(unreal.MaterialExpressionComponentMask, 150, 0, r=True, g=True, b=False, a=False)
link(world, xy)

SAMPLER = {"B": unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, "N": unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
           "ORM": unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR}
tex = {}
row = 0
for key, prefix in SETS.items():
    uv = op(unreal.MaterialExpressionDivide, xy, const(TILE_CM[key], 150, row * 520 + 100), 300, row * 520)
    for i, suffix in enumerate(("B", "N", "ORM", "H")):
        texture = unreal.load_asset(prefix + suffix)
        if suffix == "H":
            sampler = (unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE if not texture.get_editor_property("srgb")
                       else unreal.MaterialSamplerType.SAMPLERTYPE_GRAYSCALE)
            if texture.get_editor_property("compression_settings") not in (
                    unreal.TextureCompressionSettings.TC_GRAYSCALE, unreal.TextureCompressionSettings.TC_ALPHA):
                sampler = (unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR if not texture.get_editor_property("srgb")
                           else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        else:
            sampler = SAMPLER[suffix]
        s = node(unreal.MaterialExpressionTextureSample, 500, row * 520 + i * 125, texture=texture, sampler_type=sampler)
        link(uv, s, "UVs")
        tex[(key, suffix)] = s
    row += 1

# Masks: slope from the vertex normal, sand patches from two octaves of world noise.
normal_ws = node(unreal.MaterialExpressionVertexNormalWS, 800, 1700)
nz = node(unreal.MaterialExpressionComponentMask, 950, 1700, r=False, g=False, b=True, a=False)
link(normal_ws, nz)
slope = node(unreal.MaterialExpressionSmoothStep, 1100, 1700)
link(nz, slope, "Value")
link(const(0.97, 950, 1800), slope, "Min")
link(const(0.85, 950, 1860), slope, "Max")   # 0 on flat ground, 1 on slopes steeper than about 32 degrees

noise_big = node(unreal.MaterialExpressionNoise, 800, 1950, scale=0.00045, levels=3, output_min=0.0, output_max=1.0,
                 quality=1)
link(world, noise_big, "Position")
noise_small = node(unreal.MaterialExpressionNoise, 800, 2150, scale=0.0012, levels=3, output_min=0.0, output_max=1.0,
                   quality=1)
link(world, noise_small, "Position")
mix = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, noise_big, const(0.8, 950, 2000), 1000, 1950),
         op(unreal.MaterialExpressionMultiply, noise_small, const(0.2, 950, 2200), 1000, 2150), 1150, 2050)
sand_raw = node(unreal.MaterialExpressionSmoothStep, 1300, 2050)
link(mix, sand_raw, "Value")
link(const(0.50, 1150, 2150), sand_raw, "Min")
link(const(0.62, 1150, 2210), sand_raw, "Max")
flat = op(unreal.MaterialExpressionSubtract, const(1.0, 1250, 1650), slope, 1300, 1700)
sand_w = op(unreal.MaterialExpressionMultiply, sand_raw, flat, 1450, 2000)

soil_raw = node(unreal.MaterialExpressionSmoothStep, 1300, 2300)
link(noise_small, soil_raw, "Value")
link(const(0.58, 1150, 2350), soil_raw, "Min")
link(const(0.86, 1150, 2410), soil_raw, "Max")
soil_w = op(unreal.MaterialExpressionMax, soil_raw, slope, 1450, 2300)


def height_alpha(weight, h_top, h_base, x, y):
    # alpha = saturate((w - 0.5) * 4 + (h_top - h_base) * 2 + 0.5)
    w = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionSubtract, weight, const(0.5, x, y + 60), x + 120, y),
           const(2.5, x + 120, y + 60), x + 240, y)
    dh = op(unreal.MaterialExpressionMultiply,
            op(unreal.MaterialExpressionSubtract, h_top, h_base, x + 120, y + 120), const(2.0, x + 120, y + 180), x + 240, y + 120)
    s = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionAdd, w, dh, x + 360, y), const(0.5, x + 360, y + 60),
           x + 480, y)
    sat = node(unreal.MaterialExpressionSaturate, x + 600, y)
    link(s, sat)
    return sat


def r_of(sample):
    m = node(unreal.MaterialExpressionComponentMask, 900, 0, r=True, g=False, b=False, a=False)
    link(sample, m)
    return m


a_soil = height_alpha(soil_w, r_of(tex[("soil", "H")]), r_of(tex[("gravel", "H")]), 1600, 2300)
a_sand = height_alpha(sand_w, r_of(tex[("sand", "H")]), r_of(tex[("gravel", "H")]), 1600, 2000)


def blend(suffix, y):
    base = lerp(tex[("gravel", suffix)], tex[("soil", suffix)], a_soil, 2400, y)
    return lerp(base, tex[("sand", suffix)], a_sand, 2550, y)


color = blend("B", 0)
normal = blend("N", 300)
orm = blend("ORM", 600)

# Mars grade: desaturate a little, warm tint, then a layer of dust over everything.
desat = node(unreal.MaterialExpressionDesaturation, 2700, 0, luminance_factors=unreal.LinearColor(0.3, 0.59, 0.11, 0.0))
link(color, desat, "")
link(const(0.55, 2600, 80), desat, "Fraction")
tinted = op(unreal.MaterialExpressionMultiply, desat, const(MARS_TINT, 2700, 120), 2850, 0)
dusted = lerp(tinted, const(DUST, 2850, 120), const(DUST_AMOUNT, 2850, 180), 3000, 0)
# Large-scale brightness variation so the tiling does not read from far away.
macro = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, noise_big, const(0.3, 2900, 260), 3000, 260),
           const(0.85, 3000, 320), 3100, 260)
final_color = op(unreal.MaterialExpressionMultiply, dusted, macro, 3200, 0)

rough = node(unreal.MaterialExpressionComponentMask, 2800, 600, r=False, g=True, b=False, a=False)
link(orm, rough)
rough_hi = op(unreal.MaterialExpressionMax, rough, const(0.8, 2800, 680), 2950, 600)   # dry regolith is matte
ao = node(unreal.MaterialExpressionComponentMask, 2800, 760, r=True, g=False, b=False, a=False)
link(orm, ao)

MEL.connect_material_property(final_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
MEL.connect_material_property(rough_hi, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
MEL.connect_material_property(const(0.0, 3200, 800), "", unreal.MaterialProperty.MP_METALLIC)
MEL.recompile_material(mat)
EAL.save_asset(full)

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() in ("TD_Terrain", "TD_TerrainFar"):
        actor.set_editor_property("landscape_material", mat)
        print("material on", actor.get_actor_label())
print("regolith material ok")
