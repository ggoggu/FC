# AGENTS.md - FC Architecture & Anti-Slop Harness (UE 5.8)

## 1. Core Tenet: Single-Player is Local Multiplayer
Single-player runs as a local listen server. Every system must operate cleanly across a network boundary without single-player shortcuts.

| Tier | Classes | Hard Boundaries |
| :--- | :--- | :--- |
| **1. Server Authority** | `AGameModeBase`, `AAIController`, Subsystems | Combat, inventory, spawning, and state mutations guarded by `HasAuthority()`. Cooked Engine Assets (`UDataTable`, `UPrimaryDataAsset`) only—no runtime disk `.csv` I/O. |
| **2. Replicated State** | `AGameStateBase`, `APlayerState`, `UActorComponent` | Explicit `DOREPLIFETIME_CONDITION` (`COND_OwnerOnly`, `COND_SimulatedOnly`, `COND_InitialOnly`) + `ReplicatedUsing = OnRep_*`. Dynamic arrays must use `FFastArraySerializer`. |
| **3. Autonomous Client** | `APlayerController`, `ACharacter`, Enhanced Input | Dispatches intent via `Server_*` RPCs (`WithValidation`). Server RPCs are ONLY valid on connection-owned actors (`PlayerController`, `PlayerState`, possessed `Pawn`, or their components—never `AAIController` or world actors). |
| **4. Presentation & UI** | `AHUD`, `UUserWidget`, `UMVVMViewModelBase`, Niagara | Client-only. Server/GAS code must **never** include Slate, UMG, or ViewModel headers. UI reacts strictly via `FieldNotify` and `OnRep_*` delegates. |

---

## 2. Deterministic Verification Loop & Active Hooks
There are no optional execution modes. Mechanical gates enforce every code or asset change:

1. **Ground Truth Asset Inspection (Before Coding)**:
   - Never guess `.uasset` internals (widgets, slots, notifies, variables).
   - Use live **`wildcard-mcp`** (`list_toolsets` / `describe_toolset` / `call_tool` -> `fc_toolset.FCTools`, `UMGToolSet`, `MVVMToolset`, `BlueprintTools`) when Unreal Editor is running, or `python Scripts/pipeline.py inspect --asset <path>` offline.
2. **Mandatory Incremental Static Gate (After Every C++ Edit)**:
   - Immediately run `python Scripts/pipeline.py static --file <modified_file_path>` (`~0.1s`).
   - Enforced mechanically by `.agents/hooks.json` (`Stop` pre-completion gate + `PreToolUse` Anti-Reward-Hacking guard) and `.git/hooks/pre-commit`.
3. **Compilation, Tests, Audit & Harness Meta-Tests**:
   - **Full Static Check**: `python Scripts/pipeline.py static`
   - **UBT Build (Max 5 cycles, 2x identical / 3x consecutive Circuit Breaker)**: `python Scripts/pipeline.py build`
   - **Headless Tests / Net Degradation**: `python Scripts/pipeline.py test` | `python Scripts/pipeline.py net`
   - **Proof-Carrying Adversarial Audit (`/interrogate`)**: `python Scripts/pipeline.py audit`
   - **Harness Self-Test**: `python Scripts/tests/test_linters.py`

---

## 3. Anti-Slop Hard Constraints (Mechanically Enforced)
1. **Minimal Diff & Architecture Reuse**: Apply the smallest clean functional diff. Reuse existing subsystems, components, and DataAssets from the map below before introducing new classes.
2. **Banned Footgun APIs (Hard `ERROR` in Linter)**:
   - Never call `UGameplayStatics::GetPlayerController/GetPlayerPawn/GetPlayerCharacter(World, 0)`.
   - Never bind raw `this` lambdas (`.AddLambda([this])` / `.BindLambda([this])`); use `AddWeakLambda(this, ...)` / `BindWeakLambda(this, ...)`.
   - Never add `ConstructorHelpers::FObjectFinder` or `FClassFinder` in C++ constructors (legacy items frozen in `Scripts/linter_baseline.json`; 0 new allowed).
