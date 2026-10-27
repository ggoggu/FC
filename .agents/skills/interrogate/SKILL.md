---
name: interrogate
description: >-
  Trigger an adversarial code review and architecture audit using an independent read-only subagent.
  Use this skill whenever the user asks for code review, adversarial review, code audit,
  inspection before committing, checking network/replication optimizations, or verifying conciseness and architecture reuse.
---

# /interrogate - Adversarial Code & Architecture Audit

This skill invokes an independent, read-only subagent (`adversarial-auditor`) to ruthlessly critique recent code changes against the FC project's 5 Core Engineering Dimensions before committing.

## When to Run

- After implementing a new gameplay feature, component, or system.
- Before committing or merging code (`python Scripts/pipeline.py full`).
- Whenever the user prompts: `"검사해줘"`, `"/interrogate"`, `"코드 리뷰해줘"`, `"최적화 검사해줘"`, or `"다른 모델(서브에이전트)에 확인받아줘"`.

---

## The 5 Core Audit Dimensions

1. **Code Conciseness (코드 간결성)**
   - Minimal surface area changes; no superfluous boilerplate or premature over-engineering.
2. **Reuse of Existing Architecture & Code (기존 구조 및 코드 활용)**
   - Strict utilization of existing subsystems, utilities, components, and data tables.
3. **Scoped Extensibility (범위 내 적정 확장성)**
   - Clean, modular design sized for the specified feature requirements without speculative YAGNI violations.
4. **Network & Replication Optimization (네트워크 최적화)**
   - Strict Server Authority, RPC Ownership, `DOREPLIFETIME_CONDITION`, RepNotify over tick RPCs, and FastArraySerializer.
5. **General Performance Optimization (일반 최적화)**
   - Zero hot-path heap allocations, `bCanEverTick = false` by default, proper `TObjectPtr` & GC lifecycle management.

---

## Execution Workflow

### Step 1: Collect Context & State Intent (Filtering & Staging)
1. Run `git add -N .` to ensure newly created untracked C++ files are recognized by git diff.
2. Extract the text-only C++ diff (excluding binary `.uasset` / `.umap` assets):
   ```powershell
   git diff HEAD -- "*.h" "*.cpp" "*.cs"
   ```
   *(Or run `python Scripts/pipeline.py audit --json` to get an aggregated diff & static replication status).*
3. State the explicit feature intent in 1-2 paragraphs (what the code is supposed to do and why).

### Step 2: Invoke the Adversarial Auditor Subagent
Select the appropriate model tier based on the audit scope:
- **`flash`**: Fast iterative checks on small changes (<150 diff lines, local component logic).
- **`pro`** (Recommended for Multiplayer/Architecture): Core replication changes, RPC declarations, GAS abilities/attributes, or large architectural refactors.

Invoke the defined `adversarial-auditor` subagent using `invoke_subagent`:
- **TypeName**: `adversarial-auditor`
- **Role**: `Adversarial Code Auditor`
- **Model**: `pro` (or `flash` for fast track)
- **Prompt**:
  ```text
  Please perform a rigorous adversarial audit on the following changes.
  
  ## Stated Intent
  <Brief summary of intent>
  
  ## Code Changes (Git Diff / Targeted Files)
  <Git diff or file contents>
  
  Audit the code strictly against the 5 dimensions:
  1. Code Conciseness
  2. Architecture & Existing Code Reuse
  3. Scoped Extensibility
  4. Network & Replication Optimization
  5. General Performance Optimization
  
  Provide the structured Markdown report with your Verdict.
  ```

### Step 3: Triage Findings & Closed-Loop Remediation
- If Verdict is **`CHANGES REQUESTED (BLOCKING)`**:
  - The main agent must **autonomously resolve all `BLOCKING` issues immediately** (e.g. adding missing `HasAuthority()` guards, changing `COND_None` to `COND_InitialOnly`, fixing RPC ownership).
  - Review `WARNING` and `SUGGESTION` items for practical value and apply where clean.
  - Re-verify using `python Scripts/pipeline.py static` (or `pipeline.py build`).
  - Present the audit findings alongside the applied fix comparison to the user.
- If Verdict is **`APPROVED (LGTM)`**:
  - Present the audit summary to the user.
  - Proceed with commit or testing (`python Scripts/pipeline.py full`).
