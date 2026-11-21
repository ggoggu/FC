# 05. Combat & Inventory Networking Standards

Reference SSOT implementations in this project:
- Fast Array containers: `Source/FC/Public/Combat/Element/FCElementStackContainer.h`, `Source/FC/Public/Data/Card/FCCardHandContainer.h`
- Combat validation utilities: `Source/FC/Public/Combat/FCCombatUtils.h`

## 1. Fast Array Serialization (`FFastArraySerializer`)
Dynamic replicated collections (inventories, card hands, active element stacks, status lists) **must** use `FFastArraySerializer` rather than raw `UPROPERTY(Replicated) TArray<T>` (`BW_RAW_TARRAY_REPLICATED` error in `audit_bandwidth.py`).

Required contract:
1. **Item struct (`: public FFastArraySerializerItem`)**: Holds element payload and implements `PostReplicatedAdd`, `PostReplicatedChange`, `PreReplicatedRemove`.
2. **Container struct (`: public FFastArraySerializer`)**:
   - Holds `UPROPERTY() TArray<FItemType> Items;` and `UPROPERTY(NotReplicated) TObjectPtr<UActorComponent> OwnerComponent;`.
   - Implements `bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)` calling `FFastArraySerializer::FastArrayDeltaSerialize<FItemType, FContainerType>(Items, DeltaParms, *this)`.
   - Specializes `TStructOpsTypeTraits<FContainerType>` with `WithNetDeltaSerializer = true`.
   - Calls `MarkItemDirty(Item)` / `MarkArrayDirty()` on every server mutation.
3. **Bandwidth Filtering**: Register private containers (hands, inventories) with `DOREPLIFETIME_CONDITION(Class, Container, COND_OwnerOnly)` (`COND_None` only for public world/combat indicators such as visible enemy element stacks).

## 2. Authoritative Combat & Hit Verification
1. **Zero Client Payload Trust**: Clients transmit only intent (`AttackTag`, `CardIndex`, `TargetActor`, aim direction). Never accept client-computed damage, cost, or unchecked hit coordinates.
2. **`_Validate` Sanitization**: Verify `!ContainsNaN()` / `FMath::IsFinite()` on vectors/floats, enum bounds, container length caps, and coarse reach thresholds (with bounded latency tolerance) to kick tampered packets.
3. **`_Implementation` Server Verification**:
   - Guard with `HasAuthority()`.
   - Verify caller pawn is valid and `!IsDead()`, in a valid combat phase, and meets cooldown/mana costs.
   - Verify faction/targetability via `UFCCombatUtils::IsAttackableTarget(...)`.
   - Perform authoritative line traces / sweeps (`FCollisionQueryParams`) and FOV/obstruction checks server-side; when implementing lag rewind, bound rewind buffers strictly (e.g., max `200ms`).
