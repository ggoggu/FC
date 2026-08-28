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

## 3. Loop Engineering Lifecycle

When implementing features, fixing bugs, or refactoring code, follow this standardized autonomous development cycle:

```
[ Step 1: Specs & Architecture ]
               |
               v
[ Step 2: Write C++ Code & Headers ]
               |
               v
[ Step 3: Compile via UBT (Scripts/build_harness.py) ] <----+
               |                                           | (Self-Correction Loop:
          Compilation                                      |  Parse JSON & Patch)
           Succeeded? ---- NO (Max 5 attempts) ------------+
               |
              YES
               v
[ Step 4: Verify Replication (Scripts/verify_replication.py) ] <---+
               |                                                   | (Self-Correction:
          Replication                                              |  Fix Violations)
           Compliant? ---- NO -------------------------------------+
               |
              YES
               v
[ Step 5: (Optional) Headless Specs (Scripts/run_tests.py) ]
               |
               v
[ Step 6: Present Clean Diffs & Architectural Summary ]
```

### Protocol Steps:

1. **Step 1: Specifications & Architecture**:
   - Analyze requirements against single-player/multiplayer authority boundaries.
   - Identify which properties require replication and choose the optimal replication condition (`COND_OwnerOnly`, etc.).

2. **Step 2: C++ Implementation**:
   - Apply Unreal Engine 5.8 coding standards (C++20, `TObjectPtr`, IWYU, PascalCase).
   - Write headers with clean forward declarations and implement `.cpp` logic with strict authority checks.

3. **Step 3: Build Harness Execution (`Scripts/build_harness.py`)**:
   - Execute the build harness to invoke UBT and UHT.
   - If errors occur, parse the JSON diagnostic output (`error_count`, `file`, `line`, `message`) and autonomously patch the code.
   - Iterate up to **5 cycles**. If still failing, present full diagnostic logs and articulate the blocker.

4. **Step 4: Replication Static Verification (`Scripts/verify_replication.py`)**:
   - Run the replication analyzer to verify:
     - `GetLifetimeReplicatedProps` implementation and `#include "Net/UnrealNetwork.h"`.
     - Mandatory `WithValidation` on all `Server_*` RPCs.
     - `bReplicates = true` in constructor of replicated actors.
     - Zero UI/UMG header leaks in server gameplay actor headers.
   - Autonomously resolve any reported violations.

5. **Step 5: Automated Testing (`Scripts/run_tests.py`)**:
   - Execute headless automation tests to verify runtime behavior and regression prevention.

6. **Step 6: Summary & Handoff**:
   - Provide a concise summary of the architectural changes and verified diffs.
