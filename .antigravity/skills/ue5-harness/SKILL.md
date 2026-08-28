---
name: ue5-harness
description: Unreal Engine 5.8 Build Harness, Replication Verifier, and Test Runner tools for autonomous loops.
---

# UE5 Harness & Loop Engineering Skill

This skill provides direct terminal entry points and automated diagnostic workflows for compiling, verifying network replication standards, and executing automation tests in Unreal Engine 5.8.

## Available Actions

1. **Build Project (`ue5_build`)**:
   - Command: `python Scripts/build_harness.py --pretty`
   - Invokes UnrealBuildTool (UBT), parses C2xxx, LNKxxxx, and UHT errors, and returns structured JSON.

2. **Verify Replication (`ue5_verify_replication`)**:
   - Command: `python Scripts/verify_replication.py --pretty`
   - Scans `Source/` for missing `GetLifetimeReplicatedProps`, missing `WithValidation`, missing `bReplicates`, forbidden UI header inclusions in server classes, and improper RepNotify names.

3. **Run Automation Tests (`ue5_run_tests`)**:
   - Command: `python Scripts/run_tests.py --pretty`
   - Executes `UnrealEditor-Cmd.exe` headless automation tests and reports JSON metrics.
