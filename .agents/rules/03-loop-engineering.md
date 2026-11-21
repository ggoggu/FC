# 03. Loop Engineering, Hooks & Autonomous Self-Healing

## 1. Self-Healing Loop & Circuit Breaker
1. **Autonomous Remediation**: On `pipeline.py static` or `build` failures, parse the structured diagnostics (`RuleID`, `File:Line`, `Fix`, `Ref`), reason about the root cause, and patch immediately without stalling for manual help.
2. **Loop Budget & Oscillation Circuit Breaker (`Saved/LoopMemory/last_error_state.json`)**:
   - Budget: max **5 iterative self-correction cycles** per task.
   - `pipeline.py` computes **line-number-invariant** fingerprints `(file, rule_or_code)` scoped per target (`static::<file>` or `build`).
   - **Breaker Trigger**: If the same fingerprint repeats **2 consecutive times** (or any **3 consecutive failures** occur on the same target), halt blind patching immediately and run a **Reflexion Step** (state why the prior hypothesis failed and re-inspect Ground Truth headers/assets before changing strategy).
3. **Mechanical Anti-Reward-Hacking Guard**:
   - Never weaken, delete, or comment out checks in `Scripts/verify_*.py`, `Scripts/audit_bandwidth.py`, `Scripts/linter_baseline.json`, or test assertions.
   - Enforced mechanically by `.agents/hooks.json` (`PreToolUse` `guard_verifiers.py` -> `force_ask`), `.agents/hooks/stop_static_gate.py` (`Stop` -> `continue` on unverified C++ errors), and `.git/hooks/pre-commit`.

## 2. Deterministic Verification Sequence
```
[ Edit C++ (.h/.cpp) ]
         |
         v
[ Incremental Static Gate (~0.1s) ]
  python Scripts/pipeline.py static --file <path>
         |
         v
[ Repo-Wide / Compile / Runtime Verification ]
  - static  : GAS + Bandwidth + Replication (all Hard Constraints = ERROR)
  - build   : UBT compile harness + JSON diagnostics + Circuit Breaker
  - test    : Headless UE Automation Specs (`FC.*`; fails on 0 matched tests)
  - net     : Network degradation run (`-pktlag/-pktloss/-pktdup`; fails on missing map/timeout/0 tests)
  - audit   : C++ diff + FULL static check (GAS + Bandwidth + Replication) for `/interrogate`
  - full    : static -> build -> test -> [--auto-commit]
```
- **Harness Self-Test**: Whenever harness scripts or hooks are legitimately updated, run `python Scripts/tests/test_linters.py`.

## 3. Error Triage Quick Reference
| Error Pattern | Root Cause | Resolution |
| :--- | :--- | :--- |
| `C2065` | Missing include / forward decl | Add `#include` in `.cpp` or forward declare in `.h`. |
| `LNK2019 / LNK2001` | Missing module in `FC.Build.cs` or undefined symbol | Add module dependency or implement body. |
| `CODE_UNSAFE_RAW_THIS_LAMBDA` | `.AddLambda([this])` / `.BindLambda([this])` | Switch to `.AddWeakLambda(this, ...)` / `.BindWeakLambda(this, ...)`. |
| `CODE_HARDCODED_ASSET_PATH` | New `ConstructorHelpers::FObjectFinder`/`FClassFinder` | Use `TSoftObjectPtr`, `UPrimaryDataAsset`, or BP Class Defaults. |
| `REPL_BYPASSED_AUTHORITY_SETTER` | `Replicated` + `BlueprintReadWrite` missing `BlueprintSetter` | Add `BlueprintSetter = Set<Prop>` or use `BlueprintReadOnly`. |

## 4. Editor Asset Inspection Protocol (BP / WBP / Anim / DT)
Never hallucinate `.uasset` internals. Follow strict priority:
1. **Priority 1 — Live Editor MCP (`wildcard-mcp`, Editor running)**:
   - Call `list_toolsets` / `describe_toolset` / `call_tool` on `wildcard-mcp`.
   - Prefer `fc_toolset.FCTools.inspect_asset_summary`, `UMGToolSet.UMGToolSet` (+ `ObjectTools.list_properties` before property reads/writes), `MVVMToolset.UMVVMToolset`, and `editor_toolset.toolsets.blueprint.BlueprintTools`.
2. **Priority 2 — Offline Fast Scan (Editor closed)**:
   - `python Scripts/pipeline.py inspect --asset <path>` (`Saved/AssetCache/asset_schema_cache.json` `mtime_ns` cache; add `--no-cache` to refresh or `--deep` for headless reflection when `wildcard-mcp` is unavailable).

## 5. Worktree Isolation (`Workspace="share"`) for UE 5.8
1. **Read-Only Subagents (`Workspace="inherit"`)**: `/interrogate` (`adversarial-auditor`) and `research` always run with `Workspace="inherit"` and `EnableWriteTools=false`.
2. **Parallel C++ Worktrees (`Workspace="share"`)**: Lock shared contract headers (`FC.Build.cs`, shared types) upfront, assign disjoint directory ownership, validate inside worktrees via `pipeline.py static` + `/interrogate`, and run UBT `pipeline.py build` once after integrating into the main workspace.
3. **Binary `.uasset` / Live MCP Pinning**: Never mutate `.uasset`/`.umap` or invoke `wildcard-mcp` from a `share`/`branch` worktree—pin all live editor actions to the primary `inherit` workspace.

## 6. Rule-to-Linter Promotion & Frozen Legacy Baseline
- Do not pile prose warnings into `AGENTS.md`. Promote recurring defects into deterministic checks in `Scripts/verify_*.py` or `Scripts/audit_bandwidth.py` + regression tests in `Scripts/tests/test_linters.py`.
- Pre-existing legacy `ConstructorHelpers` debt is explicitly tracked in `Scripts/linter_baseline.json` (inspect full debt anytime via `python Scripts/verify_replication.py --ignore-baseline --pretty`). Never add new entries to the baseline.
