# AGENTS.md - FC Architecture & Anti-Slop Constraints (UE 5.8)

## 1. Core Tenet: Single-Player is Local Multiplayer
Single-player runs as a local listen server. Every system must operate cleanly across a network boundary without single-player shortcuts.

| Tier | Classes | Hard Boundaries |
| :--- | :--- | :--- |
| **1. Server Authority** | `AGameModeBase`, `AAIController`, Subsystems | Combat, inventory, spawning, and state mutations guarded by `HasAuthority()`. Cooked Engine Assets (`UDataTable`, `UPrimaryDataAsset`) only—no runtime disk `.csv` I/O. |
| **2. Replicated State** | `AGameStateBase`, `APlayerState`, `UActorComponent` | Explicit `DOREPLIFETIME_CONDITION` (`COND_OwnerOnly`, `COND_SimulatedOnly`, `COND_InitialOnly`) + `ReplicatedUsing = OnRep_*`. Dynamic arrays must use `FFastArraySerializer`. |
| **3. Autonomous Client** | `APlayerController`, `ACharacter`, Enhanced Input | Dispatches intent via `Server_*` RPCs (`WithValidation`). Server RPCs are ONLY valid on connection-owned actors (`PlayerController`, `PlayerState`, possessed `Pawn`, or their components). |
| **4. Presentation & UI** | `AHUD`, `UUserWidget`, `UMVVMViewModelBase`, Niagara | Client-only. Server/GAS code must **never** include Slate, UMG, or ViewModel headers. UI reacts strictly via `FieldNotify` and `OnRep_*` delegates. |

---

## 2. Deterministic Verification Loop
There are no optional execution modes. Every code or asset change follows the same deterministic verification loop:

1. **Ground Truth Asset Inspection (Before Coding)**:
   - Never guess `.uasset` internals (widgets, slots, notifies, variables).
   - Use live `unreal-mcp` (`FCTools.inspect_asset_summary`, `UMGToolSet`, `MVVMToolset`, `BlueprintTools`) when Unreal Editor is running, or `python Scripts/pipeline.py inspect --asset <path>` offline.
2. **Mandatory Incremental Static Gate (After Every C++ Edit)**:
   - Whenever any `.h` or `.cpp` file is created or modified, run:
     ```powershell
     python Scripts/pipeline.py static --file <modified_file_path>
     ```
3. **Compilation, Release & Audit**:
   - **Full Static Check**: `python Scripts/pipeline.py static`
   - **UBT Build (Max 5 self-heal cycles, 2x repeat Circuit Breaker)**: `python Scripts/pipeline.py build`
   - **Adversarial Audit (`/interrogate`)**: `python Scripts/pipeline.py audit`
   - **Full Pipeline (Static -> Build -> Test)**: `python Scripts/pipeline.py full`

---

## 3. Anti-Slop Hard Constraints
1. **Minimal Diff & Architecture Reuse**: Apply the smallest clean functional diff. Reuse existing subsystems, components, and DataTables before introducing new classes.
2. **Banned Footgun APIs (Enforced by Linter)**:
   - Never call `UGameplayStatics::GetPlayerController/GetPlayerPawn/GetPlayerCharacter(World, 0)`.
   - Never bind raw `this` lambdas (`.AddLambda([this])`); use `AddWeakLambda(this, ...)`.
   - Never use `ConstructorHelpers::FObjectFinder` or hardcoded `/Game/...` paths in C++ constructors.
3. **Zero Escape-Hatch Comments**: Do not write `// TODO`, `// FIXME`, `// HACK`, or narrative comments rationalizing incomplete validation or architectural shortcuts.
4. **Rule-to-Linter Promotion**: Promote recurring defects directly into deterministic checks in `Scripts/verify_replication.py`, `Scripts/verify_gas.py`, or `Scripts/audit_bandwidth.py`.

---

## 4. Domain Rules Index (`.antigravity/rules/`)
- `01-ue58-coding-standards.md` : C++20, `TObjectPtr`, IWYU, Footgun API & Comment bans
- `02-multiplayer-conventions.md` : Replication hygiene, RPC validation & network security
- `03-loop-engineering.md` : Self-healing circuit breaker, asset inspection & worktree isolation
- `04-gameplay-ability-system.md` : GAS `UAttributeSet`, `PreAttributeChange` NaN clamping & Gameplay Cues
- `05-combat-and-inventory-networking.md` : `FFastArraySerializer` & authoritative server trace validation
- `06-umg-mvvm-standards.md` : `UMVVMViewModelBase`, `FieldNotify`, `TSoftObjectPtr<UTexture2D>`
- `07-adversarial-audit-standards.md` : 5 core audit dimensions & 6 network security invariants (`/interrogate`)
- `08-sky-atmosphere-and-level-lighting.md` : UE 5.8 SkyAtmosphere, negative sun pitch & Lumen ground requirements
