# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Surface detail for the outpost's code-built parts and the crew suits (run inside the editor, PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/make_hull_material.py", encoding="utf-8").read())

M_TD_HullPanel: the modules, corridors, tanks and props were plain BasicShapeMaterial colours. This parent keeps
each instance's "Color" and adds a scanned painted-metal surface (free Quixel Megascans "Painted Metal" via Fab,
/Game/Fab/Free, git-ignored; sources.csv row) projected in world space from three sides, so stretched cubes and
cylinders get even seams and wear. Mars dust settles on top faces and near the ground (designer values).
Scalars: Panel (0 = no metal detail, for regolith), Dust (amount), Metallic.

M_TD_SuitTextured: the crew suits get their Tripo textures back; the instance "Color" (system colour) is now a
light tint over the textured suit instead of a flat paint. Strongly red printed patches are turned neutral grey so
nothing on the suit reads as a flag or insignia.

The MI_TD_* instances are re-parented, so no level actor changes.
"""

import unreal

PATH = "/Game/TouchdownMars/Materials"
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
METAL = "/Game/Fab/Free/Painted_Metal/ub4jadtlw_tier_1/Textures/T_ub4jadtlw_4K_"
SUIT = "/Game/TouchdownMars/Tripo/Astronaut/astronaut_3d_model_"
TILE_CM = 260.0
DUST = (0.36, 0.22, 0.14)


def new_material(name):
    full = f"{PATH}/{name}"
    if EAL.does_asset_exist(full):
        mat = unreal.load_asset(full)
        MEL.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset(name, PATH, unreal.Material, unreal.MaterialFactoryNew())
    return mat, full


class G:
    def __init__(self, mat):
        self.mat = mat

    def node(self, cls, x, y, **props):
        n = MEL.create_material_expression(self.mat, cls, x, y)
        for k, v in props.items():
            n.set_editor_property(k, v)
        return n

    def link(self, a, b, pin="", out=""):
        MEL.connect_material_expressions(a, out, b, pin)

    def const(self, v, x, y):
        if isinstance(v, tuple):
            return self.node(unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(v[0], v[1], v[2], 1.0))
        return self.node(unreal.MaterialExpressionConstant, x, y, r=v)

    def op(self, cls, a, b, x, y):
        n = self.node(cls, x, y)
        self.link(a, n, "A")
        self.link(b, n, "B")
        return n

    def lerp(self, a, b, alpha, x, y):
        n = self.node(unreal.MaterialExpressionLinearInterpolate, x, y)
        self.link(a, n, "A")
        self.link(b, n, "B")
        self.link(alpha, n, "Alpha")
        return n

    def mask(self, src, r, g, b, x, y):
        n = self.node(unreal.MaterialExpressionComponentMask, x, y, r=r, g=g, b=b, a=False)
        self.link(src, n)
        return n

    def append(self, a, b, x, y):
        return self.op(unreal.MaterialExpressionAppendVector, a, b, x, y)

    def vparam(self, name, default, x, y):
        return self.node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                         default_value=unreal.LinearColor(*default, 1.0))

    def sparam(self, name, default, x, y):
        return self.node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=default)


# ---------------------------------------------------------------- M_TD_HullPanel
for suffix in ("B", "N", "ORM"):
    assert EAL.does_asset_exist(METAL + suffix), "import the Fab 'Painted Metal' surface first"
mat, hull_path = new_material("M_TD_HullPanel")
g = G(mat)
mat.set_editor_property("tangent_space_normal", False)   # the normal below is built in world space

world = g.node(unreal.MaterialExpressionWorldPosition, -2400, 0)
scaled = g.op(unreal.MaterialExpressionDivide, world, g.const(TILE_CM, -2400, 100), -2250, 0)
wx = g.mask(scaled, True, False, False, -2100, -100)
wy = g.mask(scaled, False, True, False, -2100, 0)
wz = g.mask(scaled, False, False, True, -2100, 100)
uv_x = g.append(wy, wz, -1950, -200)    # seen along X
uv_y = g.append(wx, wz, -1950, 0)       # seen along Y
uv_z = g.append(wx, wy, -1950, 200)     # seen from above

normal_ws = g.node(unreal.MaterialExpressionVertexNormalWS, -2100, 500)
absn = g.node(unreal.MaterialExpressionAbs, -1950, 500)
g.link(normal_ws, absn)
sharp = g.node(unreal.MaterialExpressionPower, -1800, 500)
g.link(absn, sharp, "Base")
g.link(g.const(4.0, -1950, 600), sharp, "Exp")
total = g.op(unreal.MaterialExpressionAdd, g.op(unreal.MaterialExpressionAdd, g.mask(sharp, True, False, False, -1650, 450),
                                                g.mask(sharp, False, True, False, -1650, 520), -1500, 450),
             g.mask(sharp, False, False, True, -1650, 590), -1350, 500)
weights = g.op(unreal.MaterialExpressionDivide, sharp, total, -1200, 500)
w_x = g.mask(weights, True, False, False, -1050, 450)
w_y = g.mask(weights, False, True, False, -1050, 520)
w_z = g.mask(weights, False, False, True, -1050, 590)


def tri(suffix, sampler, y):
    texture = unreal.load_asset(METAL + suffix)
    out = []
    for i, uv in enumerate((uv_x, uv_y, uv_z)):
        s = g.node(unreal.MaterialExpressionTextureSample, -1700, y + i * 140, texture=texture, sampler_type=sampler)
        g.link(uv, s, "UVs")
        out.append(s)
    return out


b_x, b_y, b_z = tri("B", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -900)
o_x, o_y, o_z = tri("ORM", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, -450)
n_x, n_y, n_z = tri("N", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 900)


def blend3(ax, ay, az, x, y):
    return g.op(unreal.MaterialExpressionAdd,
                g.op(unreal.MaterialExpressionAdd, g.op(unreal.MaterialExpressionMultiply, ax, w_x, x, y),
                     g.op(unreal.MaterialExpressionMultiply, ay, w_y, x, y + 70), x + 150, y),
                g.op(unreal.MaterialExpressionMultiply, az, w_z, x, y + 140), x + 300, y)


base_rgb = blend3(b_x, b_y, b_z, -1000, -900)
orm = blend3(o_x, o_y, o_z, -1000, -450)
# Wear only: the paint's own colour is dropped (it is blue); its brightness becomes grime and chips.
lum = g.node(unreal.MaterialExpressionDesaturation, -650, -900, luminance_factors=unreal.LinearColor(0.3, 0.59, 0.11, 0.0))
g.link(base_rgb, lum, "")
g.link(g.const(1.0, -750, -820), lum, "Fraction")
grime = g.op(unreal.MaterialExpressionDivide, g.mask(lum, True, False, False, -500, -900), g.const(0.45, -500, -820), -350, -900)
panel = g.sparam("Panel", 1.0, -500, -700)
sat = g.node(unreal.MaterialExpressionSaturate, -250, -1000)
g.link(grime, sat)
detail = g.lerp(g.const(1.0, -350, -760), g.op(unreal.MaterialExpressionAdd, g.op(unreal.MaterialExpressionMultiply, sat, g.const(0.5, -250, -950), -150, -1000), g.const(0.55, -150, -940), -50, -1000), panel, 50, -850)
colour = g.vparam("Color", (0.62, 0.62, 0.60), -200, -650)
painted = g.op(unreal.MaterialExpressionMultiply, colour, detail, 200, -750)

# Dust: on faces that look up, and on everything close to the ground (the base pad is Z = 0).
up = g.node(unreal.MaterialExpressionSaturate, 0, -300)
g.link(g.op(unreal.MaterialExpressionMultiply, g.op(unreal.MaterialExpressionSubtract, g.mask(normal_ws, False, False, True, -300, -300),
                                                   g.const(0.55, -300, -230), -150, -300), g.const(2.5, -150, -230), -50, -300), up)
low = g.node(unreal.MaterialExpressionSaturate, 0, -150)
g.link(g.op(unreal.MaterialExpressionSubtract, g.const(1.0, -150, -100),
            g.op(unreal.MaterialExpressionDivide, g.mask(world, False, False, True, -300, -150), g.const(140.0, -300, -80), -150, -150), -50, -150), low)
dust_noise = g.node(unreal.MaterialExpressionNoise, 0, 0, scale=0.01, levels=2, output_min=0.4, output_max=1.0, quality=1)
g.link(world, dust_noise, "Position")
dust_mask = g.op(unreal.MaterialExpressionMultiply,
                 g.op(unreal.MaterialExpressionMax, g.op(unreal.MaterialExpressionMultiply, up, g.const(0.45, 100, -260), 200, -300),
                      g.op(unreal.MaterialExpressionMultiply, low, g.const(0.6, 100, -100), 200, -150), 350, -220),
                 dust_noise, 500, -200)
dust_amount = g.sparam("Dust", 1.0, 500, -80)
dust_alpha = g.op(unreal.MaterialExpressionMultiply, dust_mask, dust_amount, 650, -200)
final_colour = g.lerp(painted, g.const(DUST, 650, -400), dust_alpha, 800, -500)

rough = g.lerp(g.const(0.6, 600, 0), g.mask(orm, False, True, False, 450, 50), panel, 750, 0)
rough_dusty = g.lerp(rough, g.const(0.95, 750, 100), dust_alpha, 900, 0)
metallic = g.op(unreal.MaterialExpressionMultiply, g.sparam("Metallic", 0.15, 750, 200), panel, 900, 200)

# World-space normal: each projection's tangent normal tilts the surface normal along its own two axes.
def tilt(n, axes, x, y):
    xy = g.mask(n, True, True, False, x, y)
    nx = g.mask(xy, True, False, False, x + 120, y)
    ny = g.mask(xy, False, True, False, x + 120, y + 60)
    zero = g.const(0.0, x + 120, y + 120)
    parts = {"x": zero, "y": zero, "z": zero}
    parts[axes[0]] = nx
    parts[axes[1]] = ny
    return g.append(g.append(parts["x"], parts["y"], x + 240, y), parts["z"], x + 360, y)


t = g.op(unreal.MaterialExpressionAdd,
         g.op(unreal.MaterialExpressionAdd, g.op(unreal.MaterialExpressionMultiply, tilt(n_x, "yz", -1500, 900), w_x, -1100, 900),
              g.op(unreal.MaterialExpressionMultiply, tilt(n_y, "xz", -1500, 1100), w_y, -1100, 1000), -950, 900),
         g.op(unreal.MaterialExpressionMultiply, tilt(n_z, "xy", -1500, 1300), w_z, -1100, 1100), -800, 950)
t = g.op(unreal.MaterialExpressionMultiply, t, panel, -650, 950)
t = g.op(unreal.MaterialExpressionMultiply, t, g.op(unreal.MaterialExpressionSubtract, g.const(1.0, -650, 1050), dust_alpha, -500, 1050), -500, 950)
n_world = g.node(unreal.MaterialExpressionNormalize, -350, 950)
g.link(g.op(unreal.MaterialExpressionAdd, normal_ws, t, -450, 950), n_world)

MEL.connect_material_property(final_colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(rough_dusty, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
MEL.connect_material_property(n_world, "", unreal.MaterialProperty.MP_NORMAL)
MEL.recompile_material(mat)
EAL.save_asset(hull_path)

# ---------------------------------------------------------------- M_TD_SuitTextured
mat, suit_path = new_material("M_TD_SuitTextured")
g = G(mat)
for suffix in ("basecolor", "normal", "roughness"):
    assert EAL.does_asset_exist(SUIT + suffix)
base = g.node(unreal.MaterialExpressionTextureSample, -900, 0, texture=unreal.load_asset(SUIT + "basecolor"))
nrm = g.node(unreal.MaterialExpressionTextureSample, -900, 300, texture=unreal.load_asset(SUIT + "normal"),
             sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
rgh_tex = unreal.load_asset(SUIT + "roughness")
rgh = g.node(unreal.MaterialExpressionTextureSample, -900, 600, texture=rgh_tex,
             sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR if not rgh_tex.get_editor_property("srgb")
             else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
tint = g.vparam("Color", (1.0, 1.0, 1.0), -900, -250)
# The tint colours the white suit fabric lightly; dark and coloured parts keep their texture.
soft = g.lerp(g.const((1.0, 1.0, 1.0), -650, -300), tint, g.const(0.6, -650, -230), -500, -250)
# Red printed patches on the generated suit could read as a flag: strongly red texels go neutral grey.
red = g.mask(base, True, False, False, -800, -450)
green_blue = g.op(unreal.MaterialExpressionMax, g.mask(base, False, True, False, -800, -400), g.mask(base, False, False, True, -800, -350), -650, -400)
redness = g.node(unreal.MaterialExpressionSaturate, -400, -420)
g.link(g.op(unreal.MaterialExpressionMultiply, g.op(unreal.MaterialExpressionSubtract, g.op(unreal.MaterialExpressionSubtract, red, green_blue, -550, -450),
                                                    g.const(0.12, -550, -380), -480, -450), g.const(6.0, -480, -380), -420, -450), redness)
grey = g.node(unreal.MaterialExpressionDesaturation, -500, -550, luminance_factors=unreal.LinearColor(0.3, 0.59, 0.11, 0.0))
g.link(base, grey, "")
g.link(g.const(1.0, -600, -520), grey, "Fraction")
neutral_base = g.lerp(base, g.op(unreal.MaterialExpressionMultiply, grey, g.const(0.8, -400, -520), -300, -550), redness, -250, -450)
coloured = g.op(unreal.MaterialExpressionMultiply, neutral_base, soft, -350, -100)
boots = g.node(unreal.MaterialExpressionSaturate, -500, 150)
world = g.node(unreal.MaterialExpressionWorldPosition, -900, 900)
obj = g.node(unreal.MaterialExpressionObjectPositionWS, -900, 1000)
height = g.op(unreal.MaterialExpressionSubtract, g.mask(world, False, False, True, -750, 900), g.mask(obj, False, False, True, -750, 1000), -600, 950)
g.link(g.op(unreal.MaterialExpressionSubtract, g.const(1.0, -600, 1050), g.op(unreal.MaterialExpressionDivide, height, g.const(45.0, -600, 1100), -500, 1000), -400, 1000), boots)
dusty = g.lerp(coloured, g.const(DUST, -250, 100), g.op(unreal.MaterialExpressionMultiply, boots, g.const(0.55, -250, 200), -150, 150), 0, 0)
MEL.connect_material_property(dusty, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(nrm, "", unreal.MaterialProperty.MP_NORMAL)
MEL.connect_material_property(g.mask(rgh, True, False, False, -500, 600), "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(mat)
EAL.save_asset(suit_path)

# ---------------------------------------------------------------- re-parent the instances
hull = unreal.load_asset(hull_path)
suit = unreal.load_asset(suit_path)
PANEL = {"MI_TD_Base_Hull": (1.0, 1.0, 0.15), "MI_TD_Base_Tank": (1.0, 1.0, 0.3), "MI_TD_Base_WaterTank": (1.0, 1.0, 0.2),
         "MI_TD_Base_Rib": (1.0, 0.8, 0.5), "MI_TD_Base_Dark": (1.0, 0.7, 0.3), "MI_TD_Base_Accent": (1.0, 0.8, 0.1),
         "MI_TD_Greybox_Hull": (1.0, 1.0, 0.15), "MI_TD_Greybox_Dark": (1.0, 0.7, 0.3), "MI_TD_Plain_White": (1.0, 1.0, 0.1),
         "MI_TD_Plain_Steel": (1.0, 0.5, 0.6), "MI_TD_Base_Regolith": (0.0, 1.4, 0.0), "MI_TD_Base_Pad": (0.0, 0.8, 0.0)}
for name, (panel_amount, dust_amount, metal) in PANEL.items():
    mi = unreal.load_asset(f"{PATH}/{name}")
    MEL.set_material_instance_parent(mi, hull)
    MEL.set_material_instance_scalar_parameter_value(mi, "Panel", panel_amount)
    MEL.set_material_instance_scalar_parameter_value(mi, "Dust", dust_amount)
    MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", metal)
    if name == "MI_TD_Base_Regolith":   # piled regolith takes the crater floor's dusty tone, not a bright orange
        MEL.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(0.30, 0.19, 0.12, 1.0))
    MEL.update_material_instance(mi)
    EAL.save_asset(f"{PATH}/{name}", only_if_is_dirty=False)
for name in ("MI_TD_Suit_Air", "MI_TD_Suit_Greenhouse", "MI_TD_Suit_Shelter"):
    mi = unreal.load_asset(f"{PATH}/{name}")
    MEL.set_material_instance_parent(mi, suit)
    MEL.update_material_instance(mi)
    EAL.save_asset(f"{PATH}/{name}", only_if_is_dirty=False)
print("hull and suit materials ok")
