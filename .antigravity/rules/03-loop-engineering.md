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

## 2. Execution Sequence

For every code generation or modification task:

```
[ Step 1: Write Code ]
          |
          v
[ Step 2: Build Harness ] ------------> FAILED (Parse JSON errors -> Apply targeted fix -> Re-run)
          |                                (Max 5 cycles)
       SUCCESS
          |
          v
[ Step 3: Verify Replication ] -------> FAILED (Review violations -> Fix replication -> Re-run)
          |
       SUCCESS
          |
          v
[ Step 4: Run Automation Specs ] -----> FAILED (Identify failing spec -> Adjust logic -> Re-run)
          |
       SUCCESS
          |
          v
[ Step 5: Final Clean Summary & Diff Presentation ]
```

---

## 3. Tool Invocations

Subagents and harness scripts interface through the following standardized entry points:

- **Build Harness**:
  ```powershell
  python Scripts/build_harness.py --json
  ```
- **Replication Static Analysis**:
  ```powershell
  python Scripts/verify_replication.py --json
  ```
- **Automated Tests**:
  ```powershell
  python Scripts/run_tests.py --json
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
