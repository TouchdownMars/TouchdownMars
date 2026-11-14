# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Model swaps, round 2 (research_notes/Mars uessue modelleri/model_plan.md section 4.3). Run LAST, after
setup_fps.py, inside the editor (PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/setup_models2.py", encoding="utf-8").read())

- Relay antenna: the grey cone TD_Base_RelayDish becomes the NASA 3D Resources Radome on the relay mast
  (size [DESIGNER VALUE]: 1.5 m tall; the surface relay is a fixed-pointing design, DRA5 addendum Tbl 4-15).
- Rover: the grey box TD_Rover is deleted and its "OpsShow_OffloadRover" tag moves to the NASA SEV, so the
  real rover appears when the crew off-loads it and leaves while it is out on a traverse.
A swap copies every tag (DesignShow_, OpsShow_, Inspect_) to the new actor and removes them from the
placeholder, then hides it; otherwise the mission scene would show the grey shape again.
"""

import math

import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if les.get_current_level().get_outer().get_name() != "L_TD_Outpost":
    les.load_level("/Game/TouchdownMars/Maps/L_TD_Outpost")


def by_label(label):
    return next((a for a in eas.get_all_level_actors() if a.get_actor_label() == label), None)


# Re-runnable: each TD_M2_ actor remembers its placeholder (tag M2From_<label>); give the tags back first.
LEGACY = {"TD_M2_Radome": "TD_Base_RelayDish"}
for actor in eas.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith("TD_M2_"):
        tags = [str(t) for t in actor.tags]
        for t in tags:   # extra placeholders this actor hid (tags kept as M2Hide_<label>=tag1;tag2)
            if t.startswith("M2Hide_"):
                hidden_label, _, kept = t[len("M2Hide_"):].partition("=")
                extra = by_label(hidden_label)
                if extra:
                    extra.tags = [unreal.Name(k) for k in kept.split(";") if k]
                    extra.set_actor_hidden_in_game(False)
                    extra.set_actor_enable_collision(True)
                    extra.set_is_temporarily_hidden_in_editor(False)
        tags = [t for t in tags if not t.startswith("M2Hide_")]
        source = next((t[len("M2From_"):] for t in tags if t.startswith("M2From_")), LEGACY.get(label))
        old = by_label(source) if source else None
        if old:
            old.tags = [unreal.Name(t) for t in tags if not t.startswith("M2From_")]
            old.set_actor_hidden_in_game(False)
            old.set_actor_enable_collision(True)
            old.set_is_temporarily_hidden_in_editor(False)
        eas.destroy_actor(actor)
log = []


def swap(placeholder_label, mesh_path, label, location, scale, yaw=0.0):
    old = by_label(placeholder_label)
    mesh = unreal.load_asset(mesh_path)
    if not old or not mesh:
        log.append(f"skip {label}: placeholder {bool(old)}, mesh {bool(mesh)}")
        return None
    new = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0))
    new.set_actor_label(label)
    new.set_folder_path("Models2")
    comp = new.static_mesh_component
    comp.set_static_mesh(mesh)
    new.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    new.tags = list(old.tags) + [unreal.Name(f"M2From_{placeholder_label}")]
    old.tags = []
    old.set_actor_hidden_in_game(True)
    old.set_actor_enable_collision(False)
    old.set_is_temporarily_hidden_in_editor(True)
    log.append(f"{placeholder_label} -> {label}, tags {[str(t) for t in new.tags]}")
    return new


# Relay: radome base on the mast top (mast spans 0..650 cm).
mast = by_label("TD_Base_RelayMast")
top = mast.get_actor_location().z + mast.get_actor_bounds(False)[1].z if mast else 650.0
swap("TD_Base_RelayDish", "/Game/TouchdownMars/NASA/Radome/Radome_static/StaticMeshes/Radome_static",
     "TD_M2_Radome", (900.0, 1450.0, top), 150.0 / 2200.0)

TRIPO = "/Game/TouchdownMars/Tripo/{0}/TD_Tripo_{0}/StaticMeshes/TD_Tripo_{0}"


def fit(placeholder_label, name, label, mode="vertical", sink=0.0):
    """Swap a placeholder for a Tripo mesh fitted to the placeholder's own size.
    vertical: footprint from the placeholder bounds, from the ground (Z >= 0 pad) to its top.
    tube: a lying cylinder; the mesh's long X axis follows the cylinder axis, diameter kept round."""
    old = by_label(placeholder_label)
    mesh = unreal.load_asset(TRIPO.format(name))
    if not old or not mesh:
        log.append(f"skip {label}: placeholder {bool(old)}, mesh {bool(mesh)}")
        return None
    box = mesh.get_bounding_box()
    msize = box.max - box.min
    mcx, mcy = (box.max.x + box.min.x) * 0.5, (box.max.y + box.min.y) * 0.5
    origin, ext = old.get_actor_bounds(False)
    if mode == "tube":
        scale3 = old.get_actor_scale3d()
        axis = old.get_actor_rotation().get_up_vector()
        length = 100.0 * scale3.z
        diameter = 100.0 * scale3.x
        yaw = math.degrees(math.atan2(axis.y, axis.x))
        s_len = length / msize.x
        s_round = diameter / msize.y
        tube_centre_z = (box.max.z - msize.y * 0.5) * s_round      # tube top minus half its width
        z = origin.z - tube_centre_z
        new = swap(placeholder_label, TRIPO.format(name), label, (origin.x, origin.y, z), 1.0, yaw)
        if new:
            new.set_actor_scale3d(unreal.Vector(s_len, s_round, s_round))
            ox, oy, r = -mcx * s_len, -mcy * s_round, math.radians(yaw)
            dx, dy = ox * math.cos(r) - oy * math.sin(r), ox * math.sin(r) + oy * math.cos(r)
            new.set_actor_location(unreal.Vector(origin.x + dx, origin.y + dy, z), False, False)
        return new
    bottom = max(origin.z - ext.z, 0.0) - sink
    height = origin.z + ext.z - bottom
    sx, sy, sz = 2.0 * ext.x / msize.x, 2.0 * ext.y / msize.y, height / msize.z
    new = swap(placeholder_label, TRIPO.format(name), label, (origin.x - mcx * sx, origin.y - mcy * sy, bottom - box.min.z * sz), 1.0)
    if new:
        new.set_actor_scale3d(unreal.Vector(sx, sy, sz))
    return new


# Corridors: the six lying cylinders become the insulated tunnel segment.
for corridor in ("Airlock", "ECLSS", "Greenhouse", "Greenhouse2", "Lab", "Shelter"):
    fit(f"TD_Base_Corridor_{corridor}", "CorridorTube", f"TD_M2_Corridor_{corridor}", "tube")

# Light masts: the ten beacon posts (320 cm) become tripod masts; the glowing lamp spheres stay on top.
for beacon in ("Airlock", "HubE", "HubW", "ISRU", "LZ1a", "LZ1b", "LZ2", "Lab", "PowerA", "PowerB"):
    old = by_label(f"TD_Base_Beacon_{beacon}")
    if old:
        o, e = old.get_actor_bounds(False)
        mesh = unreal.load_asset(TRIPO.format("LightMast"))
        s = (2.0 * e.z) / (mesh.get_bounding_box().max.z - mesh.get_bounding_box().min.z)
        swap(f"TD_Base_Beacon_{beacon}", TRIPO.format("LightMast"), f"TD_M2_Mast_{beacon}", (o.x, o.y, o.z - e.z), s)

# Regolith mounds: the shelter cover, the design-room berm and the crew-built berm.
fit("TD_Art_ShelterMound", "ShelterMound", "TD_M2_ShelterMound", sink=20.0)
fit("TD_Base_RegolithBerm", "ShelterMound", "TD_M2_RegolithBerm", sink=20.0)
fit("TD_Ops_CrewBerm", "ShelterMound", "TD_M2_CrewBerm", sink=10.0)

# B2 life support and B3 lab: the squashed HDU copies become the Tripo habitat module (B1 keeps NASA's HDU).
# Height kept from the placeholder, aspect kept from the mesh [DESIGNER VALUE].
for module, yaw in (("TD_Base_ECLSS", 180.0), ("TD_Base_Lab", -90.0)):
    old = by_label(module)
    mesh = unreal.load_asset(TRIPO.format("HabModule"))
    if old and mesh:
        o, e = old.get_actor_bounds(False)
        mb = mesh.get_bounding_box()
        s = 2.0 * e.z / (mb.max.z - mb.min.z)
        swap(module, TRIPO.format("HabModule"), f"TD_M2_{module[8:]}", (o.x, o.y, 0.0), s, yaw)

# Science payload cases next to the cargo lander: the round-1 crate stack becomes the hard-shell payload case
# (about 90 x 60 x 56 cm [DESIGNER VALUE]); the actor and its OpsShow tag stay.
for crate in ("TD_Ops_PayloadCrates0", "TD_Ops_PayloadCrates1"):
    actor = by_label(crate)
    mesh = unreal.load_asset(TRIPO.format("PayloadCase"))
    if actor and mesh:
        actor.static_mesh_component.set_static_mesh(mesh)
        actor.set_actor_scale3d(unreal.Vector(0.9, 0.9, 0.9))
        log.append(f"{crate}: payload case")

# Power junction boxes at both ends of the three cable trays (new, no placeholder): 120 cm tall [DESIGNER VALUE].
for i in range(3):
    tray = by_label(f"TD_Base_CableTray{i}")
    mesh = unreal.load_asset(TRIPO.format("PowerJunctionBox"))
    if not tray or not mesh:
        continue
    yaw = tray.get_actor_rotation().yaw
    half = 50.0 * tray.get_actor_scale3d().x
    centre = tray.get_actor_location()
    for end, sign in (("a", 1.0), ("b", -1.0)):
        x = centre.x + sign * half * math.cos(math.radians(yaw)) + 60.0 * math.sin(math.radians(yaw))
        y = centre.y + sign * half * math.sin(math.radians(yaw)) - 60.0 * math.cos(math.radians(yaw))
        hit = unreal.LandscapeService.get_height_at_location("TD_Terrain", x, y)
        z = hit.height if hit.valid else centre.z
        box = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z - 5.0), unreal.Rotator(pitch=0.0, yaw=yaw + 90.0, roll=0.0))
        box.set_actor_label(f"TD_M2_Junction{i}{end}")
        box.set_folder_path("Models2")
        box.static_mesh_component.set_static_mesh(mesh)
        box.set_actor_scale3d(unreal.Vector(1.2, 1.2, 1.2))
        box.tags = [unreal.Name("Inspect_CableTray")]
        log.append(f"junction box at tray {i} end {end}")

