# Loop Engineering & Autonomous Self-Healing Protocols

This document specifies the operational protocols for autonomous agents executing modifications, builds, static replication verification, worktree isolation, and loop memory management.

---

## 1. Autonomous Self-Healing Principles

1. **No Unnecessary Human Interruption**:
   - When a build fails via `Scripts/build_harness.py` or replication analysis fails via `Scripts/verify_replication.py`, do not ask the user for manual fixes or stop prematurely.
   - Autonomous agents must parse the structured JSON diagnostic output, locate the offending files and lines, reason about the root cause, and apply targeted patches immediately.

2. **Self-Healing Loop Budget & Oscillation Circuit Breaker**:
   - Up to **5 iterative self-correction cycles** are allocated per task for resolving build/replication errors.
   - **Oscillation Circuit Breaker (Ralph Loop Guard)**: If the exact same error fingerprint `(file, line, error_code/rule)` recurs **2 consecutive times** despite patching, halt blind retries immediately and execute a **Reflexion Step** (explicitly state why the previous hypothesis failed and inspect Ground Truth headers/assets before attempting a new structural fix).
   - **Anti-Reward-Hacking Guard**: Never weaken, delete, or comment out verification rules in `Scripts/verify_*.py`, `Scripts/audit_bandwidth.py`, or unit test assertions to force a passing status.
   - If after 5 iterations (or circuit-breaker escalation) the build or static verification is still failing, halt the loop and output a detailed diagnosis containing:
     - The persistent error codes (`C2xxx`, `LNKxxxx`, UHT errors).
     - The files and line numbers where the error recurs.
     - Hypotheses on the root cause and recommended structural adjustments.

---

## 2. Execution Sequence & Tiered Modes (ACI)

- **Default (Fast Mode)**: Regular coding and Q&A run with zero script overhead.
- **Incremental File-Scoped ACI (`--file`)**: For ultra-fast (~0.03s) pinpoint verification of a single modified C++ header/source pair without scanning the entire repository.
- **On-Demand Pipelines**: When verification is requested, invoke the master runner `Scripts/pipeline.py`:

```
[ Step 1: Write Code / Refactor ]
          |
          v
[ On-Demand: python Scripts/pipeline.py <mode> ]
  - 'static'  : GAS, Bandwidth, Replication Static Verification (~0.4s full / ~0.03s with --file)
  - 'build'   : UBT compile harness + JSON self-healing loop (Max 5 cycles, Circuit Breaker on 2x repeat)
  - 'balance' : Mathematical TTK and effective DPS curve simulation (~0.2s)
  - 'test'    : Headless automation specs
  - 'audit'   : C++ diff packaging & static checks for /interrogate SubAgent review
  - 'full'    : Static -> Build -> Test -> [Auto-Commit]
```

---

## 3. Tool Invocations

The project provides a unified master runner for all verification tasks:

- **Incremental Single-File Static Check (ACI Fast Loop)**:
  ```powershell
  python Scripts/pipeline.py static --file Source/FC/Public/Map/FCDoorBase.h
  ```
- **Static Verification**:
  ```powershell
  python Scripts/pipeline.py static
  ```
- **UBT Build & Patch Loop**:
  ```powershell
  python Scripts/pipeline.py build
  ```
- **Combat / TTK Balance**:
  ```powershell
  python Scripts/pipeline.py balance
  ```
- **Full Release Pipeline**:
  ```powershell
  python Scripts/pipeline.py full --auto-commit -m "<conventional_commit_msg>"
  ```

---

## 4. Error Parsing & Triage Guide

| Error Type | Common Source | Automated Resolution Strategy |
| :--- | :--- | :--- |
| **`C2065: undeclared identifier`** | Missing header / forward declaration | Check type name and add required `#include` in `.cpp` or forward declare in `.h`. |
| **`UHT Error: Missing WithValidation`** | `UFUNCTION(Server, ...)` without `WithValidation` | Add `WithValidation` to specifiers and declare both `_Implementation` and `_Validate`. |
| **`UHT Error: Replicated property missing LifetimeProps`** | Replicated variable declared without `GetLifetimeReplicatedProps` | Implement `GetLifetimeReplicatedProps` with `DOREPLIFETIME_CONDITION`. |
| **`LNK2019 / LNK2001: unresolved external symbol`** | Missing Module dependency in `Build.cs` or unimplemented member | Add missing module to `FC.Build.cs` (e.g. `NetCore`, `EnhancedInput`) or define the function body. |
| **Replication Verification Failure** | Server header includes widget or missing `bReplicates` | Move widget includes to `.cpp` / use forward declaration; ensure `bReplicates = true;` in constructor. |
| **`REPL_BYPASSED_AUTHORITY_SETTER`** | `Replicated` + `BlueprintReadWrite` without `BlueprintSetter` | Add `BlueprintSetter = Set<PropertyName>` or change to `BlueprintReadOnly` to enforce server authority. |

