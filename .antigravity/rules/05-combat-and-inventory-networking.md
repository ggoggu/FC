# Combat and Inventory Networking Standards

High-performance multiplayer games demand strict bandwidth optimization for dynamic inventories and uncompromised server authority for combat hit detection.

---

## 1. Fast Array Serialization Pattern

Dynamic item lists, weapon inventories, active status buffs, and equipment slots **MUST** implement Unreal Engine's `FFastArraySerializer` architecture rather than replicating standard `TArray<...>`.

### 1. Item Struct Definition (`FFastArraySerializerItem`)
Each element in the container inherits from `FFastArraySerializerItem` and defines replication callbacks:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "FCInventoryTypes.generated.h"

struct FFCInventoryContainer;

USTRUCT(BlueprintType)
struct FC_API FFCInventoryItem : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "FC|Inventory")
    FGuid ItemGuid;

    UPROPERTY(BlueprintReadOnly, Category = "FC|Inventory")
    int32 ItemID = 0;

    UPROPERTY(BlueprintReadOnly, Category = "FC|Inventory")
    int32 Quantity = 1;

    // Fast Array replication callbacks
    void PostReplicatedAdd(const FFCInventoryContainer& InArraySerializer);
    void PostReplicatedChange(const FFCInventoryContainer& InArraySerializer);
    void PreReplicatedRemove(const FFCInventoryContainer& InArraySerializer);
};
```

### 2. Container Struct Definition (`FFastArraySerializer`)
The container struct holds the array of items, tracks the owning component, and implements `NetDeltaSerialize`:

```cpp
USTRUCT(BlueprintType)
struct FC_API FFCInventoryContainer : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FFCInventoryItem> Items;

    UPROPERTY(NotReplicated)
    TObjectPtr<UActorComponent> OwnerComponent = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FFCInventoryItem, FFCInventoryContainer>(Items, DeltaParms, *this);
    }

    // Helper mutation API (Server Only)
    void AddItem(const FFCInventoryItem& NewItem);
    void RemoveItem(const FGuid& ItemGuid);
};

template<>
struct TStructOpsTypeTraits<FFCInventoryContainer> : public TStructOpsTypeTraitsBase2<FFCInventoryContainer>
{
    enum
    {
        WithNetDeltaSerializer = true,
    };
};
```

### 3. Component Integration & Bandwidth Filtering
Always register the container property with `COND_OwnerOnly` in `GetLifetimeReplicatedProps`:

```cpp
// In Header:
UPROPERTY(Replicated, Category = "FC|Inventory")
FFCInventoryContainer InventoryList;

// In Source:
void UFCInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UFCInventoryComponent, InventoryList, COND_OwnerOnly);
}
```

---

## 2. Authoritative Combat & Hit Verification

### 1. Server-Side Trace Validation
- Clients predict attack animations and may send a hit intent request via Server RPC (`Server_SubmitHitTarget(const FFCHitVerificationData& HitData)`).
- The Server **never trusts client hit coordinates or damage amounts**.
- The Server performs authoritative line traces / sphere sweeps (`FCollisionQueryParams`) against the target's current and historically buffered position.

```cpp
bool AFCCombatCharacter::Server_SubmitHitTarget_Validate(const FFCHitVerificationData& HitData)
{
    // 1. Validate target actor pointer
    if (!HitData.TargetActor.IsValid())
    {
        return false;
    }

    // 2. Validate reach distance to prevent teleport/range exploits
    const float MaxWeaponReachSq = FMath::Square(HitData.MaxAllowedDistance + 50.0f); // 50cm latency tolerance
    const float DistanceSq = FVector::DistSquared(GetActorLocation(), HitData.TargetActor->GetActorLocation());
    if (DistanceSq > MaxWeaponReachSq)
    {
        return false;
    }

    return true;
}

void AFCCombatCharacter::Server_SubmitHitTarget_Implementation(const FFCHitVerificationData& HitData)
{
    if (!HasAuthority())
    {
        return;
    }

    // Authoritative sweep validation
    FHitResult ServerHitResult;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CombatHitValidation), false, this);
    const FVector Start = GetActorLocation();
    const FVector End = HitData.TargetActor->GetActorLocation();

    if (GetWorld()->LineTraceSingleByChannel(ServerHitResult, Start, End, ECC_Pawn, QueryParams))
    {
        if (ServerHitResult.GetActor() == HitData.TargetActor.Get())
        {
            ApplyAuthoritativeCombatDamage(HitData.TargetActor.Get(), HitData.AttackTag);
        }
    }
}
```

### 2. Anti-Cheat & Position Verification Tenets
1. **Never Replicate Damage Payloads from Client**: Clients transmit only intent tags (`AttackTag`) and targeted entities (`TargetActor`). All damage formulas and attribute scaling are computed server-side in `UAttributeSet` or Gameplay Effects.
2. **Raycast / Sweep Cone Verification**: Validate that the target is within the attacker's forward field of view (FOV cone) and unobstructed by world geometry.
3. **Rewind & Latency Compensation**: When evaluating hits with latency, compare against timestamped actor transform buffers with bounded max rewind time (e.g., maximum 200ms ping window).
