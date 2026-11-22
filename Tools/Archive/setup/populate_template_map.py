import unreal

def populate_copied_template():
    level_path = "/Game/Test_MCP/Lv_MCP_MiniMap"
    print(f"=== Loading Copied Template: {level_path} ===")
    
    loaded = unreal.EditorLevelLibrary.load_level(level_path)
    if not loaded:
        print(f"[ERROR] Failed to load {level_path}")
        return False

    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    print("Pre-existing actors in Epic Template:")
    for a in all_actors:
        print(f"  - {a.get_actor_label()} ({a.get_class().get_name()})")

    # Ensure SkyLight has real-time capture
    for a in all_actors:
        if "SkyLight" in a.get_class().get_name():
            comp = a.get_component_by_class(unreal.SkyLightComponent)
            if comp:
                try:
                    comp.set_editor_property("real_time_capture", True)
                    print("  [OK] SkyLight real_time_capture set to True")
                except Exception as e:
                    print(f"  [WARN] {e}")

    # Ensure Sun has atmosphere sun light enabled
    for a in all_actors:
        if "DirectionalLight" in a.get_class().get_name():
            comp = a.get_component_by_class(unreal.DirectionalLightComponent)
            if comp:
                try:
                    comp.set_editor_property("atmosphere_sun_light", True)
                    comp.set_editor_property("cast_shadows", True)
                    print("  [OK] DirectionalLight atmosphere_sun_light set to True")
                except Exception as e:
                    print(f"  [WARN] {e}")

    # Clean only our previous custom objects if any
    for a in all_actors:
        label = a.get_actor_label()
        if any(tag in label for tag in ["Arena_Floor", "Pillar_", "BP_MCP_TestBeacon", "Barrier_"]):
            unreal.EditorLevelLibrary.destroy_actor(a)

    # 1. Spawn Arena Floor (24m x 24m)
    cls_sm = unreal.StaticMeshActor
    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    grid_mat = unreal.EditorAssetLibrary.load_asset("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")

    floor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        cls_sm, unreal.Vector(0.0, 0.0, -10.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    if floor and cube_mesh:
        floor.set_actor_label("Arena_Floor")
        sm = floor.static_mesh_component
        sm.set_static_mesh(cube_mesh)
        if grid_mat:
            sm.set_material(0, grid_mat)
        floor.set_actor_scale3d(unreal.Vector(24.0, 24.0, 0.2))
        print("  ✓ Created 24m x 24m Arena Floor")

    # 2. Spawn 4 Sun-Casting Pillars (4m height)
    pillars = [
        ("Pillar_NW", unreal.Vector(-600.0, 600.0, 150.0)),
        ("Pillar_NE", unreal.Vector(600.0, 600.0, 150.0)),
        ("Pillar_SW", unreal.Vector(-600.0, -600.0, 150.0)),
        ("Pillar_SE", unreal.Vector(600.0, -600.0, 150.0)),
    ]
    for p_name, p_loc in pillars:
        p = unreal.EditorLevelLibrary.spawn_actor_from_class(cls_sm, p_loc, unreal.Rotator(0.0, 0.0, 0.0))
        if p and cube_mesh:
            p.set_actor_label(p_name)
            sm = p.static_mesh_component
            sm.set_static_mesh(cube_mesh)
            if grid_mat:
                sm.set_material(0, grid_mat)
            p.set_actor_scale3d(unreal.Vector(1.0, 1.0, 3.5))
    print("  ✓ Placed 4 Sun-Casting Pillars")

    # 3. Spawn Test Beacon Actor
    bp_beacon = unreal.EditorAssetLibrary.load_asset("/Game/Test_MCP/BP_MCP_TestBeacon")
    if bp_beacon:
        beacon = unreal.EditorLevelLibrary.spawn_actor_from_class(
            bp_beacon.generated_class(), unreal.Vector(0.0, 0.0, 50.0), unreal.Rotator(0.0, 0.0, 0.0)
        )
        if beacon:
            beacon.set_actor_label("BP_MCP_TestBeacon_Instance")
            print("  ✓ Spawned BP_MCP_TestBeacon at Center (0, 0, 50)")

    # 4. Save Level
    saved = unreal.EditorLevelLibrary.save_current_level()
    print(f"=== Successfully Saved Level: {saved} ===")
    return True

if __name__ == "__main__":
    populate_copied_template()
