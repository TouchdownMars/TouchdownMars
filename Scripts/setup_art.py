# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Phase 11: dress L_TD_Outpost with the NASA 3D Resources meshes (plain team materials, no insignia).
Run after setup_outpost.py, setup_terrain.py, import_nasa_models.py (editor, PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/setup_art.py", encoding="utf-8").read())
The system modules keep their colored lights; their greybox blocks are hidden behind real hardware shapes.
"""

import unreal

ROOT = "/Game/TouchdownMars"
NASA = f"{ROOT}/NASA"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Outpost")
log = []

hab_a = unreal.load_asset(f"{NASA}/HabitatA/Habitat_Demonstration_Unit_part_1/StaticMeshes/Habitat_Demonstration_Unit_part_1")
hab_b = unreal.load_asset(f"{NASA}/HabitatB/Habitat_Demonstration_Unit_part_2/StaticMeshes/Habitat_Demonstration_Unit_part_2")
suit = unreal.load_asset(f"{NASA}/Z2Suit/Z2_Spacesuit/StaticMeshes/Z2_Spacesuit")
white = unreal.load_asset(f"{ROOT}/Materials/MI_TD_Plain_White")
sphere = unreal.load_asset("/Engine/BasicShapes/Sphere")

for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith("TD_Art_"):
        actors.destroy_actor(actor)


def place(label, mesh, location, scale, yaw=0.0, material=None):
    actor = actors.spawn_actor_from_object(mesh, unreal.Vector(*location), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale) if isinstance(scale, (int, float)) else unreal.Vector(*scale))
    actor.set_folder_path("Art")
    if material:
        actor.static_mesh_component.set_material(0, material)
    origin, extent = actor.get_actor_bounds(False)
    here = actor.get_actor_location()
    actor.set_actor_location(unreal.Vector(here.x, here.y, here.z - (origin.z - extent.z)), False, False)  # bottom on the pad (Z = 0)
    log.append(f"{label}: size {extent.x * 2 / 100:.1f} x {extent.y * 2 / 100:.1f} x {extent.z * 2 / 100:.1f} m")
    return actor


# Habitat (AIR): NASA Habitat Demonstration Unit, about 9.5 m across at this scale.
place("TD_Art_Habitat", hab_a, (0, 0, 0), 0.025, yaw=180)
# Shelter: a regolith-covered mound (shielding with regolith and stowed cargo), 7 m wide, with a dark door.
ground = unreal.load_asset(f"{ROOT}/Materials/M_TD_MarsGround")
mound = place("TD_Art_ShelterMound", sphere, (150, 1100, 0), (7, 7, 4.4), material=ground)
here = mound.get_actor_location()
mound.set_actor_location(unreal.Vector(here.x, here.y, -40), False, False)
place("TD_Art_ShelterDoor", unreal.load_asset("/Engine/BasicShapes/Cube"), (-200, 1100, 0), (0.6, 1.4, 2.0),
      material=unreal.load_asset(f"{ROOT}/Materials/MI_TD_Greybox_Dark"))
# Greenhouse dome: half-buried sphere, 6 m wide (scale: 1 unit = 1 cm).
dome = place("TD_Art_GreenhouseDome", sphere, (100, -1150, 0), (6, 6, 6), material=white)
here = dome.get_actor_location()
dome.set_actor_location(unreal.Vector(here.x, here.y, 0), False, False)  # centre on the pad: half-buried dome

# Hide the greybox blocks of the system modules; keep their colored lights, lift them over the new shapes.
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label in ("TD_Habitat_Air", "TD_Greenhouse", "TD_Shelter"):
        actor.get_component_by_class(unreal.StaticMeshComponent).set_editor_property("visible", False)
        actor.set_actor_scale3d(unreal.Vector(1, 1, 1))
        here = actor.get_actor_location()
        actor.set_actor_location(unreal.Vector(here.x, here.y, 450), False, False)
        light = actor.get_component_by_class(unreal.PointLightComponent)
        light.set_editor_property("attenuation_radius", 1400.0)
    if label in ("TD_HabitatDoor", "TD_Antenna", "TD_AntennaDish"):
        actor.set_actor_hidden_in_game(True)
        actor.set_is_temporarily_hidden_in_editor(True)

# Thinner energy lines for the real-scale scene.
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() == "TD_FissionUnit":
        actor.set_editor_property("thickness_per_cell", 0.025)
        actor.set_editor_property("min_thickness", 0.015)

# Crew: NASA Z2 suit (closed visor) instead of the cylinder.
for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith("TD_Crew_"):
        body = actor.get_component_by_class(unreal.StaticMeshComponent)
        body.set_static_mesh(suit)
        body.set_material(0, white)
        actor.set_actor_scale3d(unreal.Vector(1, 1, 1))
        body.set_relative_scale3d(unreal.Vector(1, 1, 1))
        origin, extent = actor.get_actor_bounds(False)
        here = actor.get_actor_location()
        actor.set_actor_location(unreal.Vector(here.x, here.y, here.z - (origin.z - extent.z)), False, False)
        actor.set_actor_rotation(unreal.Rotator(pitch=0, yaw=180, roll=0), False)

les.save_current_level()
print("\n".join(log))
