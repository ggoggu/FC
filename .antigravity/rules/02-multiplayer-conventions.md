# Multiplayer & Network Replication Conventions

All gameplay systems must be designed for authoritative server-client synchronization, adhering to strict replication hygiene and bandwidth optimization rules.

---

## 1. Replication Core Hygiene

### 1. Header & Source Inclusions
- In any `.cpp` file implementing replication lifecycle functions or registering properties, include:
  ```cpp
  #include "Net/UnrealNetwork.h"
  ```

### 2. Lifetime Replicated Properties Registration
- For every class declaring replicated properties (`Replicated` or `ReplicatedUsing`), you **must**:
  1. Declare the virtual override in the header:
     ```cpp
     virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
     ```
  2. Implement it in the `.cpp`, always calling `Super::GetLifetimeReplicatedProps(OutLifetimeProps)`:
     ```cpp
     void AFCCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
     {
         Super::GetLifetimeReplicatedProps(OutLifetimeProps);

         DOREPLIFETIME_CONDITION(AFCCharacter, Health, COND_None);
         DOREPLIFETIME_CONDITION(AFCCharacter, InventoryData, COND_OwnerOnly);
     }
     ```

### 3. Conditional Replication (`DOREPLIFETIME_CONDITION`)
- **Mandatory**: Do not use raw `DOREPLIFETIME` without evaluating conditional filtering. Prefer explicit conditions to conserve network bandwidth:
  - `COND_OwnerOnly`: Private data like inventory, personal quest state, stamina, ammo.
  - `COND_SkipOwner`: Visual data already locally predicted by the autonomous proxy.
  - `COND_SimulatedOnly`: Data needed solely by simulated proxies (e.g. cosmetic interpolation tags).
  - `COND_InitialOnly`: Static attributes established on spawn that do not mutate during actor lifetime.

### 4. RepNotify Naming & Signatures
- Replicated variables using callbacks must specify `ReplicatedUsing = OnRep_PropertyName`.
- The callback function must be named `OnRep_PropertyName`.
- Include the old value parameter when state delta transitions need processing:
  ```cpp
  UPROPERTY(ReplicatedUsing = OnRep_Health, Category = "FC|Stats")
  float Health = 100.0f;

  UFUNCTION()
  void OnRep_Health(float OldHealth);
  ```

---

## 2. RPC Architecture & Validation

### 1. Server RPCs (`Server_*`)
- **Mandatory `WithValidation`**: Every Server RPC **must** specify `WithValidation`.
  ```cpp
  UFUNCTION(Server, Reliable, WithValidation, Category = "FC|Combat")
  void Server_ExecuteAttack(const FFCAttackParameters& Params);
  ```
- **Implementation & Validation Pair**:
  - `Server_ExecuteAttack_Validate(const FFCAttackParameters& Params)`: Validate parameters against cheating, impossible distances, invalid indices, or cooldown states. Return `false` to kick/flag malicious requests.
  - `Server_ExecuteAttack_Implementation(const FFCAttackParameters& Params)`: Authoritative gameplay execution.

### 2. Client RPCs (`Client_*`)
- Use Client RPCs for targeted notifications directed to the owning controller/client:
  ```cpp
  UFUNCTION(Client, Reliable, Category = "FC|UI")
  void Client_OnQuestCompleted(int32 QuestId, const FFCQuestReward& Reward);
  ```
- Do not replicate bulk continuous state via Client RPCs; use replicated properties for state synchronization.

### 3. Multicast RPCs (`Multicast_*`)
- **Cosmetic Only**: Multicasts are strictly reserved for non-critical, transient cosmetic effects (e.g., sound cues, particle bursts, screen shake).
- **Never mutate gameplay state** (health, inventory, position) inside a Multicast RPC.
- Prefer RepNotifies over Multicast RPCs when late-joining clients need the correct state.

---

## 3. Actor Replication Initialization

### Actor Constructors
In the constructor of any Actor that replicates or sends RPCs:
```cpp
bReplicates = true;
ACharacter::SetReplicates(true); // or SetReplicates(true);
```

### Component Constructors
In the constructor of any `UActorComponent` that replicates:
```cpp
SetIsReplicatedByDefault(true);
```

---

## 4. Authority Isolation & UI Decoupling

### Authority Guarding
All mutation of authoritative state (spawning actors, inflicting damage, granting XP, modifying inventory) must be guarded:
```cpp
if (HasAuthority()) // or GetLocalRole() == ROLE_Authority
{
    ApplyAuthoritativeDamage(TargetActor, DamageAmount);
}
```

### Zero Server-UI Coupling
- **Never** instantiate, reference, or modify UMG Widgets or HUDs on the server.
- Server-side code must not include `#include "Blueprint/UserWidget.h"` or Slate headers.
- UI elements must react passively to replicated property updates (`OnRep_*`) or Client RPC triggers on the autonomous client.
