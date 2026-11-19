import unreal

def setup_true_openworld_sky():
    print("=== [MCP Test] Rebuilding Map with UE5 True Movable Sky & Sun ===")
    
    package_path = "/Game/Test_MCP"
    level_path = f"{package_path}/Lv_MCP_MiniMap"
    
    # 1. Load Level
    if not unreal.EditorAssetLibrary.does_asset_exist(level_path):
        print(f"[ERROR] Level does not exist: {level_path}")
        return False
        
    unreal.EditorLevelLibrary.load_level(level_path)
    print(f"[1/5] Loaded Level: {level_path}")

    # 2. Destroy existing lighting, sky, and atmosphere actors completely
    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for a in all_actors:
        cls_name = a.get_class().get_name()
        label = a.get_actor_label()
        if any(keyword in cls_name or keyword in label for keyword in [
            "DirectionalLight", "SkyLight", "SkyAtmosphere", "VolumetricCloud", 
            "ExponentialHeightFog", "PostProcess", "PointLight", "Floor", "Wall"
        ]):
            unreal.EditorLevelLibrary.destroy_actor(a)
    print("[2/5] Completely purged old lighting & world actors.")

    # 3. Create Full Real-Time Movable Environment (UE5 Lumen + Virtual Shadow Maps)
    print("[3/5] Constructing Complete Dynamic Sky Suite...")

    # 3-1. SkyAtmosphere (Atmospheric Scattering)
    try:
        sky_atm = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.SkyAtmosphere, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
        )
        if sky_atm:
            sky_atm.set_actor_label("SkyAtmosphere_Default")
            print("  ✓ Created SkyAtmosphere (Blue Sky & Horizon)")
    except Exception as e:
        print(f"  [ERROR] SkyAtmosphere: {e}")

    # 3-2. DirectionalLight (Sun) - CRITICAL: Must be MOVABLE for Lumen & Atmosphere!
    sun_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 500.0), unreal.Rotator(-38.0, 45.0, 0.0)
    )
    if sun_actor:
        sun_actor.set_actor_label("DirectionalLight_Sun")
        sun_comp = sun_actor.get_component_by_class(unreal.DirectionalLightComponent)
        if sun_comp:
            # Set Movable
            sun_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            # Link to SkyAtmosphere
            sun_comp.set_editor_property("atmosphere_sun_light", True)
            sun_comp.set_editor_property("atmosphere_sun_light_index", 0)
            # Physical Light settings
            sun_comp.set_editor_property("intensity", 65000.0) # Physical Sunlight in Lux!
            sun_comp.set_editor_property("light_color", unreal.Color(255, 250, 240, 255))
            sun_comp.set_editor_property("cast_shadows", True)
            sun_comp.set_editor_property("cast_cloud_shadows", True)
            sun_comp.set_editor_property("cloud_shadow_extent", 15.0)
            sun_comp.set_editor_property("use_temperature", True)
            sun_comp.set_editor_property("temperature", 5800.0) # Warm afternoon sun
        print("  ✓ Created Movable Sun (DirectionalLight, 65,000 Lux, AtmosphereSun=True, Movable)")

    # 3-3. VolumetricCloud (Realistic Moving Clouds in Sky)
    try:
        cloud_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.VolumetricCloud, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
        )
        if cloud_actor:
            cloud_actor.set_actor_label("VolumetricCloud_Default")
            cloud_comp = cloud_actor.get_component_by_class(unreal.VolumetricCloudComponent)
            if cloud_comp:
                # Load default cloud material from engine
                cloud_mat = unreal.EditorAssetLibrary.load_asset("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")
                if cloud_mat:
                    cloud_comp.set_material(cloud_mat)
            print("  ✓ Created VolumetricCloud (Realistic Cloud Layers)")
    except Exception as e:
        print(f"  [ERROR] VolumetricCloud: {e}")

    # 3-4. SkyLight - CRITICAL: Must be MOVABLE and RealTimeCapture for Ambient Sky Light!
    sky_light_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0.0, 0.0, 400.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    if sky_light_actor:
        sky_light_actor.set_actor_label("SkyLight_RealTime")
        sky_comp = sky_light_actor.get_component_by_class(unreal.SkyLightComponent)
        if sky_comp:
            sky_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            sky_comp.set_editor_property("real_time_capture", True)
            sky_comp.set_editor_property("intensity", 1.0)
            sky_comp.set_editor_property("cloud_ambient_occlusion", True)
        print("  ✓ Created Movable SkyLight (RealTimeCapture=True, Ambient Sky Illumination)")

    # 3-5. ExponentialHeightFog (Atmosphere Fog & Inscattering)
    try:
        fog_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.ExponentialHeightFog, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
        )
        if fog_actor:
            fog_actor.set_actor_label("ExponentialHeightFog_Default")
            fog_comp = fog_actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
            if fog_comp:
                fog_comp.set_editor_property("fog_density", 0.01)
                fog_comp.set_editor_property("fog_height_falloff", 0.2)
                fog_comp.set_editor_property("volumetric_fog", True)
                fog_comp.set_editor_property("volumetric_fog_distance", 6000.0)
            print("  ✓ Created ExponentialHeightFog (Volumetric Sun Rays & Horizon Blend)")
    except Exception as e:
        print(f"  [ERROR] ExponentialHeightFog: {e}")

    # 3-6. PostProcessVolume (Lumen Global Illumination & Auto-Exposure)
    try:
        pp_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.PostProcessVolume, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
        )
        if pp_actor:
            pp_actor.set_actor_label("PostProcess_LumenAutoExposure")
            pp_actor.set_editor_property("unbound", True) # Infinite extent
            pp_settings = pp_actor.get_editor_property("settings")
            # Lumen Settings
            pp_settings.set_editor_property("dynamic_global_illumination_method", unreal.DynamicGlobalIlluminationMethod.LUMEN)
            pp_settings.set_editor_property("reflection_method", unreal.ReflectionMethod.LUMEN)
            # Physical Exposure for 65000 Lux Sun
            pp_settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
            pp_settings.set_editor_property("auto_exposure_min_brightness", 0.03)
            pp_settings.set_editor_property("auto_exposure_max_brightness", 12.0)
            pp_settings.set_editor_property("auto_exposure_bias", 0.0)
            pp_actor.set_editor_property("settings", pp_settings)
            print("  ✓ Created Unbound PostProcessVolume (Lumen GI + Realistic EV100 Exposure)")
    except Exception as e:
        print(f"  [ERROR] PostProcessVolume: {e}")

    # 4. Arena Floor & Walls with Natural Sunlight Shadows
    print("[4/5] Building Open-Air Arena Floor and Pillars...")
    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    grid_mat = unreal.EditorAssetLibrary.load_asset("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")
    
    # Floor: 24m x 24m Large Open Area
    floor_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    if floor_actor and cube_mesh:
        floor_actor.set_actor_label("Arena_Floor_Main")
        sm_comp = floor_actor.static_mesh_component
        sm_comp.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
        sm_comp.set_static_mesh(cube_mesh)
        if grid_mat:
            sm_comp.set_material(0, grid_mat)
        floor_actor.set_actor_scale3d(unreal.Vector(24.0, 24.0, 0.2)) # 24m x 24m
        print("  ✓ Created Large Open-Air Floor (24m x 24m)")

    # Low boundary barriers (0.8m height so sky is 100% visible in all directions!)
    wall_specs = [
        ("Barrier_North", unreal.Vector(0.0, 1200.0, 40.0), unreal.Vector(24.4, 0.4, 0.8)),
        ("Barrier_South", unreal.Vector(0.0, -1200.0, 40.0), unreal.Vector(24.4, 0.4, 0.8)),
        ("Barrier_East",  unreal.Vector(1200.0, 0.0, 40.0),  unreal.Vector(0.4, 24.4, 0.8)),
        ("Barrier_West",  unreal.Vector(-1200.0, 0.0, 40.0),  unreal.Vector(0.4, 24.4, 0.8)),
    ]
    for label, loc, scale in wall_specs:
        w_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, loc, unreal.Rotator(0.0, 0.0, 0.0))
        if w_actor and cube_mesh:
            w_actor.set_actor_label(label)
            sm = w_actor.static_mesh_component
            sm.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
            sm.set_static_mesh(cube_mesh)
            if grid_mat:
                sm.set_material(0, grid_mat)
            w_actor.set_actor_scale3d(scale)
    print("  ✓ Created Low Perimeter Barriers (Open Horizon Guaranteed)")

    # 4 Decorative Sun-Casting Pillars (To clearly show sun angle & dynamic shadows!)
    pillar_locs = [
        ("SunPillar_NW", unreal.Vector(-600.0, 600.0, 200.0)),
        ("SunPillar_NE", unreal.Vector(600.0, 600.0, 200.0)),
        ("SunPillar_SW", unreal.Vector(-600.0, -600.0, 200.0)),
        ("SunPillar_SE", unreal.Vector(600.0, -600.0, 200.0)),
    ]
    for p_label, p_loc in pillar_locs:
        p_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, p_loc, unreal.Rotator(0.0, 0.0, 0.0))
        if p_actor and cube_mesh:
            p_actor.set_actor_label(p_label)
            sm = p_actor.static_mesh_component
            sm.set_static_mesh(cube_mesh)
            p_actor.set_actor_scale3d(unreal.Vector(1.0, 1.0, 4.0)) # 4m tall pillar
    print("  ✓ Placed 4 Sun-Casting Pillars (Clear Sunlight & Shadow Contrast)")

    # 5. Place PlayerStart and Central Beacon
    print("[5/5] Placing PlayerStart and Test Beacon...")
    # PlayerStart
    pstart = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(-600.0, -600.0, 80.0), unreal.Rotator(0.0, 45.0, 0.0)
    )
    if pstart:
        pstart.set_actor_label("PlayerStart_Main")
        print("  ✓ PlayerStart set at (-600, -600, 80)")

    # Test Beacon Actor
    bp_beacon = unreal.EditorAssetLibrary.load_asset(f"{package_path}/BP_MCP_TestBeacon")
    if bp_beacon:
        beacon_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            bp_beacon.generated_class(), unreal.Vector(0.0, 0.0, 50.0), unreal.Rotator(0.0, 0.0, 0.0)
        )
        if beacon_actor:
            beacon_actor.set_actor_label("BP_MCP_TestBeacon_Instance")
            print("  ✓ BP_MCP_TestBeacon placed at Center")

    # 6. Save Level
    saved = unreal.EditorLevelLibrary.save_current_level()
    print(f"=== [SUCCESS] True OpenWorld Daylight Level Saved: {saved} ===")
    return True

if __name__ == "__main__":
    setup_true_openworld_sky()
