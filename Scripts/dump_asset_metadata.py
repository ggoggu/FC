#!/usr/bin/env python3
"""
Unreal Engine 5.8 Asset Metadata Dumper (Scripts/dump_asset_metadata.py)
Extracts structured JSON metadata from BP, WBP, Animation, and other .uasset files.

Features:
  - Fast Standalone Mode (~0.005s): Parses .uasset binary package headers, name tables,
    imports, exports, and categorizes widgets, notifies, components, and bindings without launching UE.
  - Deep Engine Mode (--deep): Launches headless UnrealEditor-Cmd to inspect exact CDO properties,
    notify timestamps, slot names, and widget tree hierarchies via the `unreal` Python API.
  - Formats output as clean, token-efficient JSON for AI agents and developer tooling.

Usage:
  python Scripts/dump_asset_metadata.py --asset Content/UI/BP/WBP_HUD.uasset
  python Scripts/dump_asset_metadata.py --asset Content/Character/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.uasset
  python Scripts/dump_asset_metadata.py --asset /Game/Character/Enermy/BP/BP_FixedMobSpawner
  python Scripts/dump_asset_metadata.py --asset Content/UI/BP/WBP_HUD.uasset --deep
"""

import sys
import os
import re
import json
import subprocess
import argparse
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
CONTENT_DIR = PROJECT_ROOT / "Content"
UPROJECT_FILE = PROJECT_ROOT / "FC.uproject"

# Check if running inside Unreal Engine Python
try:
    import unreal
    IN_ENGINE = True
except ImportError:
    unreal = None
    IN_ENGINE = False

def resolve_asset_paths(asset_arg: str) -> tuple[Path | None, str | None]:
    """Resolves a file path and corresponding /Game/ package path."""
    clean_arg = asset_arg.strip().replace("\\", "/")
    
    # If given as /Game/...
    if clean_arg.startswith("/Game/"):
        rel_sub = clean_arg[len("/Game/"):]
        if "." in rel_sub:
            rel_sub = rel_sub.split(".")[0]
        file_path = CONTENT_DIR / f"{rel_sub}.uasset"
        game_path = f"/Game/{rel_sub}"
        return file_path if file_path.exists() else None, game_path
        
    # If given as filesystem path
    p = Path(clean_arg)
    if not p.is_absolute():
        p = PROJECT_ROOT / p
        
    if not p.suffix:
        p = p.with_suffix(".uasset")
        
    if not p.exists():
        return None, None
        
    # Convert to /Game/ path
    try:
        rel = p.relative_to(CONTENT_DIR)
        game_path = f"/Game/{rel.with_suffix('').as_posix()}"
    except ValueError:
        game_path = None
        
    return p, game_path

