# Sky Atmosphere, Level Lighting & Environment Standards

This document defines the architectural guidelines and engine requirements for generating, modifying, or configuring outdoor maps, open-sky dungeon levels, and studio character presentation scenes in Unreal Engine 5.8.

---

## 1. Core Principle: Template Inheritance vs. Blank Level Void

In Unreal Engine 5.8, rendering physically accurate skies and global illumination (Lumen) relies on a delicate coupling between multiple atmospheric and lighting actors.

### The Pitfall of `new_level()` (Empty Level)
- Calling `unreal.EditorLevelLibrary.new_level()` creates a completely empty world (0 actors).
- Placing only a `SkyAtmosphere` or `DirectionalLight` in an empty world produces a **pitch-black void** because the atmospheric scattering pipeline, ground reflection, and fog integration are missing.

### Golden Rule:
1. **Prefer Template Duplication**: When creating outdoor levels programmatically, duplicate the engine standard template `/Engine/Maps/Templates/Template_Default` via `EditorAssetLibrary.duplicate_asset()`.
2. **Or Deploy the Full 6-Actor Suite**: If building a level from scratch, you **must** spawn and configure all 6 core environmental actors listed in Section 2.

---

## 2. The 6 Mandatory Environment Actors for Sky-Visible Maps

Every outdoor level or sky-visible stage must contain the following components configured to the documented specifications:

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              OUTDOOR SKY & LIGHTING SUITE                              │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ 1. DirectionalLight    : Sun source (Pitch: -30° ~ -60° downwards, AtmosphereSunLight) │
│ 2. SkyAtmosphere       : Atmospheric Rayleigh & Mie scattering shader                  │
│ 3. SkyLight            : 360° ambient fill (RealTimeCapture=True, LowerBlack=False)    │
│ 4. ExponentialHeightFog: Lower hemisphere color filler (Required by UE SkyAtmosphere)  │
│ 5. VolumetricCloud     : Volumetric cloud layers across the upper sky dome             │
│ 6. Ground Geometry     : Floor/Pedestal mesh (Lumen bounce surface; prevents void)     │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Component Configuration Standards & Pitfalls

### 3.1. DirectionalLight (Atmosphere Sun Light)
- **Sun Orientation (Critical Pitfall)**:
  - In Unreal Engine, light travels along the DirectionalLight's forward vector (`+X`).
  - **Pitch MUST be negative** (`Pitch = -30.0° ~ -60.0°`), indicating light shines downward towards the ground (`ForwardVector.Z < 0`).
  - **Negative Pitch Error**: If Pitch is positive (`+45.0°`), light shoots upward into space, meaning the sun is **underground (-45° below horizon)**. The atmosphere calculates **midnight (0 Lux scattering)**, resulting in a **100% pitch-black sky**.
- **Intensity**:
  - Default daylight standard: **`6.0 ~ 10.0 Lux`** (matches `Template_Default`).
  - **Do NOT set 50,000+ Lux** unless the entire scene is calibrated for a physical camera pipeline with ISO/shutter speed exposure.
- **Mandatory Properties**:
  - `atmosphere_sun_light = True`
  - `atmosphere_sun_light_index = 0`
  - `mobility = Movable`
  - `cast_shadows = True`

### 3.2. SkyAtmosphere
- Simulates planetary curvature and atmospheric scattering.
- **Location**: Set to `(0, 0, -6000)` or `(0, 0, 0)` matching planet origin.
- **Transform Mode**: `PlanetTopAtAbsoluteWorldZero` (virtual planet surface at `Z = 0`).

### 3.3. ExponentialHeightFog (Mandatory for SkyAtmosphere)
- **Official Epic Engine Specification**:
  - *By default, SkyAtmosphere simulates a physical planet. Below the horizon (Lower Hemisphere), there is no participating media, causing the lower half of the screen to appear completely black.*
- **Requirement**: `ExponentialHeightFog` **must** be placed in the scene to blend and illuminate the horizon.
- **Configuration**:
  - `fog_density = 0.02`
  - `fog_inscattering_luminance = LinearColor(0, 0, 0, 1)` (Set to black so SkyAtmosphere dynamically drives fog color).
  - `directional_inscattering_color = LinearColor(0, 0, 0, 1)`
  - `mobility = Movable`

