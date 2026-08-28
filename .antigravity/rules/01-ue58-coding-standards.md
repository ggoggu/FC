# Unreal Engine 5.8 C++ Coding Standards & Best Practices

All C++ codebase changes within this project must strictly comply with Unreal Engine 5.8 standards and modern C++20 conventions.

---

## 1. Modern C++20 Standards & Language Features

- **Standard**: C++20 (`/std:c++20`).
- **Standard Library & Engine Types**:
  - Prefer Engine containers (`TArray`, `TMap`, `TSet`, `TQueue`) over `std::vector`, `std::map`, etc.
  - Utilize `constexpr` and `consteval` for compile-time evaluations where appropriate.
  - Use `auto` only when the type is explicitly obvious on the same line (e.g., casting or iterator loops), otherwise declare explicit types.
  - Use structured bindings and designated initializers where readability is improved.

---

## 2. Pointer Management & Ownership

### UObject Pointers
- **Member Properties**: Always use `TObjectPtr<T>` instead of raw `T*` for all `UPROPERTY()` member variables.
  ```cpp
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
  TObjectPtr<USkeletalMeshComponent> CharacterMesh;
  ```
- **Weak References**: Always use `TWeakObjectPtr<T>` for cached references to other actors or components to prevent memory leaks and dangling pointers.
  ```cpp
  TWeakObjectPtr<APlayerController> CachedPlayerController;
  ```
- **Local Variables & Function Parameters**: Standard raw pointers `T*` or `const T*` are permitted for transient local function variables or function signatures.

### Non-UObject Pointers
- Use Unreal smart pointers:
  - `TSharedPtr<T>`: Shared ownership.
  - `TSharedRef<T>`: Non-null shared reference.
  - `TUniquePtr<T>`: Unique ownership.
  - `TWeakPtr<T>`: Non-owning weak reference to `TSharedPtr`.

---

## 3. UPROPERTY and UFUNCTION Macro Hygiene

### Specifier Ordering
Keep macro specifiers ordered logically:
1. **Access/Exposure**: `EditAnywhere`, `EditDefaultsOnly`, `VisibleAnywhere`, `BlueprintReadOnly`, `BlueprintReadWrite`.
2. **Category**: `Category = "ModuleName|Subsystem"`.
3. **Replication**: `Replicated`, `ReplicatedUsing = OnRep_PropertyName`.
4. **Metadata**: `meta = (ClampMin = "0.0", AllowPrivateAccess = "true")`.

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Combat", meta = (ClampMin = "0.0"))
float BaseDamage = 100.0f;

UFUNCTION(BlueprintCallable, Category = "FC|Combat")
void ApplyDamage(float Amount, AActor* DamageCauser);
```

### Private Access with UPROPERTY
When exposing private member variables to Blueprint, always declare `meta = (AllowPrivateAccess = "true")`.

---

## 4. Include What You Use (IWYU) & Forward Declarations

- **Forward Declarations in Headers**:
  - Never include full class headers in `.h` files unless inheriting from the class or using it as a value type member struct.
  - Always forward declare classes and structs in headers:
    ```cpp
    class USpringArmComponent;
    class UCameraComponent;
    class UInputMappingContext;
    class UInputAction;
    struct FInputActionValue;
    ```
- **Clean Includes in `.cpp`**:
  - Include `.generated.h` as the **last** include in any header file.
  - Include only the specific headers needed in `.cpp`.
  - **Never** include `Engine.h` or monolithic module headers.
  - **Never** include UMG or Slate UI headers in core gameplay actor headers.

---

## 5. Naming Conventions

Strictly adhere to the Unreal Engine prefix rules:

| Prefix | Type | Examples |
| :--- | :--- | :--- |
| **`A`** | Actor classes | `AFCActor`, `AFCCharacter`, `AFCGameMode` |
| **`U`** | UObject classes / Components | `UFCAbilityComponent`, `UFCInventorySystem` |
| **`F`** | Structs and non-UObject classes | `FFCDamageEvent`, `FFCInventoryItem` |
| **`E`** | Enumerations (Scoped `enum class`) | `EFCTeamId`, `EFCCombatState` |
| **`I`** | Interface classes | `IFCInteractableInterface`, `IFCDamageable` |
| **`T`** | Template classes | `TArray`, `TObjectPtr`, `TSubclassOf` |
| **`b`** | Boolean variables | `bIsDead`, `bReplicates`, `bCanSprint` |

---

## 6. Code Structure & Organization

Organize header class sections cleanly using standard access modifiers:

```cpp
UCLASS()
class FC_API AFCCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AFCCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // Public gameplay API
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
    TObjectPtr<USpringArmComponent> CameraBoom;

private:
    // Internal state
    UPROPERTY(ReplicatedUsing = OnRep_Health, Category = "FC|Health")
    float Health = 100.0f;

    UFUNCTION()
    void OnRep_Health(float OldHealth);
};
```
