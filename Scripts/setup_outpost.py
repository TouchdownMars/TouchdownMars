# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Phase 5: greybox L_TD_Outpost at the level-design.md §3 positions (block meshes, real scale).

Run inside the editor (MCP execute_python_code) after Scripts/setup_test_map.py (needs GM_TD):
    exec(open(r"<ProjectDir>/Scripts/setup_outpost.py", encoding="utf-8").read())
Rebuilds the level from scratch each run. Units are centimetres. The camera looks along +X; "behind" = +X.
Scale standard: crew 180 cm, habitat ~8 m long, greenhouse dome ~6 m wide, shelter ~4 m, fission unit ~60 m
behind the habitat ([DESIGNER VALUE] until a NASA distance is sourced), lander ~40 m back.
"""

import unreal

ROOT = "/Game/TouchdownMars"
LEVEL = f"{ROOT}/Maps/L_TD_Outpost"
MAT = f"{ROOT}/Materials"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
log = []


def material(name, color):
    path = f"{MAT}/{name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi = unreal.load_asset(path)
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    return mi


def block(label, shape, location, scale, mi, rotation=(0, 0, 0)):
    actor = actors.spawn_actor_from_object(unreal.load_asset(f"/Engine/BasicShapes/{shape}"), unreal.Vector(*location),
                                           unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.static_mesh_component.set_material(0, mi)
    actor.set_folder_path("Greybox")
    return actor


if unreal.EditorAssetLibrary.does_asset_exist(LEVEL):
    les.load_level(LEVEL)
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label().startswith("TD_"):
            actors.destroy_actor(actor)
else:
    les.new_level(LEVEL)

ground = material("MI_TD_Greybox_Ground", (0.30, 0.12, 0.05))   # Mars regolith tone (greybox only)
hull = material("MI_TD_Greybox_Hull", (0.55, 0.55, 0.52))
dark = material("MI_TD_Greybox_Dark", (0.05, 0.05, 0.06))

# Lighting: late-afternoon low sun, butterscotch haze (gdd.md §3 Theme 1). Dark at the opening comes from the
# system lights being off until their first cell.
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 3000), unreal.Rotator(pitch=-14, yaw=-35, roll=0))
sun.set_actor_label("TD_Sun")
sun.light_component.set_intensity(3.0)
sun.light_component.set_light_color(unreal.LinearColor(1.0, 0.78, 0.55, 1.0))
sun.light_component.set_editor_property("atmosphere_sun_light", True)
sky = actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
sky.set_actor_label("TD_SkyAtmosphere")
skylight = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
skylight.set_actor_label("TD_SkyLight")
skylight.light_component.set_editor_property("real_time_capture", True)
skylight.light_component.set_intensity(0.6)
fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
fog.set_actor_label("TD_DustHaze")
fog.component.set_editor_property("fog_density", 0.0015)
fog.component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.75, 0.48, 0.28, 1.0))

block("TD_Ground", "Plane", (0, 0, 0), (600, 600, 1), ground)

# Systems (ATDSystemModule: block + system light). AIR lives in the habitat.
for label, system, location, scale in (
        ("TD_Habitat_Air", unreal.TDSystem.AIR, (0, 0, 150), (8, 4, 3)),
        ("TD_Greenhouse", unreal.TDSystem.GREENHOUSE, (100, -1150, 150), (6, 6, 3)),
        ("TD_Shelter", unreal.TDSystem.SHELTER, (100, 1050, 200), (4, 4, 4))):
    module = actors.spawn_actor_from_class(unreal.TDSystemModule, unreal.Vector(*location))
    module.set_actor_label(label)
    module.set_actor_scale3d(unreal.Vector(*scale))
    module.set_editor_property("system", system)
    module.set_folder_path("Systems")

block("TD_HabitatDoor", "Cube", (-205, 0, 110), (0.1, 1.2, 2.2), dark)
block("TD_Antenna", "Cylinder", (150, 250, 400), (0.08, 0.08, 2.0), hull)
block("TD_AntennaDish", "Cone", (150, 250, 520), (0.6, 0.6, 0.3), hull, (60, 180, 0))
block("TD_CargoA", "Cube", (-300, -500, 50), (1, 1, 1), hull)
block("TD_CargoB", "Cube", (-300, -380, 50), (1, 1, 1), hull)
block("TD_Rover", "Cube", (-900, -1600, 60), (3, 2, 1.2), hull)
lander = block("TD_Lander", "Cone", (4000, -1800, 400), (5, 5, 8), hull)
block("TD_LanderLegs", "Cylinder", (4000, -1800, 100), (6, 6, 2), dark)

power = actors.spawn_actor_from_class(unreal.TDPowerUnit, unreal.Vector(6000, 700, 150))
power.set_actor_label("TD_FissionUnit")
power.set_folder_path("Systems")

for index, (label, system) in enumerate((("TD_Crew_FlightEngineer", unreal.TDSystem.AIR),
                                         ("TD_Crew_ScienceOfficer", unreal.TDSystem.GREENHOUSE),
                                         ("TD_Crew_HealthOfficer", unreal.TDSystem.SHELTER))):
    crew = actors.spawn_actor_from_class(unreal.TDCrewMember, unreal.Vector(-480, -420 + index * 420, 90))
    crew.set_actor_label(label)
    crew.set_editor_property("system", system)
    crew.set_editor_property("lander_location", unreal.Vector(3500, -1500 + index * 150, 90))
    crew.set_folder_path("Crew")

rig = actors.spawn_actor_from_class(unreal.TDCameraRig, unreal.Vector(-3600, -200, 2000), unreal.Rotator(pitch=-27, yaw=3, roll=0))
rig.set_actor_label("TD_CameraRig")
cam = rig.get_component_by_class(unreal.CineCameraComponent)
cam.set_editor_property("current_focal_length", 22.0)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_asset(f"{ROOT}/Blueprints/GM_TD").generated_class())
les.save_current_level()
log.append("outpost actors: " + str(len([a for a in actors.get_all_level_actors() if a.get_actor_label().startswith("TD_")])))
print("\n".join(log))
