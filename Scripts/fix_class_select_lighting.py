#!/usr/bin/env python3
"""
Fix Lv_ClassSelect Lighting, Sky, Fog, and Studio Environment
Resolves black screen by:
1. Setting DirectionalLight pitch to -35.0 (pointing downwards, sun high in sky).
2. Spawning ExponentialHeightFog (eliminates lower hemisphere black void).
3. Spawning VolumetricCloud (provides volumetric sky clouds).
4. Spawning Floor_Studio podium (provides surface for light reflections and character standing).
5. Configuring SkyLight LowerHemisphereIsBlack=False.
6. Disabling PhysicalCameraExposure in PostProcessVolume.
"""

import os
import sys
import time
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:/Users/oogg/Documents/Unreal Projects/FC")
UPROJECT_PATH = PROJECT_ROOT / "FC.uproject"
UE_CMD_EXE = Path(r"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe")
REMOTE_PY_PATH = Path(r"C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python")

IN_ENGINE_SCRIPT = """
import unreal

level_path = "/Game/Level/ClassSelect/Lv_ClassSelect"
print(f"=== [FIX] Loading Level: {level_path} ===")

loaded = unreal.EditorLevelLibrary.load_level(level_path)
if not loaded:
    print(f"[ERROR] Failed to load level: {level_path}")
    raise RuntimeError("Failed to load level")

actors = unreal.EditorLevelLibrary.get_all_level_actors()
actor_map = {a.get_actor_label(): a for a in actors}

# 1. Fix DirectionalLight (Key Light)
sun = actor_map.get("DirectionalLight_Key")
if not sun:
    for a in actors:
        if isinstance(a, unreal.DirectionalLight):
            sun = a
            break

sun_rot = unreal.Rotator()
sun_rot.pitch = -35.0
sun_rot.yaw = 45.0
sun_rot.roll = 0.0

if not sun:
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0, 0, 500),
        sun_rot
    )
    if sun:
        sun.set_actor_label("DirectionalLight_Key")

if sun:
    sun.set_actor_location(unreal.Vector(0, 0, 500), False, False)
    sun.set_actor_rotation(sun_rot, False)
    comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if comp:
        comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        comp.set_editor_property("atmosphere_sun_light", True)
        comp.set_editor_property("atmosphere_sun_light_index", 0)
        comp.set_editor_property("intensity", 6.0) # Standard daylight lux
        comp.set_editor_property("cast_shadows", True)
        comp.set_editor_property("light_color", unreal.Color(255, 255, 255, 255))
    print(f"  [OK] DirectionalLight configured: Pitch={sun.get_actor_rotation().pitch}, Intensity=6.0, SunLight=True")

# 2. Fix SkyAtmosphere
sky_atm = actor_map.get("SkyAtmosphere_ClassSelect")
if not sky_atm:
    for a in actors:
        if isinstance(a, unreal.SkyAtmosphere):
            sky_atm = a
            break

zero_rot = unreal.Rotator()
zero_rot.pitch = 0.0
zero_rot.yaw = 0.0
zero_rot.roll = 0.0

if not sky_atm:
    sky_atm = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyAtmosphere,
        unreal.Vector(0, 0, -6000),
        zero_rot
    )
    if sky_atm:
        sky_atm.set_actor_label("SkyAtmosphere_ClassSelect")
else:
    sky_atm.set_actor_location(unreal.Vector(0, 0, -6000), False, False)
print("  [OK] SkyAtmosphere verified at (0, 0, -6000)")

# 3. Fix SkyLight (Studio Light)
skylight = actor_map.get("SkyLight_Studio")
if not skylight:
    for a in actors:
        if isinstance(a, unreal.SkyLight):
            skylight = a
            break

if not skylight:
    skylight = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(0, 0, 600),
        zero_rot
    )
    if skylight:
        skylight.set_actor_label("SkyLight_Studio")

if skylight:
    skylight.set_actor_location(unreal.Vector(0, 0, 600), False, False)
    comp = skylight.get_component_by_class(unreal.SkyLightComponent)
    if comp:
        comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        comp.set_editor_property("real_time_capture", True)
        comp.set_editor_property("intensity", 1.0)
        try:
            comp.set_editor_property("lower_hemisphere_is_black", False)
        except Exception as e:
            print("  [WARN] lower_hemisphere_is_black set error:", e)
    print("  [OK] SkyLight configured: LowerHemisphereIsBlack=False, RealTimeCapture=True")

# 4. Spawn / Configure ExponentialHeightFog (Removes lower hemisphere black void)
fog = actor_map.get("ExponentialHeightFog_Studio")
if not fog:
    for a in actors:
        if isinstance(a, unreal.ExponentialHeightFog):
            fog = a
            break

if not fog:
    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.ExponentialHeightFog,
        unreal.Vector(0, 0, 0),
        zero_rot
    )
    if fog:
        fog.set_actor_label("ExponentialHeightFog_Studio")
        print("  [OK] Spawned ExponentialHeightFog_Studio")

if fog:
    comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if comp:
        comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        comp.set_editor_property("fog_density", 0.02)
        try:
            comp.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
            comp.set_editor_property("directional_inscattering_color", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
        except Exception:
            pass
    print("  [OK] ExponentialHeightFog configured")

# 5. Spawn / Configure VolumetricCloud
cloud = actor_map.get("VolumetricCloud_Studio")
if not cloud:
    for a in actors:
        if isinstance(a, unreal.VolumetricCloud):
            cloud = a
            break

if not cloud:
    cloud = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.VolumetricCloud,
        unreal.Vector(0, 0, 700),
        zero_rot
    )
    if cloud:
        cloud.set_actor_label("VolumetricCloud_Studio")
        print("  [OK] Spawned VolumetricCloud_Studio")

# 6. Spawn / Configure Floor/Podium for Character
floor = actor_map.get("Floor_Studio")
cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
mat_grid = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
if not mat_grid:
    mat_grid = unreal.EditorAssetLibrary.load_asset("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")

if not floor:
    for a in actors:
        if a.get_actor_label() == "Floor_Studio" or (isinstance(a, unreal.StaticMeshActor) and "Floor" in a.get_actor_label()):
            floor = a
            break

if not floor:
    floor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(0, 0, -10),
        zero_rot
    )
    if floor:
        floor.set_actor_label("Floor_Studio")
        sm = floor.static_mesh_component
        if cube_mesh:
            sm.set_static_mesh(cube_mesh)
        if mat_grid:
            sm.set_material(0, mat_grid)
        floor.set_actor_scale3d(unreal.Vector(15.0, 15.0, 0.2))
        print("  [OK] Spawned Floor_Studio circular podium (15m x 15m)")
else:
    floor.set_actor_location(unreal.Vector(0, 0, -10), False, False)
    floor.set_actor_scale3d(unreal.Vector(15.0, 15.0, 0.2))
    print("  [OK] Floor_Studio configured")

# 7. Configure PostProcessVolume
ppv = actor_map.get("PostProcessVolume_Unbound")
if not ppv:
    for a in actors:
        if isinstance(a, unreal.PostProcessVolume):
            ppv = a
            break

if not ppv:
    ppv = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PostProcessVolume,
        unreal.Vector(0, 0, 0),
        zero_rot
    )
    if ppv:
        ppv.set_actor_label("PostProcessVolume_Unbound")

if ppv:
    ppv.set_editor_property("unbound", True)
    ppv.set_editor_property("enabled", True)
    s = ppv.get_editor_property("settings")
    try:
        s.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
        s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
        s.set_editor_property("auto_exposure_min_brightness", 0.05)
        s.set_editor_property("auto_exposure_max_brightness", 8.0)
        s.set_editor_property("auto_exposure_bias", 0.0)
        s.set_editor_property("dynamic_global_illumination_method", unreal.DynamicGlobalIlluminationMethod.LUMEN)
        s.set_editor_property("reflection_method", unreal.ReflectionMethod.LUMEN)
    except Exception as e:
        print("  [WARN] PP settings update error:", e)
    print("  [OK] PostProcessVolume configured: PhysicalCameraExposure=False, Lumen Enabled")

# 8. Configure Studio CameraActor
cam = actor_map.get("CameraActor_StudioView")
cam_rot = unreal.Rotator()
cam_rot.pitch = -5.0
cam_rot.yaw = 0.0
cam_rot.roll = 0.0

if not cam:
    for a in actors:
        if isinstance(a, unreal.CameraActor):
            cam = a
            break

if not cam:
    cam = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.CameraActor,
        unreal.Vector(-600, 0, 130),
        cam_rot
    )
    if cam:
        cam.set_actor_label("CameraActor_StudioView")
else:
    cam.set_actor_location(unreal.Vector(-600, 0, 130), False, False)
    cam.set_actor_rotation(cam_rot, False)

if cam:
    comp = cam.camera_component
    if comp:
        comp.set_editor_property("field_of_view", 60.0)
    print("  [OK] CameraActor_StudioView positioned at (-600, 0, 130) facing podium")

# 9. Save Level
saved = unreal.EditorLevelLibrary.save_current_level()
print(f"=== [SUCCESS] Level Saved: {saved} ===")
"""

