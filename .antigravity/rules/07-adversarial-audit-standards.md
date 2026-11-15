# 07. Adversarial Code Audit & Optimization Standards

This document establishes the official review rubric used by the **`adversarial-auditor`** subagent and code review workflows (`/interrogate`) in the FC project.

All proposed code changes and refactorings are evaluated against the following **5 Core Dimensions**:

---

## 1. Code Conciseness (코드 간결성 & 최소 수정 원칙)

- **Minimal Surface Area**: Strive for the smallest clean functional diff required to fulfill the feature or fix the bug.
- **No Premature Over-Engineering**: Avoid boilerplate wrappers, single-use helper classes, excessive indirection, or redundant pass-through layers.
- **Clarity & Idiomatic C++20**: Favor readable, standard Unreal Engine idioms over deeply nested templates or obscure metaprogramming.

---

## 2. Reuse of Existing Architecture & Code (기존 구조 및 코드 활용)

- **Leverage Existing Systems**: Before creating any new subsystem, component, manager, or utility, verify whether an existing class in `Source/FC/` or the engine handles it.
- **No Parallel Workflows**: Prohibit creating duplicate data pipelines, redundant state stores, or parallel event dispatches when an established pattern already exists in the codebase.
- **Cooked Asset SSOT**: Gameplay data must originate from existing Engine Assets (`UDataTable`, `UPrimaryDataAsset`, `DeveloperSettings`). Direct loose file I/O (`FPaths::ProjectContentDir() / *.csv`) is forbidden.

---

## 3. Scoped Extensibility (범위 내 적정 확장성)

- **Fit-to-Scope Design**: If a new structure, interface, or class must be introduced, its extensibility must strictly match the **scope of the provided design/specification**.
- **YAGNI Enforcement**: Reject speculative generalizations designed for hypothetical "future" features that are not part of the active design doc.
- **Modular Boundaries**: Ensure clean separation of concerns so that future planned variants (e.g., adding a new weapon type or ability) require only adding data or extending a leaf subclass, without modifying core engine loops or coupling unrelated systems.

---

## 4. Network & Replication Optimization (네트워크 및 복제 최적화)

- **Authority Discipline**:
  - All state mutation (health, inventory, combat resolution) must be strictly guarded by `HasAuthority()` or `GetLocalRole() == ROLE_Authority`.
- **RPC Ownership Validation**:
  - `Server_*` RPCs can **only** be declared on actors owned by the calling client's Connection (`APlayerController`, `APlayerState`, possessed `APawn`, or their components). Never place Server RPCs on unowned world actors.
- **Bandwidth Economy**:
  - Every replicated property must use explicit `DOREPLIFETIME_CONDITION` (`COND_OwnerOnly`, `COND_SimulatedOnly`, `COND_InitialOnly`) to eliminate redundant replication traffic.
  - Reject per-tick or high-frequency RPCs. Favor `ReplicatedUsing = OnRep_*` for state synchronization.
- **Fast Array Serialization**:
  - Dynamic replicated lists or inventories must use `FFastArraySerializer` instead of raw `TArray<T>` to replicate only delta elements.

### 4-B. Network Security & Anti-Cheat Invariants (범용 네트워크 보안 불변량)
Every network/gameplay change must additionally be audited against these 6 universal security invariants:
1. **Zero Client Trust (클라이언트 페이로드 불신)**:
   - Never trust client-supplied `AActor*`, `TargetLocation`, `Destination` enums, `ItemId`, `Count`, or damage/cost values without independent server-side verification (reach/distance, line of sight, `IsAttackableTarget` / faction rules, ownership, and data-asset constraints).
2. **Lifecycle & Cost Gating (상태·생존·비용 게이트)**:
   - Every `Server_*` RPC must verify on the server that the caller possesses a valid, alive pawn (`!IsDead()`), is in a valid game state/phase, and satisfies cooldowns and resource costs.
3. **Information Disclosure Control (정보 노출 통제)**:
   - Confidential or hidden game state (private hand/deck/mana/cooldowns, undiscovered rooms, secret doors/traps, server-only RNG) must never replicate with `COND_None` to unauthorized clients; enforce `COND_OwnerOnly` or discovery-gated replication.
4. **Input Sanitization & Anti-DoS**:
   - All floating-point/vector RPC payloads must check `!ContainsNaN()` / `FMath::IsFinite()`; enum parameters must be bounds-validated; dynamic `TArray`/`FString` payloads must enforce upper size limits; `Reliable` Server RPCs must be rate-limited against packet flooding; `NetMulticast` cosmetics must be `Unreliable`.
5. **Identity & Persistence Integrity**:
   - Player persistence and matchmaking keys must prefer verified server identities (`FUniqueNetIdRepl`) over mutable client-supplied names (`PlayerName` / URL options) and enforce server authority (`GetAuthGameMode()` / `HasAuthority()`).
6. **GAS Security**:
   - Player abilities triggered via input/card must use server-enforced execution policies (`ServerInitiated` / `ServerOnly`), validate `TargetData`/prediction keys, and sanitize `NaN` in `UAttributeSet::PreAttributeChange`.

---

## 5. General Performance Optimization (일반 성능 및 리소스 최적화)

- **Tick Hygiene**:
  - Set `PrimaryActorTick.bCanEverTick = false` by default on all Actors and ActorComponents unless per-frame continuous simulation is strictly required. Use timers or event delegates instead.
- **Hot-Path Allocations**:
  - Prohibit dynamic heap allocations (`TArray::Add` without `Reserve`, string formatting, lambda copy captures) inside `Tick()`, animation notifies, or tight physics loops.
- **Memory & GC Safety**:
  - All UObject member properties must use `TObjectPtr<T>`.
  - Non-owning cached references must use `TWeakObjectPtr<T>` to prevent memory leaks and circular reference cycles.
  - Pass non-primitive parameters by `const &` to avoid unnecessary copy constructors.
- **IWYU (Include-What-You-Use)**:
  - Keep header includes strictly minimal; use forward declarations (`class UAbilitySystemComponent;`) in headers and `#include` in `.cpp`.