### 3.4. SkyLight (Ambient Lighting)
- **Lower Hemisphere Setting (Critical Pitfall)**:
  - **`lower_hemisphere_is_black = False`**
  - Setting this to `True` forcefully blacks out all ambient bounce originating below the horizon, creating harsh unnatural shadows.
- **Capture**: `real_time_capture = True`, `intensity = 1.0`, `mobility = Movable`.

### 3.5. VolumetricCloud / Sky Background
- Spawn `VolumetricCloud` at `(0, 0, 700)` or include a sky dome to give the camera depth and realistic cloud coverage rather than a flat gradient.

### 3.6. Ground Geometry (Lumen Surface Requirement)
- Lumen Global Illumination and reflection solvers require physical 3D surfaces to bounce photons.
- An empty level with no meshes will appear dark and unlit.
- Always include at least a 15m–20m floor, landscape tile, or circular pedestal (`Floor_Studio` / `Arena_Floor`) centered under the active gameplay/camera area.

---

## 4. Post-Process & Exposure Standards

- **Physical Camera Exposure Pitfall**:
  - In `PostProcessVolume`, ensure `auto_exposure_apply_physical_camera_exposure = False` unless you are authoring a dedicated cinematic shot with calibrated physical camera sensors. When enabled without physical lighting calibration, it crushes standard 6 Lux scenes into blackness.
- **UI & Character Selection Scenes**:
  - Set `AutoExposureMethod = AEM_HISTOGRAM`.
  - Constrain exposure range (`MinBrightness = 0.05`, `MaxBrightness = 8.0`) or use manual EV100 to prevent jarring eye-adaptation flickering when switching menus or characters.
- **ViewTarget Binding**:
  - In UI/Presentation levels where `DefaultPawnClass = nullptr`, the local `APlayerController` must explicitly invoke `SetViewTarget(StudioCameraActor)` in `BeginPlay()`; otherwise, the viewport defaults to `(0,0,0)` inside the floor mesh.

---

## 5. Verified Python Automation Template

When writing automated Python scripts (`Scripts/setup_*_map.py`), use this standard template to guarantee a fully lit, sky-visible level:

```python
import unreal

def setup_sky_and_lighting(level_path):
    unreal.EditorLevelLibrary.load_level(level_path)
    
    # 1. Directional Light (Negative pitch for downward sunlight)
    sun_rot = unreal.Rotator()
    sun_rot.pitch = -35.0  # Sun 35 degrees above horizon
    sun_rot.yaw = 45.0
    sun_rot.roll = 0.0
    
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 500), sun_rot
    )
    sun.set_actor_label("DirectionalLight_Sun")
    comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    comp.set_editor_property("atmosphere_sun_light", True)
    comp.set_editor_property("intensity", 6.0)
    
    # 2. Sky Atmosphere
    sky_atm = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector(0, 0, -6000), unreal.Rotator()
    )
    sky_atm.set_actor_label("SkyAtmosphere_Default")
    
    # 3. Sky Light (Allow ground bounce)
    skylight = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 600), unreal.Rotator()
    )
    skylight.set_actor_label("SkyLight_Default")
    sky_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
    sky_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sky_comp.set_editor_property("real_time_capture", True)
    sky_comp.set_editor_property("lower_hemisphere_is_black", False)
    
    # 4. Exponential Height Fog (Eliminates lower hemisphere black void)
    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator()
    )
    fog.set_actor_label("ExponentialHeightFog_Default")
    fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    fog_comp.set_editor_property("fog_density", 0.02)
    fog_comp.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0, 0, 0, 1))
    
    # 5. Volumetric Cloud
    cloud = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.VolumetricCloud, unreal.Vector(0, 0, 700), unreal.Rotator()
    )
    cloud.set_actor_label("VolumetricCloud_Default")
    
    # 6. Ground Surface (Lumen reflection & shadow receiver)
    floor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(0, 0, -10), unreal.Rotator()
    )
    floor.set_actor_label("Floor_Default")
    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    floor.static_mesh_component.set_static_mesh(cube_mesh)
    floor.set_actor_scale3d(unreal.Vector(15.0, 15.0, 0.2))
    
    unreal.EditorLevelLibrary.save_current_level()
```
