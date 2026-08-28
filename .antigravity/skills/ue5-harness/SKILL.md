---
name: ue5-harness
description: Unreal Engine 5.8 Build Harness, Replication Verifier, GAS Verifier, Bandwidth Auditor, TTK Simulator, and Test Runner tools for autonomous loops.
---

# UE5 Harness & Loop Engineering Skill

This skill provides direct terminal entry points and automated diagnostic workflows for compiling, verifying network replication standards, auditing GAS and bandwidth compliance, evaluating TTK balance curves, and executing automation tests in Unreal Engine 5.8.

## Available Actions

1. **Build Project (`ue5_build`)**:
   - Command: `python Scripts/build_harness.py --pretty`
   - Invokes UnrealBuildTool (UBT), parses C2xxx, LNKxxxx, and UHT errors, and returns structured JSON.

2. **Verify Replication (`ue5_verify_replication`)**:
   - Command: `python Scripts/verify_replication.py --pretty`
   - Scans `Source/` for missing `GetLifetimeReplicatedProps`, missing `WithValidation`, missing `bReplicates`, forbidden UI header inclusions in server classes, and improper RepNotify names.

3. **Verify GAS (`ue5_verify_gas`)**:
   - Command: `python Scripts/verify_gas.py --pretty`
   - Statically inspects `UAttributeSet` subclasses for `ATTRIBUTE_ACCESSORS` macros, `OnRep_*` callbacks with `GAMEPLAYATTRIBUTE_REPNOTIFY`, `DOREPLIFETIME_CONDITION_NOTIFY`, and flags UI coupling in `UGameplayAbility`.

4. **Audit Bandwidth (`ue5_audit_bandwidth`)**:
   - Command: `python Scripts/audit_bandwidth.py --pretty`
   - Flags raw `TArray<...>` replication lacking `FFastArraySerializer`, heavy string/map replication, and unconditioned `DOREPLIFETIME` usages.

5. **Simulate Combat TTK (`ue5_simulate_ttk`)**:
   - Command: `python Scripts/simulate_ttk_balance.py --pretty`
   - Calculates effective DPS, hits-to-kill, and TTK curves across armor tiers; supports `--benchmark` and `--markdown`.

6. **Simulate Network PIE Degradation (`ue5_simulate_net_pie`)**:
   - Command: `python Scripts/simulate_net_pie.py --pretty`
   - Spawns headless dedicated server/clients with `-pktlag`, `-pktloss`, and `-pktdup` to detect RPC drops and desync anomalies.

7. **Run Automation Tests (`ue5_run_tests`)**:
   - Command: `python Scripts/run_tests.py --pretty`
   - Executes `UnrealEditor-Cmd.exe` headless automation tests and reports JSON metrics.
