# 01. Unreal Engine 5.8 C++20 Coding Standards

All C++ changes in `Source/FC/` must comply with UE 5.8 C++20 (`/std:c++20`) and the project's deterministic anti-slop linters.

## 1. Types & Pointer Ownership
- **Engine Containers**: Prefer `TArray`, `TMap`, `TSet`, `TQueue` over STL (`std::vector`, `std::map`).
- **`auto` Discipline**: Use `auto` only when the type is obvious on the same line (casts, range-for iterators).
- **UObject Member Properties**: Always use `TObjectPtr<T>` instead of raw `T*` in `UPROPERTY()` declarations (`UE58_RAW_POINTER_UPROPERTY` error).
- **Non-Owning Cached References**: Use `TWeakObjectPtr<T>` for cached Actors/Components to prevent dangling pointers and GC cycles. Raw `T*` / `const T*` is allowed only for transient locals and function parameters.
- **Non-UObject Smart Pointers**: Use `TSharedPtr`, `TSharedRef`, `TUniquePtr`, `TWeakPtr`.

## 2. UPROPERTY / UFUNCTION & Naming Hygiene
- **Specifier Order**: Access (`EditDefaultsOnly`, `VisibleAnywhere`, `BlueprintReadOnly`) -> `Category = "FC|Subsystem"` -> Replication (`ReplicatedUsing = OnRep_X`) -> `meta = (ClampMin = "0.0", AllowPrivateAccess = "true")`.
- **Private Exposure**: Private Blueprint-exposed `UPROPERTY` members must specify `meta = (AllowPrivateAccess = "true")`.
- **Prefixes**: `A` (Actor), `U` (UObject/Component), `F` (Struct/plain type), `E` (scoped `enum class`), `I` (Interface), `T` (Template), `b` (boolean).

## 3. Include What You Use (IWYU)
- **Headers (`.h`)**: Forward-declare classes/structs (`class USpringArmComponent;`) whenever not inheriting or embedding by value. Keep `.generated.h` as the **last** `#include`.
- **Sources (`.cpp`)**: Include only concrete headers used. Never include `Engine.h` or monolithic module headers. Never include UMG/Slate/ViewModel headers in server/gameplay/GAS headers or server `.cpp` files.

## 4. Minimal Footprint, Flat Control Flow & Zero Escape-Hatch Comments
- **Minimal Diff**: Edit only what the task requires; do not reformat or refactor unrelated working code.
- **Flat Control Flow (Anti-Arrow Pattern)**: Avoid deeply nested `if` chains (e.g., cascading `nullptr`/validity checks). Flatten control flow using Guard Clauses (early returns), immediately-invoked local lambdas (IIFE), or shared extraction helpers (e.g., struct static unpackers) without introducing unnecessary single-use wrapper methods.
- **SSOT & Reuse**: Survey existing subsystems and DataAssets (`AGENTS.md` §4) before introducing new classes. Never duplicate ID lists (Starter Decks, Ability IDs, Status Definitions) across C++ classes—read from a single `UDataTable` or `UPrimaryDataAsset`.
- **Zero Escape-Hatch Comments** (`CODE_ESCAPE_HATCH_COMMENT` error): Keep valid `/** ... */` UHT/API docstrings, but never leave `// TODO`, `// FIXME`, `// HACK`, `// XXX`, `// Workaround`, or `// For now` comments.

## 5. Banned Footgun APIs (Hard `ERROR` in `verify_replication.py`)
1. **Index-0 Single-Player Getters** (`CODE_BANNED_INDEX0_PLAYER_API`): Never call `UGameplayStatics::GetPlayerController/GetPlayerPawn/GetPlayerCharacter(World, 0)`. Resolve via `GetController()`, `GetOwningPlayer()`, or Server RPC context.
2. **Raw `this` Delegate Lambdas** (`CODE_UNSAFE_RAW_THIS_LAMBDA`): Never call `.AddLambda([this, ...])` or `.BindLambda([this, ...])`. Use `.AddWeakLambda(this, ...)` / `.BindWeakLambda(this, ...)`.
3. **Hardcoded Constructor Asset Paths** (`CODE_HARDCODED_ASSET_PATH`): Never add `ConstructorHelpers::FObjectFinder` or `FClassFinder` in C++ constructors. Configure assets via `TSoftObjectPtr`, `UPrimaryDataAsset`, or Blueprint Class Defaults (`BP_*`). *(Pre-existing legacy items are frozen in `Scripts/linter_baseline.json`; 0 new items allowed.)*
