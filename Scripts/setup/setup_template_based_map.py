import unreal

def duplicate_and_populate():
    src = "/Engine/Maps/Templates/Template_Default"
    dest = "/Game/Test_MCP/Lv_MCP_MiniMap"
    print(f"=== Duplicating {src} -> {dest} ===")
    
    # If exists, delete first to clean duplicate
    if unreal.EditorAssetLibrary.does_asset_exist(dest):
        unreal.EditorAssetLibrary.delete_asset(dest)
        print(f"Deleted old {dest}")
        
    duplicated = unreal.EditorAssetLibrary.duplicate_asset(src, dest)
    if not duplicated:
        print(f"[WARN] duplicate_asset returned None, trying direct level load and save as...")
        unreal.EditorLevelLibrary.load_level(src)
        unreal.EditorLevelLibrary.save_current_level()
        # Fallback to copy file
        
    print(f"Duplicate result: {duplicated}")
    
    # Load duplicated level
    unreal.EditorLevelLibrary.load_level(dest)
    print(f"Loaded {dest}")
    
    # Check existing actors in template
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    print("Actors in duplicated level:")
    for a in actors:
        print(f"  - {a.get_actor_label()} ({a.get_class().get_name()})")
        
    # Add our Floor, Pillars, PlayerStart, and BP_MCP_TestBeacon
    cls_sm = unreal.StaticMeshActor
    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    mat_grid = unreal.EditorAssetLibrary.load_asset("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")
    
    # 1. Floor: 20m x 20m
    floor = unreal.EditorLevelLibrary.spawn_actor_from_class(cls_sm, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))
    if floor and cube_mesh:
        floor.set_actor_label("Arena_Floor")
        sm = floor.static_mesh_component
        sm.set_static_mesh(cube_mesh)
        if mat_grid:
            sm.set_material(0, mat_grid)
        floor.set_actor_scale3d(unreal.Vector(20.0, 20.0, 0.2))
        print("Spawned Arena Floor (20m x 20m)")
        
    # 2. Four Sun Pillars (To show clear shadow and sunlight angle)
    pillars = [
        ("Pillar_NW", unreal.Vector(-500, 500, 150)),
        ("Pillar_NE", unreal.Vector(500, 500, 150)),
        ("Pillar_SW", unreal.Vector(-500, -500, 150)),
        ("Pillar_SE", unreal.Vector(500, -500, 150)),
    ]
    for p_name, p_loc in pillars:
        p = unreal.EditorLevelLibrary.spawn_actor_from_class(cls_sm, p_loc, unreal.Rotator(0,0,0))
        if p and cube_mesh:
            p.set_actor_label(p_name)
            p.static_mesh_component.set_static_mesh(cube_mesh)
            p.set_actor_scale3d(unreal.Vector(1.0, 1.0, 3.0))
            
    # 3. Beacon Instance
    bp_beacon = unreal.EditorAssetLibrary.load_asset("/Game/Test_MCP/BP_MCP_TestBeacon")
    if bp_beacon:
        beacon = unreal.EditorLevelLibrary.spawn_actor_from_class(bp_beacon.generated_class(), unreal.Vector(0,0,50), unreal.Rotator(0,0,0))
        if beacon:
            beacon.set_actor_label("BP_MCP_TestBeacon_Instance")
            print("Spawned Beacon at Center")
            
    # 4. Save Level
    saved = unreal.EditorLevelLibrary.save_current_level()
    print(f"=== Saved Perfect Template-based Level: {saved} ===")

if __name__ == "__main__":
    duplicate_and_populate()