---

## 5. Editor Asset Inspection Protocol (BP / WBP / Animation)

To guarantee 100% reasoning accuracy and eliminate hallucinations when working with binary `.uasset` files:

1. **Mandatory Ground Truth Inspection**:
   - Before reasoning about or modifying code that interfaces with existing Blueprints (`BP_*`), Widget Blueprints (`WBP_*`), or Animation assets (`MM_*`, `ABP_*`), autonomous agents **MUST** inspect the asset metadata using:
     ```powershell
     python Scripts/pipeline.py inspect --asset <path>
     ```
     *(or append `--deep` for live reflection, `--no-cache` to bypass `mtime` schema cache)*
2. **Extracted Schema Utilization**:
   - **Widget Blueprints (WBP)**: Cross-reference `detected_widgets`, `detected_viewmodels`, and `detected_bindings` with C++ `UMVVMViewModelBase` / `FieldNotify` properties.
   - **AnimMontages & AnimBlueprints**: Use `detected_notifies`, `detected_slots`, and `detected_tracks` to verify exact notify names, gameplay tags, and target skeletal slots without guessing.
   - **Actor Blueprints (BP)**: Verify `detected_components`, `detected_variables`, and parent C++ classes before adding or binding components.
3. **Deep Engine Reflection Fallback**:
   - When exact property values or runtime CDO numbers are required, append `--deep` to execute headless reflection extraction via Unreal Engine.

---

## 6. Worktree Isolation Protocol (`Workspace="share"`) for UE 5.8

When orchestrating subagents across isolated workspaces in Unreal Engine 5.8, adhere strictly to the following three rules to prevent `.generated.h` environment drift, UBT lock contention, and binary merge conflicts:

1. **Read-Only Subagents (`Workspace="inherit"`)**:
   - Code auditors (`adversarial-auditor` via `/interrogate`) and codebase researchers (`research`) must always run with `Workspace="inherit"` and `EnableWriteTools=false` for zero filesystem overhead and immediate access to uncommitted diffs.
2. **Parallel C++ Implementation (`Workspace="share"` / Git Worktree)**:
   - **Upfront Disjoint File Ownership**: Before spawning parallel write-enabled subagents in `share` worktrees, lock shared contract headers (`FC.Build.cs`, `FCGameplayTags.h`, core interfaces) in the main workspace and assign non-overlapping directory ownership per worktree (e.g., Worktree A owns `Source/FC/*/Dungeon/*`, Worktree B owns `Source/FC/*/AbilitySystem/*`).
   - **Two-Stage Verification Gate (Worktree Static -> Main Build)**: Because `git worktree` does not copy git-ignored `Intermediate/*.generated.h` or `Binaries/`, subagents inside `share` worktrees must validate via AST static verification (`python Scripts/pipeline.py static`) and `/interrogate`. Full UHT/UBT compilation (`pipeline.py build`) is executed once in the main (`inherit`) workspace after integrating the worktree changes.
3. **Binary `.uasset` & Live Editor MCP Restriction**:
   - Never mutate binary `.uasset`/`.umap` files or invoke Live Editor MCP (`unreal-engine` port 8000) from a `share` or `branch` worktree. All binary asset mutations are strictly pinned to the primary `inherit` workspace bound to the active Unreal Editor instance.

---

## 7. Multi-Tier Loop Memory & Rule-to-Linter Promotion

To prevent both **Session Amnesia (Compaction Cliff)** and **Prompt Bloat (Context Rot)**, the project enforces a structured loop memory hierarchy:

1. **Working State Anchoring (`scratch/loop_state.json`)**:
   - During long-horizon or multi-step debugging tasks, persist active state (`current_goal`, `verified_assets`, `failed_hypotheses`, `pending_files`) in `<conversation-id>/scratch/loop_state.json` so critical context survives `/compact` summarization.
2. **Asset Schema Cache Memory (`Saved/AssetCache/asset_schema_cache.json`)**:
   - `dump_asset_metadata.py` automatically caches extracted `.uasset` schemas keyed by file path, `mtime_ns`, and file size. Unchanged assets return verified Ground Truth metadata in `0ms` without re-parsing binary packages or re-spawning headless editors.
3. **Rule-to-Linter Promotion (Zero-Token Permanent Memory)**:
   - Do not accumulate an unbounded markdown list of past mistakes. Whenever a recurring structural defect is caught by `/interrogate` or UBT self-healing (e.g., `Replicated` + `BlueprintReadWrite` missing `BlueprintSetter`, or raw pointer `UPROPERTY`), **promote the lesson directly into a deterministic Python check** in `Scripts/verify_replication.py`, `Scripts/verify_gas.py`, or `Scripts/audit_bandwidth.py`.
   - Once codified in the static harness, the constraint is enforced in `~0.03s` with **0 prompt token overhead**.