3. **Zero Escape-Hatch Comments (Hard `ERROR` in Linter)**: Do not leave `// TODO`, `// FIXME`, `// HACK`, `// XXX`, `// Workaround`, or `// For now` comments.
4. **Rule-to-Linter Promotion**: Promote recurring defects into `Scripts/verify_replication.py`, `Scripts/verify_gas.py`, or `Scripts/audit_bandwidth.py` alongside a regression case in `Scripts/tests/test_linters.py`.

---

## 4. Repository & SSOT Architecture Map (`Source/FC/Public/`)
| Domain | Key Classes / SSOT Entrypoints |
| :--- | :--- |
| **GAS & Combat** | `AbilitySystem/FCAttributeSet.h`, `Abilities/FCGA_*`, `Effects/FCGE_*`, `Combat/FCCombatUtils.h`, `Combat/Element/FCElementComponent.h` (`FFCElementStackContainer`), `Combat/Projectile/FCProjectileBase.h`, `Combat/Wall/FCSandWall.h` |
| **Cards & Classes** | `Card/FCCardDeckComponent.h`, `Data/Card/FCCardSubsystem.h` (`UFCCardDataAsset`, `FFCCardHandContainer`), `Data/Class/FCClassSubsystem.h` (`UFCClassDataAsset`), `Data/FCPlayerPersistenceSubsystem.h` |
| **Characters & AI** | `Character/FCCharacterBase.h`, `Character/Player/FCPlayerCharacter.h`, `Character/Mob/FCMobCharacter.h`, `Controller/Player/FCPlayerController.h`, `Controller/AI/FCMobAIController.h`, `AI/Tasks/FCBTTask_*` |
| **Game, Map & World** | `Game/FCGameMode.h`, `Game/FCGameState.h`, `Game/FCPlayerState.h`, `Game/FCGameInstance.h`, `Map/FCDungeonManager.h`, `FCRoomBase.h`, `FCDoorBase.h`, `Gameplay/Spawner/FCMobSpawnSubsystem.h`, `FCChestActor.h`, `FCCardPickupActor.h` |
| **UI (MVVM) & Camera** | `UI/Presenter/FCStatusPresenterComponent.h`, `UI/ViewModel/FCHUDViewModel.h` (`FCHandViewModel`, `FCStatusToggleContainerViewModel`), `UI/View/FCHUDWidget.h`, `UI/ClassSelect/*`, `Camera/FCCameraComponent.h` |
| **Tests & Maps** | `Source/FC/Private/Tests/FC*Test.cpp` (23 Automation Specs under `FC.*`), `Content/Level/Lv_BattleTest.umap`, `Content/Level/ClassSelect/Lv_ClassSelect.umap` |

---

## 5. Skills (`.agents/skills/`) & On-Demand Domain References (`.agents/rules/`)
- **Auto-Discovered Skills**: `interrogate` (Proof-Carrying Adversarial Audit), `ue-asset-inspector` (`wildcard-mcp` & offline `.uasset` schema inspection).
- **On-Demand Rules** *(read only when editing that domain)*:
  - `01-ue58-coding-standards.md` : C++20, `TObjectPtr`, `UPROPERTY` order, IWYU, Footgun & Comment bans
  - `02-multiplayer-conventions.md` : Replication hygiene, RPC validation, `ServerTravel` & Authority guards
  - `03-loop-engineering.md` : Circuit breaker, hooks, asset inspection, `Workspace="share"` & baseline policy
  - `04-gameplay-ability-system.md` : GAS `UAttributeSet`, Meta-Attributes (`NotReplicated`), NaN clamping & Cues
  - `05-combat-and-inventory-networking.md` : `FFastArraySerializer` & authoritative server hit verification
  - `06-umg-mvvm-standards.md` : `UMVVMViewModelBase`, `FieldNotify`, `TSoftObjectPtr<UTexture2D>`, unidirectional UI
  - `07-adversarial-audit-standards.md` : 5 core audit dimensions & 6 universal network security invariants
  - `08-sky-atmosphere-and-level-lighting.md` : UE 5.8 SkyAtmosphere, negative sun pitch & Lumen ground requirements
