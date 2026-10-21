#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Orchestrates the generation of all 3 volumes of the Antigravity Architecture Report series:
1. FC_Antigravity_Vol1_Platform_and_Core_Runtime.pdf
2. FC_Antigravity_Vol2_Customization_and_Extensibility.pdf
3. FC_Antigravity_Vol3_FC_Project_Engineering_and_Loop.pdf

Outputs are placed in Report/Architecture/ (and accessible via Report/Archtecture/).
"""

import sys
import subprocess
from pathlib import Path

SCRIPTS_DIR = Path(__file__).parent

def run_script(script_name: str):
    script_path = SCRIPTS_DIR / script_name
    print(f"\n=======================================================")
    print(f"Executing: {script_name}")
    print(f"=======================================================")
    res = subprocess.run([sys.executable, str(script_path)])
    if res.returncode != 0:
        print(f"[ERROR] {script_name} failed with exit code {res.returncode}")
        sys.exit(res.returncode)

def main():
    print("Starting Antigravity Architecture Trilogy Generation...")
    run_script("generate_antigravity_vol1.py")
    run_script("generate_antigravity_vol2.py")
    run_script("generate_antigravity_vol3.py")
    print("\n=======================================================")
    print("[ALL SUCCESS] All 3 volumes generated successfully in Report/Architecture/")
    print("=======================================================")

if __name__ == "__main__":
    main()