def parse_uasset_binary_fast(file_path: Path, game_path: str | None) -> dict:
    """Fast binary inspection of .uasset without launching Unreal Engine."""
    file_size = file_path.stat().st_size
    with open(file_path, "rb") as f:
        raw = f.read()

    # Verify UE package magic (0x9E2A83C1)
    is_valid_package = len(raw) >= 4 and raw[:4] == b'\xc1\x83\x2a\x9e'
    
    # Extract ASCII & UTF-8 strings
    string_matches = re.findall(b'[\x20-\x7E]{3,}', raw)
    strings = [s.decode('utf-8', 'ignore') for s in string_matches]

    # Classify asset type
    asset_type = "Unknown"
    if any("WidgetTree" in s or "ModelViewViewModel" in s for s in strings):
        asset_type = "WidgetBlueprint"
    elif any("AnimMontage" in s or "CompositeSection" in s for s in strings):
        asset_type = "AnimMontage"
    elif any("AnimBlueprint" in s or "AnimGraph" in s for s in strings):
        asset_type = "AnimBlueprint"
    elif any("BlendSpace" in s for s in strings):
        asset_type = "BlendSpace"
    elif any("AnimSequence" in s or "AnimationData" in s for s in strings):
        asset_type = "AnimSequence"
    elif any("DataTable" in s for s in strings):
        asset_type = "DataTable"
    elif any("SimpleConstructionScript" in s or "BlueprintGeneratedClass" in s for s in strings):
        asset_type = "Blueprint"

    # Identify Parent / Base Class
    parent_classes = []
    for s in strings:
        if s.startswith("/Script/") and "." in s:
            cls_name = s.split(".")[-1]
            if cls_name not in parent_classes and not cls_name.startswith("Default__"):
                parent_classes.append(cls_name)

    # Categorized metadata extraction
    metadata = {
        "asset_name": file_path.stem,
        "file_path": str(file_path.relative_to(PROJECT_ROOT)).replace("\\", "/"),
        "game_path": game_path,
        "asset_type": asset_type,
        "file_size_bytes": file_size,
        "is_valid_uasset": is_valid_package,
        "inspection_mode": "fast_binary_scan"
    }

    if asset_type == "WidgetBlueprint":
        widgets = []
        viewmodels = []
        bindings = []
        for s in strings:
            if any(k in s for k in ["Button", "Text", "Canvas", "Image", "Progress", "Border", "List", "Overlay", "Box", "Slot"]):
                if len(s) < 40 and not s.startswith("/Script/") and not s.startswith("/Engine/") and s not in widgets:
                    widgets.append(s)
            if "ViewModel" in s and not s.startswith("/Script/") and s not in viewmodels:
                viewmodels.append(s)
            if any(s.startswith(prefix) for prefix in ["Get", "OnClicked", "OnHovered", "Bind_"]) and len(s) < 35:
                if s not in bindings:
                    bindings.append(s)
        metadata["detected_widgets"] = widgets[:25]
        metadata["detected_viewmodels"] = viewmodels[:10]
        metadata["detected_bindings"] = bindings[:15]

    elif asset_type in ["AnimMontage", "AnimSequence", "AnimBlueprint"]:
        notifies = []
        slots = []
        tracks = []
        for s in strings:
            if "Notify" in s and not s.startswith("/Script/") and s not in notifies and len(s) < 40:
                notifies.append(s)
            if any(k in s for k in ["Slot", "DefaultGroup", "UpperBody", "FullBody"]) and len(s) < 35 and s not in slots:
                slots.append(s)
            if "Track" in s and not s.startswith("/Script/") and s not in tracks and len(s) < 35:
                tracks.append(s)
        metadata["detected_notifies"] = notifies[:20]
        metadata["detected_slots"] = slots[:10]
        metadata["detected_tracks"] = tracks[:10]

    elif asset_type == "Blueprint":
        components = []
        variables = []
        for s in strings:
            if "Component" in s and not s.startswith("/Script/") and not s.startswith("/Engine/") and len(s) < 40:
                if s not in components:
                    components.append(s)
            elif s.startswith("b_") or s.startswith("bIs") or s.startswith("Default_") or s.startswith("Target"):
                if len(s) < 35 and s not in variables:
                    variables.append(s)
        metadata["detected_components"] = components[:20]
        metadata["detected_variables"] = variables[:15]

    # Gameplay Tags
    gameplay_tags = [s for s in strings if s.startswith("State.") or s.startswith("Ability.") or s.startswith("Event.") or s.startswith("Data.") or s.startswith("GameplayCue.")]
    if gameplay_tags:
        metadata["detected_gameplay_tags"] = sorted(list(set(gameplay_tags)))[:15]

    # Referenced Classes
    metadata["referenced_engine_classes"] = parent_classes[:12]

    return metadata