def execute():
    # 1. Try remote execution first
    if REMOTE_PY_PATH.exists():
        sys.path.append(str(REMOTE_PY_PATH))
        try:
            import remote_execution
            client = remote_execution.RemoteExecution()
            client.start()
            for _ in range(8):
                time.sleep(0.2)
                if client.remote_nodes:
                    break

            if client.remote_nodes:
                node = client.remote_nodes[0]
                client.open_command_connection(node['node_id'])
                print(f"[REMOTE] Connected to Live Unreal Editor (Node: {node['node_id']})")
                res = client.run_command(IN_ENGINE_SCRIPT)
                for item in res.get("output", []):
                    print(item.get("output", "").strip())
                return True
        except Exception as e:
            print(f"[INFO] Remote execution unavailable ({e}), falling back to Cmdlet...")

    # 2. Fallback to UnrealEditor-Cmd.exe
    print("[CMDLET] Executing via UnrealEditor-Cmd.exe...")
    temp_script = PROJECT_ROOT / "Intermediate" / "fix_class_select_script.py"
    temp_script.parent.mkdir(parents=True, exist_ok=True)
    temp_script.write_text(IN_ENGINE_SCRIPT, encoding="utf-8")

    script_posix_path = str(temp_script).replace("\\", "/")
    uproject_posix_path = str(UPROJECT_PATH).replace("\\", "/")
    ue_cmd_posix_path = str(UE_CMD_EXE).replace("\\", "/")

    cmd = [
        ue_cmd_posix_path,
        uproject_posix_path,
        "-run=pythonscript",
        f"-script={script_posix_path}",
        "-unattended",
        "-nopause",
        "-nosplash",
        "-stdout"
    ]

    proc = subprocess.run(cmd, capture_output=True, text=True)
    for line in proc.stdout.splitlines():
        if any(tag in line for tag in ["[OK]", "[FIX]", "[SUCCESS]", "[ERROR]", "[WARN]"]):
            print(" ", line.strip())

    if proc.returncode != 0:
        print("[ERROR] Cmdlet execution failed with exit code:", proc.returncode)
        for line in proc.stdout.splitlines()[-20:]:
            print(" ", line)
        return False

    return True

if __name__ == "__main__":
    success = execute()
    sys.exit(0 if success else 1)
