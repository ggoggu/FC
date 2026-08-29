# AGENTS.md - Lead Systems & Multiplayer Architect Directives

Welcome to the **FC** project repository. This document defines the architectural vision, authority boundaries, coding guidelines, and the autonomous loop engineering protocols governing all development in Unreal Engine 5.8.

---

## 1. Project Vision & "Multiplayer-Ready" Architecture

The project is fundamentally designed as a high-fidelity **Single-Player experience built on a dedicated/listen server replication architecture**. 

### Architectural Tenet: Single-Player is Local Multiplayer
- All gameplay systems are built from day one to operate cleanly across a network boundary.
- Single-player mode runs as a local listen server (standalone client + server session within the engine), ensuring **100% feature parity** between single-player and co-op/multiplayer modes.
- **Never** write single-player-only shortcuts (e.g., mutating inventory or health directly on client instances without server authority).

```
+-----------------------------------------------------------------------------------+
|                              AUTHORITATIVE SERVER                                 |
|  - AGameModeBase / AGameMode                                                      |
|  - AI Controllers / Behavior Trees / Mass AI                                      |
|  - Authoritative Combat, Health, Damage, Stats & Inventory Mutation               |
|  - Server-side Validation & Anti-Cheat Logic                                      |
+----------------------------------------+------------------------------------------+
                                         |
                       Replicated State  |  Server RPCs
                    (DOREPLIFETIME /     |  (WithValidation)
                     OnRep Callbacks)    |
                                         v
+-----------------------------------------------------------------------------------+
|                              AUTONOMOUS CLIENT                                    |
|  - Local PlayerController & Enhanced Input Subsystem                              |
|  - Client-Side Predictive Movement (CharacterMovementComponent)                   |
|  - UI Intent Handling & Predictive Action Requests                                |
+----------------------------------------+------------------------------------------+
                                         |
                        Delegates &      |  Decoupled State Observers
                        RepNotifies      |  (Zero Server Coupling)
                                         v
+-----------------------------------------------------------------------------------+
|                            SIMULATED CLIENT & UI                                  |
|  - UMG Widgets / HUD / Viewmodels (Client-Only)                                   |
|  - Audio (SFX / Music) & Visual Effects (Niagara / Skeletal Meshes)               |
|  - Cosmetic Interpolation & Non-Authoritative Animations                          |
+-----------------------------------------------------------------------------------+
```

---

## 2. Authority & Separation of Concerns

All code and assets must adhere strictly to the following four-tier authority separation:

### 1. Server Authority (Server-Only Execution)
- **Classes**: `AGameModeBase`, `AAIController`, authoritative Managers/Subsystems.
- **Responsibilities**:
  - Combat resolution, damage calculations, death logic.
  - Inventory, quest, and economy mutation.
  - Spawning gameplay actors and loot drops.
  - Validation of incoming `Server_*` RPC payloads.
- **Guarantees**: Wrapped in `if (HasAuthority())` or `if (GetLocalRole() == ROLE_Authority)`.

### 2. Replicated State (Server-to-Client Propagation)
- **Classes**: `AGameStateBase`, `APlayerState`, `UActorComponent` derivatives.
- **Responsibilities**:
  - Synchronizing gameplay state (health, mana, active buffs, team scores).
  - Explicit bandwidth optimization using `DOREPLIFETIME_CONDITION` (e.g., `COND_OwnerOnly` for inventory/private stats, `COND_SimulatedOnly` for simulated proxies).
  - Synchronizing state changes through `ReplicatedUsing = OnRep_PropertyName` notifications.

### 3. Autonomous Client (Input & Prediction)
- **Classes**: `APlayerController`, `ACharacter` (autonomous proxy), Input Modifiers.
- **Responsibilities**:
  - Processing player input via Enhanced Input.
  - Local movement prediction and instant feedback.
  - Dispatches requests to the server via validated `Server_*` RPCs (`WithValidation`).

### 4. Simulated Client & UI (Presentation Tier)
- **Classes**: `AHUD`, `UUserWidget`, `UNiagaraComponent`, Audio Components.
- **Responsibilities**:
  - Rendering UI, health bars, inventory screens, minimaps.
  - Playing cosmetic sound effects and particle systems.
- **Strict Rule**: UI and presentation classes are client-only. Server-side code must **never** include or invoke Slate/UMG widgets. UI listens to replicated state via OnRep delegates or PlayerState/Controller callbacks.

---

## 3. Combat, Ability & Networking Architecture

