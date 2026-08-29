# Loop Engineering & Autonomous Self-Healing Protocols

This document specifies the operational protocols for autonomous agents executing modifications, builds, static replication verification, and automated tests.

---

## 1. Autonomous Self-Healing Principles

1. **No Unnecessary Human Interruption**:
   - When a build fails via `Scripts/build_harness.py` or replication analysis fails via `Scripts/verify_replication.py`, do not ask the user for manual fixes or stop prematurely.
   - Autonomous agents must parse the structured JSON diagnostic output, locate the offending files and lines, reason about the root cause, and apply targeted patches immediately.

2. **Self-Healing Loop Budget**:
   - Up to **5 iterative self-correction cycles** are allocated per task for resolving build/replication errors.
   - If after 5 iterations the build or static verification is still failing, halt the loop and output a detailed diagnosis containing:
     - The persistent error codes (`C2xxx`, `LNKxxxx`, UHT errors).
     - The files and line numbers where the error recurs.
     - Hypotheses on the root cause and recommended structural adjustments.

---

## 2. Execution Sequence & Tiered Modes

- **Default (Fast Mode)**: Regular coding and Q&A run with zero script overhead.
- **On-Demand Pipelines**: When verification is requested, invoke the master runner `Scripts/pipeline.py`:

```
[ Step 1: Write Code / Refactor ]
          |
          v
[ On-Demand: python Scripts/pipeline.py <mode> ]
  - 'static'  : GAS, Bandwidth, Replication Static Verification (~0.4s)
  - 'build'   : UBT compile harness + JSON self-healing loop (Max 5 cycles)
  - 'balance' : Mathematical TTK and effective DPS curve simulation (~0.2s)
  - 'test'    : Headless automation specs
  - 'full'    : Static -> Build -> Test -> [Auto-Commit]
```

---

## 3. Tool Invocations

The project provides a unified master runner for all verification tasks:

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
