#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Benchmark Runner: Python Remote Execution vs UnrealEditor-Cmd vs Static File Analysis
Measures precise execution times, memory/overhead, and asset inspection results on UE 5.8.
"""

import os
import sys
import time
import json
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
UPROJECT_PATH = PROJECT_ROOT / "FC.uproject"
UE_CMD_EXE = Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe")
REMOTE_PY_PATH = Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python")

sys.path.append(str(REMOTE_PY_PATH))
import remote_execution

TARGET_BLUEPRINTS = [
    "/Game/Character/Player/BP/BP_FCPlayerCharacter",
    "/Game/Character/Player/BP/BP_FCPlayerController",
    "/Game/Character/Enermy/hm1/BP/BP_FCHM1Character",
    "/Game/Character/Enermy/hm2/BP/BP_FCHM2Character",
    "/Game/Character/Enermy/BP/BP_FixedMobSpawner",
    "/Game/Game/BP_FCGameMode",
    "/Game/Game/BP_FCGameState",
    "/Game/Map/BP_DungeonManager",
    "/Game/Combat/BP/Common/BP_FCProjectile_Common",
    "/Game/UI/BP/WBP_HUD"
]

# The payload to execute inside Unreal Engine Python
IN_ENGINE_ANALYSIS_CODE = """
import unreal
import json
import time

def audit_blueprint(path):
    t0 = time.perf_counter()
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        return {"path": path, "valid": False, "error": "NotFound"}
    
    asset_cls = asset.get_class().get_name()
    is_bp = isinstance(asset, unreal.Blueprint)
    
    parent_name = "None"
    status_str = "None"
    var_list = []
    comp_count = 0
    
    if is_bp:
        parent_cls = asset.get_blueprint_parent_class()
        parent_name = parent_cls.get_name() if parent_cls else "None"
        status_str = str(asset.status)
        try:
            var_list = list(asset.list_member_variable_names())
        except Exception:
            var_list = []
        gen_cls = asset.generated_class()
        cdo = unreal.get_default_object(gen_cls) if gen_cls else None
        if cdo:
            for prop in dir(cdo):
                try:
                    val = getattr(cdo, prop, None)
                    if isinstance(val, unreal.ActorComponent):
                        comp_count += 1
                except Exception:
                    pass

    t1 = time.perf_counter()
    return {
        "path": path,
        "valid": True,
        "asset_name": asset.get_name(),
        "asset_class": asset_cls,
        "is_blueprint": is_bp,
        "parent_class": parent_name,
        "status": status_str,
        "variable_count": len(var_list),
        "variables_sample": var_list[:5],
        "component_count": comp_count,
        "analysis_time_ms": round((t1 - t0) * 1000, 3)
    }

def run_audit(target_paths):
    start = time.perf_counter()
    results = [audit_blueprint(p) for p in target_paths]
    total_time = (time.perf_counter() - start) * 1000
    return {
        "total_engine_time_ms": round(total_time, 2),
        "asset_count": len(results),
        "results": results
    }