### 1. Combat & Ability Framework (Gameplay Ability System)
- **Mandatory GAS Adoption**: All abilities, melee/ranged combat actions, status effects (Buffs/Debuffs), and stamina/resource systems must be authored using the Unreal Gameplay Ability System (`UGameplayAbility`, `UAbilitySystemComponent`, `UAttributeSet`, `UGameplayEffect`).
- **Gameplay Attributes & RepNotify**:
  - All gameplay attributes must inherit from `UAttributeSet` and declare accessors using the standard `ATTRIBUTE_ACCESSORS(ClassName, PropertyName)` macro.
  - Attribute replication must utilize `GAMEPLAYATTRIBUTE_REPNOTIFY(ClassName, PropertyName, OldValue)` inside `OnRep_*` callbacks and `DOREPLIFETIME_CONDITION_NOTIFY(ClassName, PropertyName, COND_None, REPNOTIFY_Always)` in `GetLifetimeReplicatedProps`.
  - Enforce attribute clamp logic inside `PreAttributeChange` and authoritative attribute modification inside `PostGameplayEffectExecute`.
- **Client Prediction vs. Authoritative Resolution**:
  - Attack startup animations, montages, and client movement feedback must use client-side prediction (`FPredictionKey`).
  - Hit detection, line traces, sphere sweeps, damage application, and gameplay tag grants/removals are strictly authoritative on the server.
  - Cosmetic cues (VFX, SFX, screen shake) must be dispatched via Gameplay Cues (`UGameplayCueNotify_*`) rather than Multicast RPCs.

### 2. Bandwidth Optimization & Inventory Networking
- **Fast Array Serialization**:
  - Dynamic item lists, weapon loadouts, and active inventory slots **MUST** implement `FFastArraySerializer` / `FFastArraySerializerItem` instead of standard replicated `TArray<...>`.
  - Item structs must implement `PostReplicatedAdd`, `PostReplicatedChange`, and `PreReplicatedRemove` to invoke minimal UI/gameplay state updates without full-array network resends.
  - The container struct must implement `NetDeltaSerialize` and register via `FNetDeltaSerializeInfo`.
- **Bandwidth-Conscious Replication Conditions**:
  - Replicated properties must use optimal conditional filters:
    - `COND_OwnerOnly`: Inventory items, private stats (stamina, ammo, quest progress), ability cooldown details.
    - `COND_SkipOwner`: Visuals, weapon holsters, and montage state already predicted locally on the autonomous client.
    - `COND_SimulatedOnly`: Interpolation helper state required solely by simulated proxies.

---

## 4. Modular Pipeline & Verification Protocols

To maintain high development velocity and optimize AI token consumption, the project utilizes a **Tiered On-Demand Pipeline Runner** (`Scripts/pipeline.py`).

### Autonomous Execution Modes:

```
┌──────────────────────────────────────────────────────────────────────────────────────┐
│                              PIPELINE EXECUTION MODES                                │
├──────────────────────────────────────────────────────────────────────────────────────┤
│ 1. [Default] Fast Mode        : Direct C++ coding & Q&A (Zero script overhead)       │
│ 2. Static Audit (`static`)    : GAS, Bandwidth, Replication checks (~1s, low tokens) │
│ 3. Build Harness (`build`)    : UBT compile & JSON error auto-correction loop        │
│ 4. Combat Balance (`balance`) : Mathematical TTK & effective DPS curves (~1s)        │
│ 5. Automation Test (`test`)   : Headless Unreal automation tests                     │
│ 6. Full Release (`full`)      : Sequential (Static -> Build -> Test -> [Auto-Commit])│
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### Protocol Guidelines:

1. **Default Mode (Fast Track)**:
   - For regular code edits, refactorings, or explanations, do **not** run verification scripts automatically. Provide clean C++ code and explanations directly.

2. **On-Demand Verification (Single Tool Call)**:
   - When verification is requested by the user, execute the unified runner in a **single command**:
     - **Static Checks**: `python Scripts/pipeline.py static`
     - **Compilation & Patching**: `python Scripts/pipeline.py build` (Parse JSON diagnostics, auto-patch up to 5 cycles)
     - **Combat Balance**: `python Scripts/pipeline.py balance`
     - **Full Validation & Commit**: `python Scripts/pipeline.py full --auto-commit -m "<conventional_commit_msg>"`

3. **Compact Reporting**:
   - The runner provides fail-fast, ultra-compact outputs. If errors occur, inspect the targeted `file:line` details and resolve them autonomously before proceeding.

