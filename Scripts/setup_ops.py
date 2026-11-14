"""Base operations scene (gdd.md Main Mechanics 14): parts of the base that appear only when the crew has done
the task. Actors carry OpsShow_<TaskId> (visible once the task is done) or OpsHide_<TaskId> (visible until it is
done); UTDMissionSubsystem::RefreshOpsScene combines this with the design tags (DesignShow_<OptionId>_<N>).

- The rover stays on the cargo lander deck until "Off-load the rover" (NASA HEOMD-415, sol 5).
- The weather mast stands only after "Stand up the weather mast".
- Payload crates sit next to the cargo lander after "Off-load the science payload".
- A regolith mound grows on the shelter after "Pile regolith on the shelter".
- Survey stakes mark the clay site after "Survey for water-bearing clays".

Run after setup_base.py (editor, PIE stopped), or headless:
    UnrealEditor-Cmd.exe TouchdownMars.uproject -run=pythonscript -script=<abs path>/Scripts/setup_ops.py -unattended
Rerunnable: removes TD_Ops_* and every Ops* tag first. Positions are [DESIGNER VALUE] layout choices.
"""

import unreal

ROOT = "/Game/TouchdownMars"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Outpost")
log = []

by_label = {}
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith("TD_Ops_"):
        actors.destroy_actor(actor)
        continue
    tags = [t for t in actor.get_editor_property("tags") if not str(t).startswith(("OpsShow_", "OpsHide_"))]
    actor.set_editor_property("tags", tags)
    by_label[label] = actor


def tag(label, *names):
    actor = by_label.get(label)
    if not actor:
        log.append(f"missing {label}")
        return
    actor.set_editor_property("tags", list(actor.get_editor_property("tags")) + [unreal.Name(n) for n in names])


def ground(x, y):
    try:
        sample = unreal.LandscapeService.get_height_at_location("TD_Terrain", x, y)
        if sample.valid:
            return sample.height
    except Exception:
        pass
    return 0.0


def spawn(label, mesh, location, scale=(1, 1, 1), mi=None, yaw=0.0, tags=(), inspect=None):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_label(f"TD_Ops_{label}")
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if mi:
        actor.static_mesh_component.set_material(0, mi)
    actor.set_folder_path("Ops")
    names = [unreal.Name(t) for t in tags]
    if inspect:
        names.append(unreal.Name(inspect))
    actor.set_editor_property("tags", names)
    return actor


def sit(actor):
    origin, extent = actor.get_actor_bounds(False)
    loc = actor.get_actor_location()
    lift = ground(loc.x, loc.y) - (origin.z - extent.z)
    actor.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + lift), False, False)
    return actor


SPHERE = unreal.load_asset("/Engine/BasicShapes/Sphere")
CYL = unreal.load_asset("/Engine/BasicShapes/Cylinder")
CRATES = unreal.load_asset(f"{ROOT}/Tripo/CargoCrates/TD_Tripo_CargoCrates/StaticMeshes/TD_Tripo_CargoCrates")
REGOLITH = unreal.load_asset(f"{ROOT}/Materials/MI_TD_Base_Regolith")
RIB = unreal.load_asset(f"{ROOT}/Materials/MI_TD_Base_Rib")
ACCENT = unreal.load_asset(f"{ROOT}/Materials/MI_TD_Base_Accent")

# Existing parts that the crew must deploy first.
tag("TD_Rover", "OpsShow_OffloadRover")
tag("TD_Base_MEDA_Mast", "OpsShow_DeployMeda")
tag("TD_Base_MEDA_Boom", "OpsShow_DeployMeda")

# Science payload crates next to the cargo lander after the off-load.
for i, (x, y, yaw) in enumerate(((5250, -2780, 15), (5480, -2900, -20))):
    sit(spawn(f"PayloadCrates{i}", CRATES, (x, y, ground(x, y)), (1.0, 1.0, 1.0), None, yaw, ("OpsShow_OffloadPayload",), "Inspect_Spares"))

# Regolith piled on the shelter by the crew (any shelter design).
spawn("CrewBerm", SPHERE, (-260, 1560, ground(-260, 1560) - 90), (5.2, 5.2, 2.6), REGOLITH, 0, ("OpsShow_BuildBerm",), "Inspect_Shelter")

# Survey stakes with orange tops around the clay site.
for i, (x, y) in enumerate(((-1950, 2600), (-1650, 2600), (-1950, 2850), (-1650, 2850))):
    spawn(f"SurveyStake{i}", CYL, (x, y, ground(x, y) + 60), (0.06, 0.06, 1.2), RIB, 0, ("OpsShow_SurveyClays",))
    spawn(f"SurveyFlag{i}", SPHERE, (x, y, ground(x, y) + 125), (0.16, 0.16, 0.16), ACCENT, 0, ("OpsShow_SurveyClays",))

count = len([a for a in actors.get_all_level_actors() if a.get_actor_label().startswith("TD_Ops_")])
log.append(f"ops actors: {count}")
les.save_current_level()
print("\n".join(log))
unreal.log("[setup_ops] " + " ".join(log))