"""

def run_via_remote_execution(client, paths):
    """Executes the analysis code over Python Remote Execution."""
    escaped_paths = json.dumps(paths)
    code = f"""
{IN_ENGINE_ANALYSIS_CODE}
final_res = run_audit({escaped_paths})
print("__BENCH_RESULT__" + json.dumps(final_res))
"""
    t0 = time.perf_counter()
    res = client.run_command(code)
    t1 = time.perf_counter()
    wall_ms = round((t1 - t0) * 1000, 2)
    
    # Parse output
    parsed_data = None
    for entry in res.get('output', []):
        text = entry.get('output', '')
        if '__BENCH_RESULT__' in text:
            raw_json = text.split('__BENCH_RESULT__')[1].strip()
            parsed_data = json.loads(raw_json)
            break
            
    return {
        "success": res.get("success", False),
        "wall_time_ms": wall_ms,
        "engine_data": parsed_data
    }

def run_via_cmdlet(paths):
    """Executes the analysis code by spawning UnrealEditor-Cmd.exe."""
    escaped_paths = json.dumps(paths)
    runner_script = PROJECT_ROOT / "Intermediate" / "temp_cmdlet_audit.py"
    runner_script.parent.mkdir(parents=True, exist_ok=True)
    
    script_content = f"""
{IN_ENGINE_ANALYSIS_CODE}
final_res = run_audit({escaped_paths})
print("__BENCH_RESULT__" + json.dumps(final_res))
"""
    runner_script.write_text(script_content, encoding='utf-8')
    
    cmd = [
        str(UE_CMD_EXE),
        str(UPROJECT_PATH),
        "-run=pythonscript",
        f"-script={runner_script}",
        "-unattended",
        "-nopause",
        "-nosplash",
        "-nullrhi"
    ]
    
    t0 = time.perf_counter()
    proc = subprocess.run(cmd, capture_output=True, text=True)
    t1 = time.perf_counter()
    wall_ms = round((t1 - t0) * 1000, 2)
    
    parsed_data = None
    for line in proc.stdout.splitlines():
        if '__BENCH_RESULT__' in line:
            raw_json = line.split('__BENCH_RESULT__')[1].strip()
            parsed_data = json.loads(raw_json)
            break
            
    return {
        "success": proc.returncode == 0 and parsed_data is not None,
        "wall_time_ms": wall_ms,
        "return_code": proc.returncode,
        "engine_data": parsed_data
    }

def run_via_static_file(paths):
    """Reads uasset raw headers from disk (no engine)."""
    t0 = time.perf_counter()
    results = []
    for p in paths:
        rel_path = p.replace("/Game/", "Content/").replace("/", "\\") + ".uasset"
        disk_path = PROJECT_ROOT / rel_path
        if disk_path.exists():
            size = disk_path.stat().st_size
            results.append({"path": p, "exists": True, "size_bytes": size})
        else:
            results.append({"path": p, "exists": False})
    t1 = time.perf_counter()
    wall_ms = round((t1 - t0) * 1000, 3)
    return {
        "wall_time_ms": wall_ms,
        "results": results
    }

def main():
    print("=================================================================")
    print("  FC Project: Python Remote Execution vs Offline Benchmark")
    print("=================================================================")
    
    # 1. Connect to Remote Execution
    print("\n[Step 1] Connecting to active Unreal Editor via Remote Execution...")
    client = remote_execution.RemoteExecution()
    client.start()
    for _ in range(10):
        time.sleep(0.3)
        if client.remote_nodes:
            break
            
    if not client.remote_nodes:
        print("[ERROR] No running Unreal Editor found with Remote Execution enabled!")
        sys.exit(1)
        
    node = client.remote_nodes[0]
    client.open_command_connection(node['node_id'])
    print(f"Connected to Editor Node: {node['node_id']} (Project: {node.get('project_name')})")

    benchmark_data = {
        "metadata": {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "engine_version": node.get("engine_version", "5.8"),
            "project_name": node.get("project_name", "FC"),
            "machine": node.get("machine"),
            "os": "Windows"
        },
        "tests": {}
    }

    # -------------------------------------------------------------
    # Test 1: Single Blueprint Deep Inspection
    # -------------------------------------------------------------
    single_bp = ["/Game/Character/Player/BP/BP_FCPlayerCharacter"]
    print("\n--- Running Test 1: Single Blueprint Deep Inspection ---")
    
    print("  1-1. Python Remote Execution (Live Editor)...")
    res_remote_single = run_via_remote_execution(client, single_bp)
    print(f"       -> Wall Time: {res_remote_single['wall_time_ms']} ms | Engine Time: {res_remote_single['engine_data']['total_engine_time_ms']} ms")

    print("  1-2. UnrealEditor-Cmd (Offline Process)...")
    res_cmdlet_single = run_via_cmdlet(single_bp)
    print(f"       -> Wall Time: {res_cmdlet_single['wall_time_ms']} ms | Engine Time: {res_cmdlet_single['engine_data']['total_engine_time_ms'] if res_cmdlet_single['engine_data'] else 'N/A'} ms")

    print("  1-3. Raw Disk Static Read (Baseline)...")
    res_static_single = run_via_static_file(single_bp)
    print(f"       -> Wall Time: {res_static_single['wall_time_ms']} ms")

    benchmark_data["tests"]["single_asset"] = {
        "asset": single_bp[0],
        "remote_execution": res_remote_single,
        "cmdlet": res_cmdlet_single,
        "static_read": res_static_single,
        "speedup_ratio": round(res_cmdlet_single['wall_time_ms'] / max(res_remote_single['wall_time_ms'], 0.001), 1)
    }

    # -------------------------------------------------------------
    # Test 2: Multi-Asset Audit (10 Core Blueprints)
    # -------------------------------------------------------------
    print("\n--- Running Test 2: Multi-Asset Audit (10 Core Blueprints) ---")
    
    print("  2-1. Python Remote Execution (Live Editor)...")
    res_remote_multi = run_via_remote_execution(client, TARGET_BLUEPRINTS)
    print(f"       -> Wall Time: {res_remote_multi['wall_time_ms']} ms | Engine Time: {res_remote_multi['engine_data']['total_engine_time_ms']} ms")

    print("  2-2. UnrealEditor-Cmd (Offline Process)...")
    res_cmdlet_multi = run_via_cmdlet(TARGET_BLUEPRINTS)
    print(f"       -> Wall Time: {res_cmdlet_multi['wall_time_ms']} ms | Engine Time: {res_cmdlet_multi['engine_data']['total_engine_time_ms'] if res_cmdlet_multi['engine_data'] else 'N/A'} ms")

    print("  2-3. Raw Disk Static Read...")
    res_static_multi = run_via_static_file(TARGET_BLUEPRINTS)
    print(f"       -> Wall Time: {res_static_multi['wall_time_ms']} ms")

    benchmark_data["tests"]["multi_asset"] = {
        "assets_count": len(TARGET_BLUEPRINTS),
        "remote_execution": res_remote_multi,
        "cmdlet": res_cmdlet_multi,
        "static_read": res_static_multi,
        "speedup_ratio": round(res_cmdlet_multi['wall_time_ms'] / max(res_remote_multi['wall_time_ms'], 0.001), 1)
    }

    # -------------------------------------------------------------
    # Test 3: Iterative Execution (5 Sequential Asset Queries)
    # -------------------------------------------------------------
    print("\n--- Running Test 3: Iterative 5-Cycle Dev Loop ---")
    iter_assets = TARGET_BLUEPRINTS[:5]
    
    print("  3-1. Python Remote Execution (5 sequential queries on persistent socket)...")
    t0_iter_remote = time.perf_counter()
    remote_iter_times = []
    for bp in iter_assets:
        r = run_via_remote_execution(client, [bp])
        remote_iter_times.append(r['wall_time_ms'])
    total_remote_iter_ms = round((time.perf_counter() - t0_iter_remote) * 1000, 2)
    print(f"       -> Total 5-Cycle Wall Time: {total_remote_iter_ms} ms (Avg: {round(total_remote_iter_ms/5, 2)} ms/query)")

    print("  3-2. Estimating Cmdlet for 5 sequential queries...")
    # Each cmdlet execution is independent because there is no persistent socket in offline mode
    estimated_cmdlet_iter_ms = round(res_cmdlet_single['wall_time_ms'] * 5, 2)
    print(f"       -> Total 5-Cycle Estimated Time: {estimated_cmdlet_iter_ms} ms (~{round(estimated_cmdlet_iter_ms/1000, 1)} s)")

    benchmark_data["tests"]["iterative_loop"] = {
        "iterations": 5,
        "remote_total_ms": total_remote_iter_ms,
        "remote_avg_ms": round(total_remote_iter_ms / 5, 2),
        "remote_cycle_times": remote_iter_times,
        "cmdlet_estimated_ms": estimated_cmdlet_iter_ms,
        "speedup_ratio": round(estimated_cmdlet_iter_ms / max(total_remote_iter_ms, 0.001), 1)
    }

    # Close connection
    client.close_command_connection()
    client.stop()

    # Save results to JSON
    out_json = PROJECT_ROOT / "Report" / "benchmark_remote_vs_cmdlet.json"
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(benchmark_data, indent=2, ensure_ascii=False), encoding='utf-8')
    print(f"\n[DONE] Benchmark completed! Results saved to: {out_json}")

if __name__ == "__main__":
    main()