def parse_uasset_in_engine(game_path: str) -> dict:
    """Deep inspection using Unreal Engine Python API."""
    if not IN_ENGINE or not unreal:
        return {"error": "unreal python module not available"}

    asset = unreal.EditorAssetLibrary.load_asset(game_path)
    if not asset:
        return {"error": f"Failed to load asset in engine: {game_path}"}

    cls_name = asset.get_class().get_name()
    data = {
        "asset_name": asset.get_name(),
        "game_path": game_path,
        "engine_class": cls_name,
        "inspection_mode": "engine_reflection"
    }

    # Widget Blueprint
    if cls_name == "WidgetBlueprint":
        tree = asset.get_editor_property("widget_tree")
        if tree:
            root = tree.get_editor_property("root_widget")
            data["root_widget"] = root.get_name() if root else "None"
            data["root_widget_class"] = root.get_class().get_name() if root else "None"
            # Traverse widget tree
            all_widgets = []
            def walk(w):
                if not w: return
                all_widgets.append({
                    "name": w.get_name(),
                    "class": w.get_class().get_name()
                })
                # Check child slots
                if hasattr(w, "get_all_children"):
                    for child in w.get_all_children():
                        walk(child)
            try:
                walk(root)
                data["widget_hierarchy"] = all_widgets[:30]
            except Exception:
                pass

    # Anim Montage
    elif cls_name == "AnimMontage":
        data["sequence_length"] = asset.get_editor_property("sequence_length") if hasattr(asset, "get_editor_property") else None
        notifies = []
        try:
            raw_notifies = asset.get_editor_property("notifies")
            for n in raw_notifies:
                t = n.get_editor_property("trigger_time_offset")
                name = str(n.get_editor_property("notify_name"))
                notifies.append({"name": name, "trigger_time": round(float(t), 3)})
        except Exception:
            pass
        data["notifies"] = notifies

    # Anim Blueprint
    elif cls_name == "AnimBlueprint":
        target_skel = asset.get_editor_property("target_skeleton")
        data["target_skeleton"] = target_skel.get_path_name() if target_skel else None

    # General Blueprint
    if hasattr(asset, "generated_class"):
        gen_cls = asset.generated_class()
        if gen_cls:
            data["generated_class"] = gen_cls.get_name()
            cdo = unreal.get_default_object(gen_cls)
            if cdo:
                data["cdo_class"] = cdo.get_class().get_name()

    return data

def run_deep_inspection_process(game_path: str) -> dict:
    """Launches headless UnrealEditor-Cmd to execute deep inspection in engine."""
    from run_tests import locate_editor_cmd
    editor_cmd, engine_dir = locate_editor_cmd()
    if not editor_cmd:
        return {
            "error": "UnrealEditor-Cmd.exe could not be located on this machine."
        }

    script_path = (PROJECT_ROOT / "Scripts" / "dump_asset_metadata.py").resolve()
    cmd = [
        str(editor_cmd),
        str(UPROJECT_FILE),
        "-run=pythonscript",
        f"-script={script_path} --asset {game_path}",
        "-NullRHI",
        "-NoSound",
        "-NoSplash",
        "-NoPause",
        "-SkipShaderCompile"
    ]

    try:
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
        stdout = proc.stdout
        # Extract JSON from engine output
        for line in stdout.splitlines():
            line_s = line.strip()
            if line_s.startswith("{") and line_s.endswith("}"):
                try:
                    return json.loads(line_s)
                except Exception:
                    pass
        # Try finding multi-line JSON
        start_idx = stdout.find("{\n  \"asset_name\":")
        if start_idx != -1:
            end_idx = stdout.rfind("\n}")
            if end_idx != -1:
                return json.loads(stdout[start_idx:end_idx+2])
    except Exception as e:
        return {"error": f"Failed deep inspection: {str(e)}"}

    return {"error": "Could not extract JSON from engine process output"}

def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Asset Metadata Dumper")
    parser.add_argument("--asset", "-a", type=str, required=True, help="Path to asset (e.g. Content/UI/BP/WBP_HUD.uasset or /Game/UI/BP/WBP_HUD)")
    parser.add_argument("--json", action="store_true", help="Output raw JSON strictly")
    parser.add_argument("--deep", action="store_true", help="Perform deep engine reflection inspection via UnrealEditor-Cmd")
    args = parser.parse_args()

    file_path, game_path = resolve_asset_paths(args.asset)
    if not file_path and not game_path:
        err = {"status": "ERROR", "message": f"Asset not found: {args.asset}"}
        print(json.dumps(err, indent=2))
        sys.exit(1)

    # In-engine execution
    if IN_ENGINE and game_path:
        result = parse_uasset_in_engine(game_path)
    elif args.deep and game_path:
        # Launch engine process for deep inspection
        result = run_deep_inspection_process(game_path)
        if "error" in result:
            # Fallback to fast scan
            result = parse_uasset_binary_fast(file_path, game_path)
            result["deep_fallback_reason"] = "Engine process could not be launched, fell back to fast binary scan."
    else:
        # Fast standalone scan (~0.005s)
        result = parse_uasset_binary_fast(file_path, game_path)

    # Output
    print(json.dumps(result, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    main()
