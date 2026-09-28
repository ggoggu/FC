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
    print("=== Starting Map Blueprints Generation ===")
    
    # 1. Resolve C++ Parent Classes
    cls_dungeon_mgr = get_or_load_class("/Script/FC.FCDungeonManager")
    cls_room_base = get_or_load_class("/Script/FC.FCRoomBase")
    cls_door_base = get_or_load_class("/Script/FC.FCDoorBase")
    cls_destructible_wall = get_or_load_class("/Script/FC.FCDestructibleWall")
    cls_wall_base = get_or_load_class("/Script/FC.FCWallBase")

    if not all([cls_dungeon_mgr, cls_room_base, cls_door_base, cls_destructible_wall, cls_wall_base]):
        print("[FATAL] One or more C++ parent classes could not be found.")
        return False

    package_path = "/Game/Map"

    # 2. Create BP_DoorBase, BP_DestructibleWall & BP_WallBase
    bp_door = create_or_get_blueprint("BP_DoorBase", package_path, cls_door_base)
    bp_wall = create_or_get_blueprint("BP_DestructibleWall", package_path, cls_destructible_wall)
    bp_solid_wall = create_or_get_blueprint("BP_WallBase", package_path, cls_wall_base)

    # 3. Create Base Room Blueprint
    bp_room_base = create_or_get_blueprint("BP_Room_Base", package_path, cls_room_base)

    # 4. Create Derived Room Blueprints
    room_parent = bp_room_base.generated_class() if bp_room_base else cls_room_base
    bp_room_start = create_or_get_blueprint("BP_Room_Start", package_path, room_parent)
    bp_room_normal = create_or_get_blueprint("BP_Room_Normal_01", package_path, room_parent)
    bp_room_treasure = create_or_get_blueprint("BP_Room_Treasure", package_path, room_parent)
    bp_room_shop = create_or_get_blueprint("BP_Room_Shop", package_path, room_parent)
    bp_room_boss = create_or_get_blueprint("BP_Room_Boss", package_path, room_parent)
    bp_room_secret = create_or_get_blueprint("BP_Room_Secret", package_path, room_parent)

    # 5. Create Dungeon Manager Blueprint
    bp_dungeon_mgr = create_or_get_blueprint("BP_DungeonManager", package_path, cls_dungeon_mgr)

    # 6. Configure CDO Defaults for BP_Room_Base
    if bp_room_base and bp_door and bp_wall:
        room_cdo = unreal.get_default_object(bp_room_base.generated_class())
        if room_cdo:
            try:
                room_cdo.set_editor_property("default_door_class", bp_door.generated_class())
                room_cdo.set_editor_property("default_secret_wall_class", bp_wall.generated_class())
                if bp_solid_wall:
                    room_cdo.set_editor_property("default_wall_class", bp_solid_wall.generated_class())
                print("[CONFIGURED] BP_Room_Base door, secret wall, and solid wall defaults set.")
            except Exception as e:
                print(f"[WARN] Failed setting BP_Room_Base properties: {e}")

    # Configure wave counts for room types
    if bp_room_start:
        cdo = unreal.get_default_object(bp_room_start.generated_class())
        if cdo:
            try:
                cdo.set_editor_property("monster_wave_count", 0)
            except Exception as e:
                print(f"[WARN] Failed setting BP_Room_Start wave count: {e}")

    if bp_room_normal:
        cdo = unreal.get_default_object(bp_room_normal.generated_class())
        if cdo:
            try:
                cdo.set_editor_property("monster_wave_count", 3)
            except Exception as e:
                print(f"[WARN] Failed setting BP_Room_Normal wave count: {e}")

    if bp_room_boss:
        cdo = unreal.get_default_object(bp_room_boss.generated_class())
        if cdo:
            try:
                cdo.set_editor_property("monster_wave_count", 1)
            except Exception as e:
                print(f"[WARN] Failed setting BP_Room_Boss wave count: {e}")

    if bp_room_treasure:
        cdo = unreal.get_default_object(bp_room_treasure.generated_class())
        if cdo:
            try:
                cdo.set_editor_property("monster_wave_count", 0)
            except Exception as e:
                print(f"[WARN] Failed setting BP_Room_Treasure wave count: {e}")

    if bp_room_shop:
        cdo = unreal.get_default_object(bp_room_shop.generated_class())
        if cdo:
            try:
                cdo.set_editor_property("monster_wave_count", 0)
            except Exception as e:
                print(f"[WARN] Failed setting BP_Room_Shop wave count: {e}")

    if bp_room_secret:
        cdo = unreal.get_default_object(bp_room_secret.generated_class())
        if cdo:
            try:
                cdo.set_editor_property("monster_wave_count", 0)
            except Exception as e:
                print(f"[WARN] Failed setting BP_Room_Secret wave count: {e}")

    # 7. Configure BP_DungeonManager Defaults
    if bp_dungeon_mgr:
        mgr_cdo = unreal.get_default_object(bp_dungeon_mgr.generated_class())
        if mgr_cdo:
            try:
                if bp_room_normal:
                    mgr_cdo.set_editor_property("default_room_class", bp_room_normal.generated_class())
                if bp_room_start:
                    mgr_cdo.set_editor_property("start_room_class", bp_room_start.generated_class())
                if bp_room_boss:
                    mgr_cdo.set_editor_property("boss_room_class", bp_room_boss.generated_class())
                if bp_room_shop:
                    mgr_cdo.set_editor_property("shop_room_class", bp_room_shop.generated_class())
                if bp_room_treasure:
                    mgr_cdo.set_editor_property("treasure_room_class", bp_room_treasure.generated_class())
                if bp_room_secret:
                    mgr_cdo.set_editor_property("secret_room_class", bp_room_secret.generated_class())
                if bp_wall:
                    mgr_cdo.set_editor_property("secret_wall_class", bp_wall.generated_class())
                
                mgr_cdo.set_editor_property("target_room_count", 15)
                mgr_cdo.set_editor_property("max_secret_rooms", 1)
                mgr_cdo.set_editor_property("cell_size", 2400.0)
                try:
                    mgr_cdo.set_editor_property("layout_pattern", unreal.FCDungeonLayoutPattern.ASYMMETRIC)
                    mgr_cdo.set_editor_property("start_location_mode", unreal.FCDungeonStartLocation.CENTER)
                    mgr_cdo.set_editor_property("b_prevent_clustering", True)
                except Exception:
                    pass
                print("[CONFIGURED] BP_DungeonManager room class references and config set.")
            except Exception as e:
                print(f"[WARN] Failed setting BP_DungeonManager properties: {e}")

    # 8. Save all created assets
    all_bps = [
        bp_door, bp_wall, bp_solid_wall, bp_room_base,
        bp_room_start, bp_room_normal, bp_room_treasure,
        bp_room_shop, bp_room_boss, bp_room_secret,
        bp_dungeon_mgr
    ]

    for bp in all_bps:
        if bp:
            try:
                unreal.BlueprintEditorLibrary.compile_blueprint(bp)
            except Exception:
                pass
            unreal.EditorAssetLibrary.save_loaded_asset(bp)

    print("=== Successfully Created and Saved All Map Blueprints! ===")
    return True

if __name__ == "__main__":
    main()
