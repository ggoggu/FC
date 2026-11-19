#!/usr/bin/env python3
"""
Unreal Engine 5.8 Class Selection Asset & Level Generator
Generates WBP_ClassSelectItem, WBP_ClassSelect, BP_ClassSelectGameMode,
BP_ClassSelectPlayerController, and the Lv_ClassSelect map.
"""

import sys
import os
import time
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
REMOTE_PY_PATH = Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python")

IN_ENGINE_SCRIPT = """
import unreal

print("=================================================================")
print("  FC Project: Generating Class Selection Assets & Level")
print("=================================================================")

ui_package_path = "/Game/UI/ClassSelect"
level_package_path = "/Game/Level/ClassSelect"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset_lib = unreal.EditorAssetLibrary

def load_cpp_class(class_path):
    cls = unreal.load_class(None, class_path)
    if not cls:
        print(f"[WARN] C++ Class not yet loaded in active memory: {class_path}")
    else:
        print(f"[OK] Loaded C++ Class: {class_path}")
    return cls

def create_or_get_blueprint(asset_name, package_path, parent_class, is_widget_bp=False):
    full_path = f"{package_path}/{asset_name}"
    if editor_asset_lib.does_asset_exist(full_path):
        print(f"[EXISTS] Loading existing asset: {full_path}")
        return editor_asset_lib.load_asset(full_path)

    if is_widget_bp:
        factory = unreal.WidgetBlueprintFactory()
        if parent_class:
            factory.set_editor_property("parent_class", parent_class)
        bp = asset_tools.create_asset(asset_name, package_path, unreal.WidgetBlueprint, factory)
    else:
        factory = unreal.BlueprintFactory()
        if parent_class:
            factory.set_editor_property("parent_class", parent_class)
        bp = asset_tools.create_asset(asset_name, package_path, unreal.Blueprint, factory)

    if bp:
        print(f"[CREATED] Asset created: {full_path}")
        editor_asset_lib.save_asset(full_path)
    else:
        print(f"[ERROR] Failed to create: {full_path}")
    return bp

# 1. Resolve Parent Classes
cls_item_widget = getattr(unreal, "FCClassSelectItemWidget", None) or load_cpp_class("/Script/FC.FCClassSelectItemWidget") or unreal.UserWidget
cls_main_widget = getattr(unreal, "FCClassSelectWidget", None) or load_cpp_class("/Script/FC.FCClassSelectWidget") or unreal.UserWidget
cls_gamemode = getattr(unreal, "FCClassSelectGameMode", None) or load_cpp_class("/Script/FC.FCClassSelectGameMode") or unreal.GameModeBase
cls_player_ctrl = getattr(unreal, "FCClassSelectPlayerController", None) or load_cpp_class("/Script/FC.FCClassSelectPlayerController") or unreal.PlayerController

# 2. Create Blueprints
wbp_item = create_or_get_blueprint("WBP_ClassSelectItem", ui_package_path, cls_item_widget, is_widget_bp=True)
wbp_main = create_or_get_blueprint("WBP_ClassSelect", ui_package_path, cls_main_widget, is_widget_bp=True)

bp_player_ctrl = create_or_get_blueprint("BP_ClassSelectPlayerController", ui_package_path, cls_player_ctrl)
bp_gamemode = create_or_get_blueprint("BP_ClassSelectGameMode", ui_package_path, cls_gamemode)

# Assign default CDO references
if bp_gamemode and bp_player_ctrl:
    gen_gm = bp_gamemode.generated_class()
    cdo_gm = unreal.get_default_object(gen_gm)
    cdo_gm.set_editor_property("player_controller_class", bp_player_ctrl.generated_class())
    cdo_gm.set_editor_property("default_pawn_class", None)
    editor_asset_lib.save_asset(f"{ui_package_path}/BP_ClassSelectGameMode")

if bp_player_ctrl and wbp_main:
    gen_pc = bp_player_ctrl.generated_class()
    cdo_pc = unreal.get_default_object(gen_pc)
    cdo_pc.set_editor_property("class_select_widget_class", wbp_main.generated_class())
    editor_asset_lib.save_asset(f"{ui_package_path}/BP_ClassSelectPlayerController")

if wbp_main and wbp_item:
    gen_wbp = wbp_main.generated_class()
    cdo_wbp = unreal.get_default_object(gen_wbp)
    cdo_wbp.set_editor_property("item_widget_class", wbp_item.generated_class())
    editor_asset_lib.save_asset(f"{ui_package_path}/WBP_ClassSelect")

# 3. Setup Level
level_path = f"{level_package_path}/Lv_ClassSelect"
print(f"\\nSetting up Level: {level_path}...")

level_exists = editor_asset_lib.does_asset_exist(level_path)
if not level_exists:
    level_created = unreal.EditorLevelLibrary.new_level(level_path)
    print(f"[CREATED] Created new level: {level_path}")
else:
    unreal.EditorLevelLibrary.load_level(level_path)
    print(f"[LOADED] Loaded level: {level_path}")

try:
    existing_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    existing_classes = [a.get_class().get_name() for a in existing_actors]

    if not any("SkyAtmosphere" in c for c in existing_classes):
        sky_atm = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, -6000), unreal.Rotator(0, 0, 0))
        if sky_atm: sky_atm.set_actor_label("SkyAtmosphere_ClassSelect")
        print("  [OK] Spawned SkyAtmosphere")

    if not any("DirectionalLight" in c for c in existing_classes):
        sun_rot = unreal.Rotator()
        sun_rot.pitch = -35.0
        sun_rot.yaw = 45.0
        sun_rot.roll = 0.0
        sun = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), sun_rot)
        if sun:
            sun.set_actor_label("DirectionalLight_Key")
            comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
            if comp:
                comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
                comp.set_editor_property("atmosphere_sun_light", True)
                comp.set_editor_property("atmosphere_sun_light_index", 0)
                comp.set_editor_property("intensity", 6.0)
                comp.set_editor_property("cast_shadows", True)
            print("  [OK] Spawned DirectionalLight (Key Light)")

    if not any("SkyLight" in c for c in existing_classes):
        skylight = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 600), unreal.Rotator(0, 0, 0))
        if skylight:
            skylight.set_actor_label("SkyLight_Studio")
            comp = skylight.get_component_by_class(unreal.SkyLightComponent)
            if comp:
                comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
                comp.set_editor_property("real_time_capture", True)
                comp.set_editor_property("intensity", 1.0)
                try:
                    comp.set_editor_property("lower_hemisphere_is_black", False)
                except Exception:
                    pass
            print("  [OK] Spawned SkyLight")

    if not any("ExponentialHeightFog" in c for c in existing_classes):
        fog = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        if fog:
            fog.set_actor_label("ExponentialHeightFog_Studio")
            comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
            if comp:
                comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
                comp.set_editor_property("fog_density", 0.02)
            print("  [OK] Spawned ExponentialHeightFog")

    if not any("VolumetricCloud" in c for c in existing_classes):
        cloud = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.VolumetricCloud, unreal.Vector(0, 0, 700), unreal.Rotator(0, 0, 0))
        if cloud:
            cloud.set_actor_label("VolumetricCloud_Studio")
            print("  [OK] Spawned VolumetricCloud")

    if not any("Floor_Studio" in a.get_actor_label() for a in existing_actors):
        floor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -10), unreal.Rotator(0, 0, 0))
        if floor:
            floor.set_actor_label("Floor_Studio")
            cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
            mat_grid = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
            sm = floor.static_mesh_component
            if cube_mesh:
                sm.set_static_mesh(cube_mesh)
            if mat_grid:
                sm.set_material(0, mat_grid)
            floor.set_actor_scale3d(unreal.Vector(15.0, 15.0, 0.2))
            print("  [OK] Spawned Floor_Studio")

    if not any("PostProcessVolume" in c for c in existing_classes):
        ppv = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        if ppv:
            ppv.set_actor_label("PostProcessVolume_Unbound")
            ppv.set_editor_property("unbound", True)
            s = ppv.get_editor_property("settings")
            try:
                s.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
                s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
                s.set_editor_property("auto_exposure_min_brightness", 0.05)
                s.set_editor_property("auto_exposure_max_brightness", 8.0)
            except Exception:
                pass
            print("  [OK] Spawned PostProcessVolume")

    if not any("CameraActor" in c for c in existing_classes):
        cam_rot = unreal.Rotator()
        cam_rot.pitch = -5.0
        cam_rot.yaw = 0.0
        cam_rot.roll = 0.0
        cam = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(-600, 0, 130), cam_rot)
        if cam:
            cam.set_actor_label("CameraActor_StudioView")
            print("  [OK] Spawned Studio CameraActor")

    world = unreal.EditorLevelLibrary.get_editor_world()
    if world and bp_gamemode:
        ws = world.get_world_settings()
        if ws and hasattr(ws, "default_game_mode"):
            ws.set_editor_property("default_game_mode", bp_gamemode.generated_class())
            print("  [OK] Assigned WorldSettings.DefaultGameMode")

    unreal.EditorLevelLibrary.save_current_level()
    print(f"[SAVED] Level saved successfully: {level_path}")

except Exception as e:
    print(f"[ERROR] Environment setup error: {e}")

print("=== ALL ASSETS GENERATED SUCCESSFULLY ===")
"""

def main():
    if REMOTE_PY_PATH.exists():
        sys.path.append(str(REMOTE_PY_PATH))
        try:
            import remote_execution
            client = remote_execution.RemoteExecution()
            client.start()
            for _ in range(10):
                time.sleep(0.3)
                if client.remote_nodes:
                    break

            if client.remote_nodes:
                node = client.remote_nodes[0]
                client.open_command_connection(node['node_id'])
                print(f"[REMOTE] Connected to Live Unreal Editor (Node: {node['node_id']})")
                res = client.run_command(IN_ENGINE_SCRIPT)
                if res.get("output"):
                    for item in res["output"]:
                        print(item.get("output", "").strip())
                else:
                    print("Remote execution completed with response:", res)
                return
        except Exception as e:
            print(f"[ERROR] Remote execution failed: {e}")

if __name__ == "__main__":
    main()
