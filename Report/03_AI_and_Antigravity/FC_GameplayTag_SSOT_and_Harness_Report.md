# FC GameplayTag SSOT 정비 및 하네스 정적 검증 보충 보고서

## 1. 배경 및 문제 진단

프로젝트의 GameplayTag 관리 체계를 점검한 결과, 다음과 같은 구조적 문제가 확인되었습니다.

1. **Native ↔ INI 이중 정의 (SSOT 부재)**
   - `Config/DefaultGameplayTags.ini`에 등록된 43개 태그 전부가 `Source/FC/Private/Data/FCGameplayTags.cpp`에 `UE_DEFINE_GAMEPLAY_TAG_COMMENT`로도 동일하게 등록되어 있었습니다(C++에는 4개 루트 태그 포함 총 47개 정의).
   - 한쪽만 수정·삭제할 경우 싱크가 깨지며, `DefaultGameplayTags.ini`만 확인하면 C++ 전용 루트 태그나 분산된 태그를 파악하기 어려웠습니다.
2. **단일 파일 집중으로 인한 머지 충돌 및 재컴파일 위험**
   - 모든 태그를 `DefaultGameplayTags.ini` 또는 중앙 `FCGameplayTags.h/.cpp` 한 곳에 누적할 경우, 병렬 작업/브랜치 간 Git 머지 충돌이 발생하기 쉽고 `FCGameplayTags.h` 수정 시 다수 C++ 파일의 연쇄 재컴파일이 유발됩니다.

---

## 2. 아키텍처 개선 및 규칙 정립

### 2-1. 계층별 태그 배치 기준 (SSOT)
| 태그 성격 | 배치 위치 | 비고 |
| :--- | :--- | :--- |
| **전역 코어 태그** (`Element.*`, `Class.*`, `Card.Type.*`, `Combat.Team.*`, `Data.*`) | `FCGameplayTags.h` / `FCGameplayTags.cpp` | 여러 시스템이 공통 참조하는 저빈도 변경 태그 |
| **단일 `.cpp` 내부 전용 태그** | 해당 `.cpp` 내부 `UE_DEFINE_GAMEPLAY_TAG_STATIC` | 헤더 수정 없음, 머지 충돌 및 연쇄 재컴파일 0 |
| **에디터/데이터/기능 확장 태그** (C++ 미참조) | `Config/Tags/<Feature>.ini` | `DefaultGameplayTags.ini` 단일 파일 누적 방지 |

### 2-2. 적용된 변경 사항
1. **`Config/DefaultGameplayTags.ini` 중복 제거**: C++ Native 태그와 중복되던 43개 `+GameplayTagList` 항목을 제거하고 SSOT 안내 주석으로 정리했습니다.
2. **도메인 규칙 보충 (`.antigravity/rules/04-gameplay-ability-system.md`)**: `3. Gameplay Tag Taxonomy & SSOT` 섹션에 배치 기준과 중복/하드코딩 금지 규칙을 명시했습니다.
3. **하네스 정적 린터 보충 (`Scripts/verify_gas.py`)**:
   - `TAG_DUPLICATE_NATIVE_INI`: `Source/FC/` 전체의 `UE_DEFINE_GAMEPLAY_TAG*` 정의와 `Config/DefaultGameplayTags.ini` 및 `Config/Tags/*.ini` 간 중복을 검출합니다.
   - `TAG_RAW_REQUEST_CALL`: C++ 코드 내 `FGameplayTag::RequestGameplayTag()` 문자열 조회를 차단하고 Native 태그 심볼 사용을 강제합니다.
4. **하네스 메타 회귀 테스트 추가 (`Scripts/tests/test_linters.py`)**:
   - `test_catches_raw_request_gameplay_tag`
   - `test_catches_duplicate_native_and_ini_gameplay_tag`
   - `test_passes_split_native_and_feature_ini_tags`

---

## 3. 검증 결과

| 검증 항목 | 실행 명령 | 결과 |
| :--- | :--- | :--- |
| **하네스 메타 테스트** | `python Scripts/tests/test_linters.py` | **OK** (`20 tests in 0.160s`) |
| **전체 정적 게이트** | `python Scripts/pipeline.py static` | **PASS** (`0 error(s), 0 warning(s)`) |
| **UE 헤드리스 자동화 테스트** | `python Scripts/pipeline.py test` | **PASSED** (`26.17s`) |

---

## 4. 태그 조회 가이드

- **에디터 내 전체 트리 및 출처 확인**: `Project Settings` → `GameplayTags` → `Manage Gameplay Tags...`에서 C++ Native, INI(`Config/Tags/*.ini`), 플러그인 태그 전체 계층과 각 태그의 Source(`Native` / `.ini`)를 확인할 수 있습니다.
- **코드 검색 시**: `DefaultGameplayTags.ini`뿐만 아니라 `FCGameplayTags.h` 및 각 `.cpp`의 `UE_DEFINE_GAMEPLAY_TAG*` 매크로를 함께 확인해야 합니다.
