#!/usr/bin/env python3
"""
Shared Unreal Engine 5.8 environment discovery helpers for FC harness runners.
Centralizes .uproject discovery, Windows registry lookup, UBT, and UnrealEditor-Cmd resolution.
"""

import os
import sys
from pathlib import Path

DEFAULT_ENGINE_CANDIDATES = [
    r"C:\Program Files\Epic Games\UE_5.8",
    r"C:\Program Files\Epic Games\UE_5.7",
    r"C:\Program Files\Epic Games\UE_5.6",
    r"C:\Program Files\Epic Games\UE_5.5",
]

ENGINE_ENV_VARS = (
    "UE_5_8_PATH",
    "UNREAL_ENGINE_PATH",
    "UE_ROOT",
    "ENGINE_PATH",
    "UE_ENGINE_DIR",
)


def find_uproject_file(start_dir: Path) -> Path | None:
    current = start_dir.resolve()
    for _ in range(5):
        uprojects = list(current.glob("*.uproject"))
        if uprojects:
            return uprojects[0]
        if current.parent == current:
            break
        current = current.parent
    return None


def query_registry_engine_paths() -> list[str]:
    paths: list[str] = []
    if sys.platform != "win32":
        return paths
    try:
        import winreg

        for root_key in (winreg.HKEY_LOCAL_MACHINE, winreg.HKEY_CURRENT_USER):
            try:
                with winreg.OpenKey(root_key, r"SOFTWARE\EpicGames\Unreal Engine") as key:
                    num_subkeys = winreg.QueryInfoKey(key)[0]
                    for i in range(num_subkeys):
                        subkey_name = winreg.EnumKey(key, i)
                        with winreg.OpenKey(key, subkey_name) as subkey:
                            try:
                                installed_dir, _ = winreg.QueryValueEx(subkey, "InstalledDirectory")
                                if installed_dir and os.path.exists(installed_dir):
                                    paths.append(installed_dir)
                            except OSError:
                                pass
            except OSError:
                pass
    except Exception:
        pass
    return paths


def _collect_engine_search_dirs(explicit_engine_dir: str | None = None) -> list[Path]:
    search_dirs: list[Path] = []
    if explicit_engine_dir:
        search_dirs.append(Path(explicit_engine_dir))

    for env_var in ENGINE_ENV_VARS:
        val = os.environ.get(env_var)
        if val and os.path.exists(val):
            p = Path(val)
            if p not in search_dirs:
                search_dirs.append(p)

    for candidate in DEFAULT_ENGINE_CANDIDATES:
        if os.path.exists(candidate):
            p = Path(candidate)
            if p not in search_dirs:
                search_dirs.append(p)

    for reg_path in query_registry_engine_paths():
        p = Path(reg_path)
        if p.exists() and p not in search_dirs:
            search_dirs.append(p)

    return search_dirs


def locate_editor_cmd(explicit_engine_dir: str | None = None) -> tuple[Path | None, Path | None]:
    platform_dir = "Win64" if sys.platform == "win32" else "Linux"
    binary_name = "UnrealEditor-Cmd.exe" if sys.platform == "win32" else "UnrealEditor-Cmd"
    for engine_dir in _collect_engine_search_dirs(explicit_engine_dir):
        editor_cmd = engine_dir / "Engine" / "Binaries" / platform_dir / binary_name
        if editor_cmd.is_file():
            return editor_cmd, engine_dir
    return None, None


def locate_ubt(explicit_engine_dir: str | None = None) -> tuple[Path | None, Path | None]:
    batch_name = "Build.bat" if sys.platform == "win32" else "Build.sh"
    for engine_dir in _collect_engine_search_dirs(explicit_engine_dir):
        ubt_batch = engine_dir / "Engine" / "Build" / "BatchFiles" / batch_name
        if ubt_batch.is_file():
            return ubt_batch, engine_dir
        ubt_path = engine_dir / "Engine" / "Binaries" / "DotNET" / "UnrealBuildTool" / "UnrealBuildTool.exe"
        if ubt_path.is_file():
            return ubt_path, engine_dir
    return None, None
