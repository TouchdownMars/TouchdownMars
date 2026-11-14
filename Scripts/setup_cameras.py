# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Camera system for L_TD_Outpost: fixed main camera CAM_Main and the Outcome Shot cameras, stage points and
placeholder props (engine basic shapes only). Rows live in Data/Source/outcome_shots.csv (DT_OutcomeShots).
Run in the editor after setup_outpost.py, setup_terrain.py, setup_art.py and scatter_rocks.py (PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/setup_cameras.py", encoding="utf-8").read())
Rerunnable: removes what it placed before (labels CAM_*, Stage_*, TD_Shot_*, TD_PlantRack).
"""

import math

import unreal

ROOT = "/Game/TouchdownMars"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Outpost")
log = []

for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith(("CAM_", "Stage_", "TD_Shot_", "TD_PlantRack")) or label == "TD_CameraRig":
        actors.destroy_actor(actor)  # the old rig had an opening push-in; the main camera is fixed now


def find(label):
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    raise RuntimeError(f"{label} not found in L_TD_Outpost")


def ground(x, y):
    sample = unreal.LandscapeService.get_height_at_location("TD_Terrain", x, y)
    return sample.height if sample.valid else 0.0


def look_at(start, target):
    return unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*start), unreal.Vector(*target))


def camera(tag, location, target, focal_mm):
    """CineCameraActor with a 16:9 filmback (23.76 x 13.365 mm), no depth of field, no letterbox."""
    rotation = look_at(location, target)
    cam = actors.spawn_actor_from_class(unreal.CineCameraActor, unreal.Vector(*location), rotation)
    cam.set_actor_label(tag)
    cam.set_folder_path("Cameras")
    cam.set_editor_property("tags", [unreal.Name(tag)])
    comp = cam.get_cine_camera_component()
    filmback = comp.get_editor_property("filmback")
    filmback.set_editor_property("sensor_width", 23.76)
    filmback.set_editor_property("sensor_height", 13.365)
    comp.set_editor_property("filmback", filmback)
    comp.set_editor_property("current_focal_length", focal_mm)
    focus = comp.get_editor_property("focus_settings")
    focus.set_editor_property("focus_method", unreal.CameraFocusMethod.DISABLE)
    comp.set_editor_property("focus_settings", focus)
    comp.set_editor_property("constrain_aspect_ratio", False)
    hfov = math.degrees(2 * math.atan(23.76 / (2 * focal_mm)))
    log.append(f"{tag}: at {tuple(round(v) for v in location)} looking at {target}, focal {focal_mm} mm (hFOV {hfov:.1f} deg)")
    return cam


def stage(tag, x, y, yaw, anim=None):
    """TargetPoint the crew member stands on during the shot; anim "Walk" or "Work" picks the suit animation."""
    point = actors.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(x, y, ground(x, y) + 1), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    point.set_actor_label(tag)
    point.set_folder_path("Cameras/Stage")
    point.set_editor_property("tags", [unreal.Name(tag)] + ([unreal.Name(f"Anim_{anim}")] if anim else []))
    return point


# Crew homes: each crew member stands in front of their system, on the camera side, outside every mesh.
CREW_HOMES = {
    "TD_Crew_FlightEngineer": (-650, -420),   # AIR, habitat corner
    "TD_Crew_ScienceOfficer": (-150, -1500),  # GREENHOUSE, beside the plant racks
    "TD_Crew_HealthOfficer": (-350, 900),     # SHELTER, near the shelter door
}
for label, (x, y) in CREW_HOMES.items():
    crew = find(label)
    crew.set_actor_location(unreal.Vector(x, y, ground(x, y) + 1), False, False)
    crew.set_actor_rotation(unreal.Rotator(pitch=0, yaw=225, roll=0), False)  # facing the main camera side

# A) Main camera: elevated three-quarter view, pitch -38, hFOV 32, looking along +X across the line of systems.
#    The habitat turns to yaw 135, so the camera sits at 45 deg to the habitat's long axis while GREENHOUSE,
#    AIR and SHELTER read left to right. Position solved (scratch frame2.py) so every system and crew member sits
#    in the top two thirds of a 16:9 frame AND clear of the HUD's CAPCOM box (top right), phase strip (top left)
#    and corner buttons (right, above the 2D panel).
habitat = find("TD_Art_Habitat")
habitat.set_actor_rotation(unreal.Rotator(pitch=0, yaw=135, roll=0), False)
main = camera("CAM_Main", (-7906, -126, 6093), (0, 0, 0), 41.4)
main.set_actor_rotation(unreal.Rotator(pitch=-38, yaw=0, roll=0), False)

# Antenna for S05: the greybox mast and dish were hidden on the habitat roof; stand them on the ground.
mast = find("TD_Antenna")
mast.set_actor_location(unreal.Vector(380, 520, 150), False, False)
mast.set_actor_scale3d(unreal.Vector(0.1, 0.1, 3.0))
mast.set_actor_hidden_in_game(False)
mast.set_is_temporarily_hidden_in_editor(False)
dish = find("TD_AntennaDish")
dish.set_actor_location(unreal.Vector(380, 520, 330), False, False)
dish.set_actor_scale3d(unreal.Vector(1.3, 1.3, 0.45))
dish.set_actor_rotation(unreal.Rotator(pitch=55, yaw=200, roll=0), False)
dish.set_actor_hidden_in_game(False)
dish.set_is_temporarily_hidden_in_editor(False)

def framed(tag, subject, yaw, pitch, dist, focal, screen_x=0.5, screen_y=0.42):
    """Camera at (pitch, yaw) placed so `subject` lands at (screen_x, screen_y) of the frame (0..1 from top-left).
    The HUD stays on screen during a shot (CAPCOM top right, 2D panel bottom third), so subjects sit upper-middle."""
    p, y = math.radians(pitch), math.radians(yaw)
    F = (math.cos(p) * math.cos(y), math.cos(p) * math.sin(y), math.sin(p))
    R = (-math.sin(y), math.cos(y), 0.0)
    U = (-math.sin(p) * math.cos(y), -math.sin(p) * math.sin(y), math.cos(p))
    tx = 23.76 / (2 * focal)
    ty = 13.365 / (2 * focal)
    nx, ny = 2 * screen_x - 1, 1 - 2 * screen_y
    ray = [F[i] + R[i] * nx * tx + U[i] * ny * ty for i in range(3)]
    n = math.sqrt(sum(c * c for c in ray))
    loc = tuple(subject[i] - ray[i] / n * dist for i in range(3))
    cam = camera(tag, loc, subject, focal)
    cam.set_actor_rotation(unreal.Rotator(pitch=pitch, yaw=yaw, roll=0), False)
    return cam


# B) Outcome Shot cameras (Actor Tag = CameraTag in DT_OutcomeShots).
# S02_EVA: over the Flight Engineer's shoulder at the cargo lander, terrain behind.
fe_x, fe_y = 2450, -600
fe_yaw = math.degrees(math.atan2(-1800 - fe_y, 4000 - fe_x))
stage("Stage_CAM_S02_EVA_Air", fe_x, fe_y, fe_yaw, "Work")
framed("CAM_S02_EVA", (fe_x, fe_y, ground(fe_x, fe_y) + 150), fe_yaw - 16, -7, 900, 24.0, screen_x=0.32, screen_y=0.42)

# S03_STORM: low angle at the SHELTER door; on choice A the crew stands at the door (going in).
framed("CAM_S03_STORM", (-360, 1090, 110), 22, -6, 1200, 24.0, screen_y=0.40)
stage("Stage_CAM_S03_STORM_Shelter_A", -245, 1100, 0, "Walk")      # first one already at the door
stage("Stage_CAM_S03_STORM_Air_A", -420, 990, 30, "Walk")
stage("Stage_CAM_S03_STORM_Greenhouse_A", -560, 1190, 10, "Walk")

# S04_GREEN: close on the plant racks in front of the greenhouse dome.
rack = actors.spawn_actor_from_class(unreal.TDPlantRack, unreal.Vector(-320, -1240, 0), unreal.Rotator(pitch=0, yaw=38, roll=0))
rack.set_actor_label("TD_PlantRack")
rack.set_folder_path("Cameras/Props")
rack.get_component_by_class(unreal.PointLightComponent).set_intensity(250.0)  # grow light: subtle, no overexposure
framed("CAM_S04_GREEN", (-320, -1240, 105), 38, -16, 600, 24.0, screen_x=0.42, screen_y=0.45)

# S05_COMMS: the dish, with a message icon that travels toward the sky and pauses (no reply is ever shown).
icon = actors.spawn_actor_from_class(unreal.TDShotProp, unreal.Vector(370, 515, 360))
icon.set_actor_label("TD_Shot_MessageIcon")
icon.set_folder_path("Cameras/Props")
icon.set_actor_scale3d(unreal.Vector(0.22, 0.22, 0.22))
icon.set_editor_property("shot_ids", [unreal.Name("S05_COMMS")])
icon.set_editor_property("travel_offset", unreal.Vector(250, 120, 420))
icon.set_editor_property("color", unreal.LinearColor(1.0, 0.75, 0.2, 1.0))
framed("CAM_S05_COMMS", (380, 520, 330), -4, 12, 950, 24.0, screen_y=0.45)

# S06_FAULT: a crew member (Flight Engineer, choice A) outside reconnecting a cable at the power unit.
unit = find("TD_FissionUnit").get_actor_location()
cx, cy = unit.x - 160, unit.y - 90
stage("Stage_CAM_S06_FAULT_Air_A", cx, cy, math.degrees(math.atan2(unit.y - cy, unit.x - cx)), "Work")
cable = actors.spawn_actor_from_class(unreal.TDShotProp, unreal.Vector((unit.x + cx) / 2, (unit.y + cy) / 2, ground(cx, cy) + 8))
cable.set_actor_label("TD_Shot_Cable")
cable.set_folder_path("Cameras/Props")
cable.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cylinder"))
length = math.hypot(unit.x - cx, unit.y - cy)
cable.set_actor_rotation(unreal.Rotator(pitch=90, yaw=math.degrees(math.atan2(unit.y - cy, unit.x - cx)), roll=0), False)
cable.set_actor_scale3d(unreal.Vector(0.04, 0.04, length / 100.0))
cable.set_editor_property("shot_ids", [unreal.Name("S06_FAULT")])
cable.set_editor_property("color", unreal.LinearColor(0.08, 0.08, 0.09, 1.0))
framed("CAM_S06_FAULT", ((unit.x + cx) / 2, (unit.y + cy) / 2, ground(cx, cy) + 120), -35, -12, 980, 24.0, screen_y=0.38)

# S07_DEPART: wide shot of the base left safe and the ascent vehicle (TD_Lander).
framed("CAM_S07_DEPART", (1100, -600, 250), 35, -14, 6000, 24.0, screen_y=0.36)

# The greybox rover sat in the bottom third of CAM_Main (reserved for the 2D panel); park it by the lander.
rover = find("TD_Rover")
rover.set_actor_location(unreal.Vector(3300, -2550, ground(3300, -2550) + 60), False, False)

les.save_current_level()
print("\n".join(log))
