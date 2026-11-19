# 08. Sky Atmosphere, Level Lighting & Environment Standards (UE 5.8)

Rules for generating or configuring outdoor maps, sky-visible dungeon stages, and studio character scenes in UE 5.8.

## 1. Template Duplication vs. Blank Level Pitfall
- `unreal.EditorLevelLibrary.new_level()` spawns a 0-actor world; adding only a `SkyAtmosphere` or `DirectionalLight` produces a **pitch-black void** (no lower-hemisphere fog scattering or Lumen bounce surface).
- **Golden Rule**: Prefer duplicating `/Engine/Maps/Templates/Template_Default` via `EditorAssetLibrary.duplicate_asset()`. If building from scratch, deploy **all 6 mandatory actors** below.

## 2. Mandatory 6-Actor Outdoor Suite & Critical Pitfalls
| Actor | Required Configuration | Pitfall Prevented |
| :--- | :--- | :--- |
| **`DirectionalLight`** | **Negative Pitch (`-30° ~ -60°`)**, `intensity = 6.0 ~ 10.0` Lux, `atmosphere_sun_light = True`, `atmosphere_sun_light_index = 0`, `mobility = Movable`, `cast_shadows = True` | Positive pitch (`+45°`) points sun underground -> 0 Lux midnight black sky. Do NOT use 50,000+ Lux without calibrated physical camera exposure. |
| **`SkyAtmosphere`** | Location `(0, 0, -6000)` or `(0, 0, 0)`, Transform Mode `PlanetTopAtAbsoluteWorldZero` | Rayleigh/Mie sky scattering alignment with `Z = 0` horizon. |
| **`SkyLight`** | **`lower_hemisphere_is_black = False`**, `real_time_capture = True`, `intensity = 1.0`, `mobility = Movable` | `lower_hemisphere_is_black = True` crushes ambient bounce under horizon into harsh black shadows. |
| **`ExponentialHeightFog`** | `fog_density = 0.02`, `fog_inscattering_luminance = LinearColor(0,0,0,1)`, `directional_inscattering_color = LinearColor(0,0,0,1)`, `mobility = Movable` | Required by UE SkyAtmosphere to fill the lower hemisphere below the horizon; black luminance lets SkyAtmosphere drive fog color dynamically. |
| **`VolumetricCloud`** | Location `(0, 0, 700)` (or sky dome) | Provides upper-sky cloud depth instead of flat gradient. |
| **Ground Geometry** | Physical floor/pedestal mesh (`>= 15m–20m`) centered under camera/play area | Lumen GI and reflections require physical surfaces to bounce photons. |

## 3. Post-Process, Exposure & UI Presentation Camera
- **Physical Camera Exposure**: Set `auto_exposure_apply_physical_camera_exposure = False` on `PostProcessVolume` unless authoring calibrated physical-sensor cinematics (otherwise standard 6–10 Lux daylight crushes to black).
- **Menu / Class Select Scenes**: Use `AutoExposureMethod = AEM_HISTOGRAM` with clamped bounds (`MinBrightness = 0.05`, `MaxBrightness = 8.0`) or manual EV100 to prevent eye-adaptation flicker.
- **ViewTarget Binding**: In UI/Presentation levels where `DefaultPawnClass = nullptr`, explicitly call `SetViewTarget(StudioCameraActor)` in local `APlayerController::BeginPlay()` so the viewport does not spawn at `(0,0,0)` inside the floor mesh.

## 4. Reference Automation Scripts
Inspect existing level-lighting setup scripts (`Scripts/setup/setup_template_based_map.py`, `Scripts/setup/fix_class_select_lighting.py`) via `read_file` before writing new map setup scripts.
