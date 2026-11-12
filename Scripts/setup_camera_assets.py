import os
import sys
import json
import time
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
UPROJECT_PATH = PROJECT_ROOT / "FC.uproject"
UE_CMD_EXE = Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe")
REMOTE_PY_PATH = Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python")

PAYLOAD_SCRIPT = """
import unreal
import os

OUT_LOG = r"C:/Users/oogg/Documents/Unreal Projects/FC/Intermediate/python_asset_result.txt"

def log_msg(msg):
    with open(OUT_LOG, "a", encoding="utf-8") as f:
        f.write(str(msg) + "\\n")

with open(OUT_LOG, "w", encoding="utf-8") as f:
    f.write("=== SCRIPT START ===\\n")

def create_or_get_asset(asset_name, package_path, asset_class):
    full_path = f"{package_path}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        log_msg(f"Asset exists: {full_path}")
        return unreal.EditorAssetLibrary.load_asset(full_path)
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(asset_name, package_path, asset_class, None)
    log_msg(f"Created asset: {full_path}")
    return asset

package_path = "/Game/Character/Player/CameraData"
if not unreal.EditorAssetLibrary.does_directory_exist(package_path):
    unreal.EditorAssetLibrary.make_directory(package_path)

# Check class availability
cam_cls = unreal.load_class(None, "/Script/FC.FCCameraModeDataAsset")
log_msg(f"unreal.load_class('/Script/FC.FCCameraModeDataAsset'): {cam_cls}")

if not cam_cls:
    log_msg("Searching for FCCameraModeDataAsset in unreal module...")
    matches = [name for name in dir(unreal) if "FC" in name]
    log_msg(f"FC Matches: {matches}")
else:
    # 1. Shoulder View Data Asset
    da_shoulder = create_or_get_asset("DA_CameraMode_Shoulder", package_path, cam_cls)
    if da_shoulder:
        da_shoulder.set_editor_property("target_arm_length", 400.0)
        da_shoulder.set_editor_property("socket_offset", unreal.Vector(0.0, 50.0, 50.0))
        da_shoulder.set_editor_property("target_offset", unreal.Vector(0.0, 0.0, 0.0))
        da_shoulder.set_editor_property("field_of_view", 90.0)
        da_shoulder.set_editor_property("b_do_collision_test", True)
        da_shoulder.set_editor_property("b_enable_camera_lag", True)
        da_shoulder.set_editor_property("camera_lag_speed", 15.0)
        da_shoulder.set_editor_property("b_enable_camera_rotation_lag", False)
        da_shoulder.set_editor_property("camera_rotation_lag_speed", 10.0)
        da_shoulder.set_editor_property("b_use_controller_rotation_yaw", False)
        da_shoulder.set_editor_property("b_orient_rotation_to_movement", True)
        da_shoulder.set_editor_property("b_hide_character_mesh", False)
        da_shoulder.set_editor_property("blend_time", 0.35)
        unreal.EditorAssetLibrary.save_loaded_asset(da_shoulder)
        log_msg("Successfully configured and saved DA_CameraMode_Shoulder")

    # 2. First Person Data Asset
    da_first_person = create_or_get_asset("DA_CameraMode_FirstPerson", package_path, cam_cls)
    if da_first_person:
        da_first_person.set_editor_property("target_arm_length", 0.0)
        da_first_person.set_editor_property("socket_offset", unreal.Vector(0.0, 0.0, 65.0))
        da_first_person.set_editor_property("target_offset", unreal.Vector(0.0, 0.0, 0.0))
        da_first_person.set_editor_property("field_of_view", 90.0)
        da_first_person.set_editor_property("b_do_collision_test", False)
        da_first_person.set_editor_property("b_enable_camera_lag", False)
        da_first_person.set_editor_property("camera_lag_speed", 10.0)
        da_first_person.set_editor_property("b_enable_camera_rotation_lag", False)
        da_first_person.set_editor_property("camera_rotation_lag_speed", 10.0)
        da_first_person.set_editor_property("b_use_controller_rotation_yaw", True)
        da_first_person.set_editor_property("b_orient_rotation_to_movement", False)
        da_first_person.set_editor_property("b_hide_character_mesh", True)
        da_first_person.set_editor_property("blend_time", 0.3)
        unreal.EditorAssetLibrary.save_loaded_asset(da_first_person)
        log_msg("Successfully configured and saved DA_CameraMode_FirstPerson")

log_msg("=== SCRIPT END ===")
"""

def try_remote():
    try:
        sys.path.append(str(REMOTE_PY_PATH))
        import remote_execution
        client = remote_execution.RemoteExecution()
        client.start()
        time.sleep(1.0)
        nodes = client.remote_nodes
        if not nodes:
            print("[INFO] No remote nodes found.")
            client.stop()
            return False

        target_node = nodes[0]
        client.open_command_connection(target_node.get("node_id"))
        print(f"[INFO] Connected to remote node: {target_node.get('node_id')}")
        res = client.run_command(PAYLOAD_SCRIPT)
        print("[INFO] Remote execution output:")
        for entry in res.get("output", []):
            print(entry.get("output", ""))
        client.stop()
        return res.get("success", False)
    except Exception as e:
        print(f"[WARN] Remote execution failed: {e}")
        return False

def try_cmdlet():
    print("[INFO] Running via UnrealEditor-Cmd...")
    temp_script = PROJECT_ROOT / "Intermediate" / "temp_create_camera_assets.py"
    temp_script.parent.mkdir(parents=True, exist_ok=True)
    temp_script.write_text(PAYLOAD_SCRIPT, encoding="utf-8")

    cmd = [
        str(UE_CMD_EXE),
        str(UPROJECT_PATH),
        "-run=pythonscript",
        f"-script={temp_script.as_posix()}",
        "-unattended",
        "-nopause",
        "-nosplash",
        "-nullrhi"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    output_log = PROJECT_ROOT / "Intermediate" / "camera_assets_cmdlet.log"
    output_log.write_text(proc.stdout + "\n--- STDERR ---\n" + proc.stderr, encoding="utf-8")
    for line in proc.stdout.splitlines():
        if any(k in line for k in ["Successfully", "Created asset", "Asset exists", "ERROR", "Exception", "FCCameraModeDataAsset"]):
            print("  ", line)
    return proc.returncode == 0

if __name__ == "__main__":
    try_cmdlet()