def hide_with(new, label):
    """Hide an extra placeholder that the new model replaces too; its tags move onto the new model."""
    extra = by_label(label)
    if not new or not extra:
        return
    kept = [str(t) for t in extra.tags]
    new.tags = list(new.tags) + [unreal.Name(t) for t in kept if t not in [str(n) for n in new.tags]] + [unreal.Name(f"M2Hide_{label}=" + ";".join(kept))]
    extra.tags = []
    extra.set_actor_hidden_in_game(True)
    extra.set_actor_enable_collision(False)
    extra.set_is_temporarily_hidden_in_editor(True)


def place(placeholder_label, name, label, location, scale3, yaw):
    new = swap(placeholder_label, TRIPO.format(name), label, location, 1.0, yaw)
    if new:
        new.set_actor_scale3d(unreal.Vector(*scale3))
    return new


# Airlock: the 7-part greybox (vessel, two end caps, two ribs) becomes one module. Its long side is the mesh Y
# axis; 6.5 m long, about 3.8 m across [DESIGNER VALUE] (the Common Habitat airlock chamber is 3.15 m, Howard 2021).
airlock = place("TD_Base_Airlock", "AirlockModule", "TD_M2_Airlock", (2025.0, -1050.0, 0.0), (10.0, 6.5, 9.5), -90.0)
for part in ("TD_Base_AirlockCap_1700", "TD_Base_AirlockCap_2350", "TD_Base_Airlock_Rib1", "TD_Base_Airlock_Rib2"):
    hide_with(airlock, part)

