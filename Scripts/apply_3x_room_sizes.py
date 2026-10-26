import sys
import time
from pathlib import Path

REMOTE_PY_PATH = Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python")
sys.path.append(str(REMOTE_PY_PATH))
import remote_execution

client = remote_execution.RemoteExecution()
client.start()
for _ in range(15):
    time.sleep(0.3)
    if client.remote_nodes:
        break

if not client.remote_nodes:
    print("[ERROR] No running Unreal Editor found with Remote Execution enabled!")
    sys.exit(1)

node = client.remote_nodes[0]
client.open_command_connection(node['node_id'])
print(f"Connected to Unreal Editor (Node: {node['node_id']})")

code = """
import unreal

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary

room_bps = [
    '/Game/Map/BP_Room_Base',
    '/Game/Map/BP_Room_Normal_01',
    '/Game/Map/BP_Room_Normal_02',
    '/Game/Map/BP_Room_Boss',
    '/Game/Map/BP_Room_Shop',
    '/Game/Map/BP_Room_Treasure',
    '/Game/Map/BP_Room_Start',
    '/Game/Map/BP_Room_Secret'
]

print("=== [1/4] Updating Room Blueprints (Sockets, Trigger, Floor Mesh) ===")
for path in room_bps:
    bp = unreal.EditorAssetLibrary.load_asset(path)
    if not bp:
        print(f"  [WARN] Blueprint not found: {path}")
        continue

    print(f"Processing: {path}")
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    for h in handles:
        data = subsystem.k2_find_subobject_data_from_handle(h)
        obj = lib.get_object_for_blueprint(data, bp)
        dname = str(lib.get_display_name(data))

        if not obj:
            continue

        # 1. Update Door Sockets
        if "NorthDoorSocket" in dname:
            obj.set_editor_property("relative_location", unreal.Vector(0.0, 3600.0, 0.0))
            print("  - NorthDoorSocket -> (0, 3600, 0)")
        elif "SouthDoorSocket" in dname:
            obj.set_editor_property("relative_location", unreal.Vector(0.0, -3600.0, 0.0))
            print("  - SouthDoorSocket -> (0, -3600, 0)")
        elif "EastDoorSocket" in dname:
            obj.set_editor_property("relative_location", unreal.Vector(3600.0, 0.0, 0.0))
            print("  - EastDoorSocket -> (3600, 0, 0)")
        elif "WestDoorSocket" in dname:
            obj.set_editor_property("relative_location", unreal.Vector(-3600.0, 0.0, 0.0))
            print("  - WestDoorSocket -> (-3600, 0, 0)")

        # 2. Update Room Bounds Trigger Box Extent
        elif "RoomBoundsTrigger" in dname and isinstance(obj, unreal.BoxComponent):
            obj.set_editor_property("box_extent", unreal.Vector(3000.0, 3000.0, 300.0))
            print("  - RoomBoundsTrigger extent -> (3000, 3000, 300)")

        # 3. Update Floor StaticMesh Scale
        elif ("StaticMesh" in dname or "Floor" in dname) and isinstance(obj, unreal.StaticMeshComponent):
            obj.set_editor_property("relative_scale3d", unreal.Vector(60.0, 60.0, 1.0))
            print(f"  - {dname} (Floor StaticMesh) scale -> (60, 60, 1)")

    # Also ensure CDO properties if direct access applies
    cdo = unreal.get_default_object(bp.generated_class())
    if cdo:
        try:
            trigger = cdo.get_editor_property("room_bounds_trigger")
            if trigger:
                trigger.set_editor_property("box_extent", unreal.Vector(3000.0, 3000.0, 300.0))
        except Exception:
            pass

    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    print(f"  -> Successfully compiled and saved: {path}")

print("\\n=== [2/4] Updating Wall Blueprints (BP_WallBase, BP_DestructibleWall) ===")
wall_bps = [
    ('/Game/Map/BP_WallBase', 'WallMesh'),
    ('/Game/Map/BP_DestructibleWall', 'IntactWallMesh')
]
for path, comp_name in wall_bps:
    bp = unreal.EditorAssetLibrary.load_asset(path)
    if not bp:
        continue
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    for h in handles:
        data = subsystem.k2_find_subobject_data_from_handle(h)
        obj = lib.get_object_for_blueprint(data, bp)
        dname = str(lib.get_display_name(data))
        if obj and isinstance(obj, unreal.StaticMeshComponent) and comp_name in dname:
            obj.set_editor_property("relative_scale3d", unreal.Vector(3.0, 72.0, 24.0))
            print(f"  - {path}: {dname} scale -> (3, 72, 24)")

    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    print(f"  -> Successfully compiled and saved: {path}")

print("\\n=== [3/4] Updating BP_DungeonManager ===")
bp_mgr = unreal.EditorAssetLibrary.load_asset('/Game/Map/BP_DungeonManager')
if bp_mgr:
    cdo = unreal.get_default_object(bp_mgr.generated_class())
    if cdo:
        cdo.set_editor_property("cell_size", 7200.0)
        print("  - BP_DungeonManager CDO cell_size -> 7200.0")
    unreal.BlueprintEditorLibrary.compile_blueprint(bp_mgr)
    unreal.EditorAssetLibrary.save_loaded_asset(bp_mgr)
    print("  -> Successfully compiled and saved BP_DungeonManager")

print("\\n=== [4/4] Verification Check ===")
for test_path in ['/Game/Map/BP_Room_Normal_01', '/Game/Map/BP_WallBase', '/Game/Map/BP_DungeonManager']:
    bp = unreal.EditorAssetLibrary.load_asset(test_path)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0,0,0))
    if actor:
        print(f"Verified Actor: {actor.get_name()}")
        for c in actor.get_components_by_class(unreal.SceneComponent):
            cname = c.get_name()
            if any(k in cname.lower() for k in ['socket', 'trigger', 'mesh', 'bounds']):
                loc = c.get_editor_property('relative_location')
                scale = c.get_editor_property('relative_scale3d')
                print(f"  {cname}: loc={loc}, scale={scale}")
                if isinstance(c, unreal.BoxComponent):
                    print(f"    extent={c.get_editor_property('box_extent')}")
        if isinstance(actor, unreal.FCDungeonManager):
            print(f"  CellSize: {actor.get_editor_property('cell_size')}")
        actor.destroy_actor()

print("\\nALL ASSET UPDATES COMPLETED SUCCESSFULLY!")
"""

res = client.run_command(code)
print("Execution success:", res.get("success"))
for item in res.get("output", []):
    print(item.get("output", ""))

client.stop()
