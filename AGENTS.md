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

## 3. Modular Pipeline & Verification Protocols

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

---

## 4. Modular Domain Rules (.antigravity/rules/)

Feature-specific standards are modularized in `.antigravity/rules/` and applied **on-demand** only when implementing relevant features:

- **General C++20 Coding Standards**: Refer to `.antigravity/rules/01-ue58-coding-standards.md` (TObjectPtr, IWYU, PascalCase).
- **Multiplayer Conventions**: Refer to `.antigravity/rules/02-multiplayer-conventions.md` (RPC validation, Server Authority).
- **Gameplay Ability System (GAS)**: When implementing GAS-based attributes, abilities, or effects, refer to `.antigravity/rules/04-gameplay-ability-system.md` (Attribute accessors, RepNotify, Gameplay Cues). Standard C++ components can be used for features where GAS is not required.
- **Fast Array & Inventory Networking**: When implementing dynamic replicated item lists or inventories, refer to `.antigravity/rules/05-combat-and-inventory-networking.md` (FFastArraySerializer, COND_OwnerOnly).
- **UMG Model-View-ViewModel (MVVM)**: When implementing UI presentation models, viewmodel collections, or field notification bindings, refer to `.antigravity/rules/06-umg-mvvm-standards.md` (UMVVMViewModelBase, FieldNotify, UE_MVVM_SET_PROPERTY_VALUE).


