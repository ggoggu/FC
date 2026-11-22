# 07. Adversarial Code Audit & Optimization Standards

Official review rubric for the read-only `adversarial-auditor` subagent (`/interrogate`).

## 0. Proof-Carrying Audit Prerequisite
Verify that the diff includes concrete execution proofs (`pipeline.py audit` / `static`, and `build` / `test` results for non-trivial gameplay or networking changes). Flag missing build/test proof on risky changes as `WARNING` or `BLOCKING`.

## 1. Code Conciseness (최소 수정 원칙)
- Minimal clean functional diff strictly scoped to the task.
- Reject premature wrappers, single-use indirections, redundant pass-throughs, and obscure template gymnastics.
- Flag deeply nested control flow (Arrow Anti-Pattern, e.g., 5+ levels of cascading `if` / `nullptr` checks) when Guard Clauses (early returns), local IIFE lambdas, or shared extraction helpers would cleanly flatten it.

## 2. Reuse of Existing Architecture & SSOT (기존 구조 활용)
- Verify existing `Source/FC/` subsystems, components, utilities, and DataAssets (`AGENTS.md` §4) are reused rather than duplicated.
- Reject parallel state stores, duplicate ID lists, or loose disk `.csv` reads (`FPaths::ProjectContentDir() / *.csv`).

## 3. Scoped Extensibility (범위 내 확장성 / YAGNI)
- Design extensibility strictly to the stated requirements—reject speculative abstractions for hypothetical future features.
- Keep modular leaf boundaries so new variants (cards, abilities, enemies) extend via data or leaf subclasses without editing core loops.

## 4. Network & Replication Optimization + 6 Universal Security Invariants
- **Optimization**: `HasAuthority()` guards, connection-owned `Server_*` RPCs only, explicit `DOREPLIFETIME_CONDITION`, `OnRep_*` over tick RPCs, `FFastArraySerializer` for dynamic arrays.
- **6 Universal Security Invariants**:
  1. **Zero Client Trust**: Independently verify client-supplied `AActor*`, locations, destinations, `ItemId`, counts, and costs server-side (distance/reach, LOS, `UFCCombatUtils::IsAttackableTarget`, ownership, DataAsset constraints).
  2. **Lifecycle & Cost Gating**: Every `Server_*` RPC checks valid possessed alive pawn (`!IsDead()`), valid game phase, cooldowns, and resource costs on the server.
  3. **Information Disclosure Control**: Never replicate private state (hand/deck/mana/cooldowns, undiscovered rooms, secret doors/traps, server RNG) with `COND_None`; enforce `COND_OwnerOnly` or discovery-gated replication.
  4. **Input Sanitization & Anti-DoS**: Sanitize float/vector RPC payloads (`!ContainsNaN()` / `FMath::IsFinite()`), validate enum bounds, cap `TArray`/`FString` lengths, rate-limit `Reliable` Server RPCs, and keep `NetMulticast` cosmetics `Unreliable`.
  5. **Identity & Persistence Integrity**: Key persistence/matchmaking on verified server identity (`FUniqueNetIdRepl`) over mutable client `PlayerName`/URL options and enforce server authority.
  6. **GAS Security**: Enforce server-controlled ability policies (`ServerInitiated` / `ServerOnly`), validate `TargetData`/prediction keys, and sanitize `NaN` in `UAttributeSet::PreAttributeChange`.

## 5. General Performance & Memory Hygiene
- **Tick Hygiene**: `bCanEverTick = false` by default; use timers or event delegates unless per-frame simulation is required.
- **Hot-Path Allocations**: No dynamic heap allocations (`TArray::Add` without `Reserve`, string formatting, copy-capturing lambdas) in `Tick()`, notifies, or tight physics loops.
- **Memory & IWYU**: `TObjectPtr<T>` for UObject `UPROPERTY`s, `TWeakObjectPtr<T>` for non-owning caches, `AddWeakLambda(this, ...)` for UObject delegates, `const &` for non-primitive params, forward declarations in `.h`.
