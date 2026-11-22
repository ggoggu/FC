import unreal

def setup_navmesh(level_path="/Game/Level/Lv_BattleTest"):
    """
    Ensures Lv_BattleTest has a NavMeshBoundsVolume centered at (0,0,0)
    with scale (400, 400, 20) and RecastNavMesh configured with Dynamic RuntimeGeneration.
    """
    print(f"=== Configuring NavMesh for {level_path} ===")
    
    loaded = unreal.EditorLevelLibrary.load_level(level_path)
    if not loaded:
        print(f"[ERROR] Failed to load level: {level_path}")
        return False

    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    nav_volume = None
    recast_actor = None
    
    for a in all_actors:
        cls_name = a.get_class().get_name()
        if "NavMeshBoundsVolume" in cls_name:
            nav_volume = a
        elif cls_name == "RecastNavMesh":
            recast_actor = a

    # 1. Setup NavMeshBoundsVolume
    if not nav_volume:
        cls_nav = unreal.load_class(None, "/Script/NavigationSystem.NavMeshBoundsVolume")
        spawn_loc = unreal.Vector(0.0, 0.0, 0.0)
        spawn_rot = unreal.Rotator(0.0, 0.0, 0.0)
        nav_volume = unreal.EditorLevelLibrary.spawn_actor_from_class(cls_nav, spawn_loc, spawn_rot)
        print(f"[INFO] Spawned new NavMeshBoundsVolume: {nav_volume.get_name()}")

    if nav_volume:
        nav_volume.set_actor_location(unreal.Vector(0.0, 0.0, 0.0), False, False)
        # Scale (400, 400, 20) covers 800m x 800m x 40m area
        nav_volume.set_actor_scale3d(unreal.Vector(400.0, 400.0, 20.0))
        print(f"[SUCCESS] NavMeshBoundsVolume configured at {nav_volume.get_actor_location()} with scale {nav_volume.get_actor_scale3d()}")

    # 2. Setup RecastNavMesh
    if recast_actor:
        try:
            recast_actor.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
            print("[SUCCESS] RecastNavMesh runtime_generation set to DYNAMIC.")
        except Exception as e:
            print(f"[WARN] Failed setting runtime_generation on RecastNavMesh: {e}")

        try:
            recast_actor.set_editor_property("tile_size_uu", 1200.0)
            print("[SUCCESS] RecastNavMesh tile_size_uu set to 1200.0.")
        except Exception as e:
            print(f"[WARN] Failed setting tile_size_uu on RecastNavMesh: {e}")

    unreal.EditorLevelLibrary.save_current_level()
    print(f"=== Successfully Saved {level_path} with NavMesh Configuration ===")
    return True

if __name__ == "__main__":
    setup_navmesh()