# Weather station: the MEDA mast and boom become one mast, 2.6 m tall [DESIGNER VALUE].
mast = by_label("TD_Base_MEDA_Mast")
if mast:
    o, e = mast.get_actor_bounds(False)
    meda = place("TD_Base_MEDA_Mast", "WeatherMast", "TD_M2_WeatherMast", (o.x, o.y, max(o.z - e.z, 0.0)), (2.6, 2.6, 2.6), 0.0)
    hide_with(meda, "TD_Base_MEDA_Boom")

# Water wall: six upright frames of water bags around the shelter, bags facing out (1.4 x 0.6 x 2.4 m [DESIGNER VALUE]).
shelter = by_label("TD_Shelter")
sc = shelter.get_actor_location() if shelter else unreal.Vector(100.0, 1050.0, 0.0)
for i in range(6):
    w = by_label(f"TD_Base_WaterWall{i}")
    if w:
        loc = w.get_actor_location()
        face = math.degrees(math.atan2(sc.y - loc.y, sc.x - loc.x))
        place(f"TD_Base_WaterWall{i}", "WaterWallModule", f"TD_M2_WaterWall{i}", (loc.x, loc.y, 0.0), (1.4, 2.0, 2.73), face + 90.0)

# ISRU: the plant box and its two tanks.
fit("TD_Base_ISRU_Plant", "ISRUPlant", "TD_M2_ISRUPlant")
fit("TD_Base_ISRU_Tank0", "StorageTankH", "TD_M2_ISRUTank0", sink=40.0)
fit("TD_Base_ISRU_Tank1", "StorageTankH", "TD_M2_ISRUTank1", sink=40.0)

# Rover: the NASA SEV takes the off-load tag; the grey box goes.
sev = by_label("TD_Prop_SEV")
box = by_label("TD_Rover")
if sev:
    tags = [str(t) for t in sev.tags]
    if "OpsShow_OffloadRover" not in tags:
        sev.tags = list(sev.tags) + [unreal.Name("OpsShow_OffloadRover")]
    log.append(f"TD_Prop_SEV tags: {[str(t) for t in sev.tags]}")
if box:
    eas.destroy_actor(box)
    log.append("TD_Rover box deleted")

les.save_current_level()
print("\n".join(log))
