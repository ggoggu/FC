# 02. Multiplayer & Network Replication Conventions

All gameplay systems operate under authoritative server-client replication. *(See `07-adversarial-audit-standards.md` §4-B for the 6 Universal Network Security Invariants.)*

## 1. Replication Core Hygiene
- **Header Include**: Any `.cpp` using `DOREPLIFETIME*` or `GetLifetimeReplicatedProps` must include `#include "Net/UnrealNetwork.h"` (`REPL_MISSING_NET_INCLUDE`).
- **Lifetime Registration**: Every class with `Replicated` / `ReplicatedUsing` properties must override `GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const` and call `Super::GetLifetimeReplicatedProps(OutLifetimeProps)` first.
- **Conditional Replication Mandatory** (`BW_RAW_DOREPLIFETIME` error): Never use raw `DOREPLIFETIME(Class, Prop)`. Always use `DOREPLIFETIME_CONDITION` with explicit scope:
  - `COND_OwnerOnly` (private hand, deck, mana, cooldowns, personal inventory)
  - `COND_SkipOwner` (data already locally predicted by autonomous proxy)
  - `COND_SimulatedOnly` (cosmetic proxy interpolation)
  - `COND_InitialOnly` (spawn-time immutable attributes)
  - `COND_None` (public world/combat state visible to all relevant clients)
- **RepNotify Convention**: Use `ReplicatedUsing = OnRep_<PropName>` with `UFUNCTION() void OnRep_<PropName>(OldValue)` when delta transitions matter.
- **Blueprint Authority Protection** (`REPL_BYPASSED_AUTHORITY_SETTER`): Never combine `Replicated` + `BlueprintReadWrite` without `BlueprintSetter = Set<PropName>` when an authority setter exists; prefer `BlueprintReadOnly`.

## 2. RPC Architecture & Ownership
- **Server RPCs (`Server_*`)**:
  - `WithValidation` is **mandatory** (`REPL_SERVER_RPC_MISSING_VALIDATION`).
  - **Never** leave parameterized `Server_*_Validate` as unconditional `return true;` (`NET_SEC_TRIVIAL_RPC_VALIDATE`). Sanitize floats/vectors with `!ContainsNaN()` / `FMath::IsFinite()` (`NET_SEC_UNSANITIZED_FLOAT_OR_VECTOR_RPC`), validate enum bounds, and cap `TArray`/`FString` sizes with `.Num()`/`.Len()` (`NET_SEC_UNBOUNDED_RPC_CONTAINER`).
  - **Connection Ownership Prerequisite** (`REPL_SERVER_RPC_UNOWNED_ACTOR`): Clients may ONLY invoke Server RPCs on `APlayerController`, `APlayerState`, possessed `APawn`/`ACharacter`, or their components—**never** on `AAIController` or unowned field actors (route pickup/chest/door interaction through the client's controller or deck component).
  - Rate-limit high-impact `Reliable` Server RPCs with server timestamps.
- **Client RPCs (`Client_*`)**: Use for targeted notifications to the owning client; never replicate continuous bulk state via Client RPCs.
- **Multicast RPCs (`Multicast_*`)**: Must be `Unreliable` (`NET_SEC_RELIABLE_MULTICAST`) and **strictly cosmetic**—never mutate gameplay state inside a Multicast. Use `OnRep_*` when late-joiners need current state.

## 3. Replication Initialization & Authority Isolation
- **Constructors**: Replicated Actors must set `bReplicates = true;` (`REPL_ACTOR_CTOR_MISSING_BREPLICATES`); replicated `UActorComponent`s must call `SetIsReplicatedByDefault(true);` (`REPL_COMPONENT_CTOR_MISSING_REPLICATION`).
- **C++ Authority Guards**: Guard all state mutations and every non-const `BlueprintAuthorityOnly` method in C++ with `HasAuthority()` (or `GetOwner()->HasAuthority()` / `GetWorld()->GetAuthGameMode() != nullptr` for subsystems) (`NET_SEC_AUTHORITY_ONLY_MISSING_CPP_GUARD`).
- **Zero Server-UI Coupling**: Never include UMG, Slate, or ViewModel headers in gameplay Actor/GAS headers or server `.cpp` files (`REPL_UI_HEADER_IN_SERVER_ACTOR`, `REPL_UI_HEADER_IN_SERVER_CPP`).
- **Level Transitions**: Always use `GetWorld()->ServerTravel(MapURL, false)` on the server; never call `UGameplayStatics::OpenLevel()` in `GameMode` (`REPL_OPENLEVEL_IN_GAMEMODE`).
