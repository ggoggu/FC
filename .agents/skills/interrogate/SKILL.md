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

### Step 1: Collect Context & State Intent
1. Run `git diff HEAD` (or staged diff) to obtain the exact character-level code changes.
2. State the explicit feature intent in 1-2 paragraphs (what the code is supposed to do and why).

### Step 2: Invoke the Adversarial Auditor Subagent
Invoke the defined `adversarial-auditor` subagent using `invoke_subagent`:
- **TypeName**: `adversarial-auditor`
- **Role**: `Adversarial Code Auditor`
- **Model**: `inherit` or `pro` (use `pro` for complex networking/architectural audits)
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

### Step 3: Triage Findings & Apply Fixes
- If Verdict is **`CHANGES REQUESTED`**:
  - Address all `BLOCKING` issues first.
  - Review `WARNING` and `SUGGESTION` items for practical value.
  - Re-run UBT compilation / static verification via `python Scripts/pipeline.py static`.
- If Verdict is **`APPROVED (LGTM)`**:
  - Present the audit summary to the user.
  - Proceed with commit or testing.
