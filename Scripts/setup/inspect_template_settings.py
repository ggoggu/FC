import unreal

def inspect_template_default():
    template_path = "/Engine/Maps/Templates/Template_Default"
    print(f"=== Inspecting {template_path} ===")
    
    loaded = unreal.EditorLevelLibrary.load_level(template_path)
    if not loaded:
        print("[ERROR] Failed to load Template_Default")
        return
        
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for a in actors:
        cls_name = a.get_class().get_name()
        label = a.get_actor_label()
        print(f"\n[ACTOR] {label} ({cls_name})")
        
        if "DirectionalLight" in cls_name:
            c = a.get_component_by_class(unreal.DirectionalLightComponent)
            if c:
                print(f"  Intensity: {c.get_editor_property('intensity')}")
                print(f"  Mobility: {c.get_editor_property('mobility')}")
                print(f"  AtmosphereSunLight: {c.get_editor_property('atmosphere_sun_light')}")
                print(f"  Temperature: {c.get_editor_property('temperature')}")
                print(f"  Rotation: {a.get_actor_rotation()}")
                
        elif "SkyLight" in cls_name:
            c = a.get_component_by_class(unreal.SkyLightComponent)
            if c:
                print(f"  Intensity: {c.get_editor_property('intensity')}")
                print(f"  Mobility: {c.get_editor_property('mobility')}")
                print(f"  RealTimeCapture: {c.get_editor_property('real_time_capture')}")
                
        elif "PostProcess" in cls_name:
            print(f"  Unbound: {a.get_editor_property('unbound')}")
            s = a.get_editor_property('settings')
            try:
                print(f"  ExposureMethod: {s.get_editor_property('auto_exposure_method')}")
                print(f"  ExposureMinEV100: {s.get_editor_property('auto_exposure_min_brightness')}")
                print(f"  ExposureMaxEV100: {s.get_editor_property('auto_exposure_max_brightness')}")
                print(f"  ExposureBias: {s.get_editor_property('auto_exposure_bias')}")
            except Exception as e:
                print(f"  Settings read: {e}")

if __name__ == "__main__":
    inspect_template_default()
