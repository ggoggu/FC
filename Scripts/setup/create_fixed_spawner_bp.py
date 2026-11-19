import unreal

def get_or_load_class(class_path):
    cls = unreal.load_class(None, class_path)
    if not cls:
        print(f"[ERROR] Failed to load class: {class_path}")
    return cls

def create_or_get_blueprint(asset_name, package_path, parent_class):
    full_path = f"{package_path}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        print(f"[EXISTS] Loading existing blueprint: {full_path}")
        return unreal.EditorAssetLibrary.load_asset(full_path)

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    bp = asset_tools.create_asset(asset_name, package_path, unreal.Blueprint, factory)
    if bp:
        print(f"[CREATED] Created blueprint: {full_path}")
    else:
        print(f"[ERROR] Failed to create blueprint: {full_path}")
    return bp

def main():
    print("=== Creating Fixed Mob Spawner Blueprint ===")

    cls_parent = get_or_load_class("/Script/FC.FCFixedMobSpawner")
    if not cls_parent:
        print("[FATAL] Could not find C++ class AFCFixedMobSpawner.")
        return False

    cls_hm1 = get_or_load_class("/Game/Character/Enermy/hm1/BP/BP_FCHM1Character.BP_FCHM1Character_C")
    cls_hm2 = get_or_load_class("/Game/Character/Enermy/hm2/BP/BP_FCHM2Character.BP_FCHM2Character_C")

    package_path = "/Game/Character/Enermy/BP"
    asset_name = "BP_FixedMobSpawner"

    bp = create_or_get_blueprint(asset_name, package_path, cls_parent)
    if not bp:
        print("[FATAL] Failed to create BP_FixedMobSpawner")
        return False

    cdo = unreal.get_default_object(bp.generated_class())
    if cdo:
        try:
            if cls_hm1:
                cdo.set_editor_property("default_mob_class", cls_hm1)

            slots = []

            # Slot 0: Front Vanguard (HM1) at +250 X
            s0 = unreal.FCFixedMobSpawnSlot()
            if cls_hm1:
                s0.set_editor_property("mob_class", cls_hm1)
            s0.set_editor_property("slot_tag", "Front_Vanguard")
            t0 = unreal.Transform(location=[250.0, 0.0, 0.0], rotation=[0.0, 0.0, 0.0], scale=[1.0, 1.0, 1.0])
            s0.set_editor_property("relative_transform", t0)
            s0.set_editor_property("b_respawn_on_death", False)
            slots.append(s0)

            # Slot 1: Left Ranger (HM2) at Y = -200, rotated +30 deg yaw
            s1 = unreal.FCFixedMobSpawnSlot()
            if cls_hm2:
                s1.set_editor_property("mob_class", cls_hm2)
            s1.set_editor_property("slot_tag", "Left_Ranger")
            t1 = unreal.Transform(location=[0.0, -200.0, 0.0], rotation=[0.0, 0.0, 30.0], scale=[1.0, 1.0, 1.0])
            s1.set_editor_property("relative_transform", t1)
            s1.set_editor_property("b_respawn_on_death", True)
            s1.set_editor_property("respawn_delay", 5.0)
            slots.append(s1)

            # Slot 2: Right Ranger (HM2) at Y = +200, rotated -30 deg yaw
            s2 = unreal.FCFixedMobSpawnSlot()
            if cls_hm2:
                s2.set_editor_property("mob_class", cls_hm2)
            s2.set_editor_property("slot_tag", "Right_Ranger")
            t2 = unreal.Transform(location=[0.0, 200.0, 0.0], rotation=[0.0, 0.0, -30.0], scale=[1.0, 1.0, 1.0])
            s2.set_editor_property("relative_transform", t2)
            s2.set_editor_property("b_respawn_on_death", True)
            s2.set_editor_property("respawn_delay", 5.0)
            slots.append(s2)

            cdo.set_editor_property("spawn_slots", slots)
            print("[CONFIGURED] Configured default fixed slots (Front_Vanguard, Left_Ranger, Right_Ranger).")
        except Exception as e:
            print(f"[WARN] Error setting CDO properties: {e}")

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as e:
        print(f"[WARN] Compile failed: {e}")

    saved = unreal.EditorAssetLibrary.save_loaded_asset(bp)
    print(f"[SAVED] Saved BP_FixedMobSpawner: {saved}")
    return True

if __name__ == "__main__":
    main()
