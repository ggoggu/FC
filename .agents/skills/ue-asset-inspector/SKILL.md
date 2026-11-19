---
name: ue-asset-inspector
description: >-
  Inspects Unreal Engine 5.8 BP, WBP, Animation (MM/ABP), MVVM, and DataTable assets
  using Live Editor MCP (`wildcard-mcp`) first for exact reflection/graph truth, with
  offline binary scanning (`Scripts/dump_asset_metadata.py`) as a ~0.005s fallback.
---

# UE Asset Inspector Skill

## Tiered Inspection Priority (Ground Truth First)

### 1. Primary: Live Editor MCP (`wildcard-mcp`, when Unreal Editor is running)
Use `wildcard-mcp` (`list_toolsets` / `describe_toolset` / `call_tool`) whenever the Editor is running for 100% accurate live reflection, full widget trees, slot properties, MVVM bindings, and Blueprint graph logic:
- **Unified Project Summary (`fc_toolset.FCTools`)**:
  - `inspect_asset_summary(asset_path)` — 1-call Ground Truth JSON summary for `WBP_*`, `BP_*`, `MM_*`, or `DT_*`.
  - `list_project_viewmodels()` — lists native C++ & BP `UMVVMViewModelBase` subclasses.
  - `read_datatable_rows(table_path, row_filter, limit)` — reads `DT_CardCatalog` or other project DataTable rows.
- **Widget Blueprints (`UMGToolSet.UMGToolSet` + `ObjectTools` + `MVVMToolset`)**:
  - `GetWidgetDescription` / `GetWidgets` for exact depth-first widget hierarchy and `Slot` refs.
  - **Rule**: Always call `ObjectTools.list_properties(widget_or_slot)` before reading/writing widget or slot properties.
  - `MVVMToolset.UMVVMToolset` (`ListWidgetViewModels`, `ListWidgetViewBindings`) for MVVM bindings.
- **Actor / Object Blueprints (`editor_toolset.toolsets.blueprint.BlueprintTools`)**:
  - `get_parent`, `list_variables`, `get_variable_replication` for inheritance & replicated properties.
  - `find_nodes` -> `get_connected_subgraph` / `read_graph_dsl` for exact graph logic without dumping huge graphs whole.

### 2. Fallback: Offline Fast Scanner (`Scripts/dump_asset_metadata.py` / `Scripts/pipeline.py inspect`)
Use when Unreal Editor / `wildcard-mcp` is unavailable or when you only need a read-only, ~0.005s (0ms `mtime_ns` cached) quick check of names, notifies, or tags:
- Command: `python Scripts/dump_asset_metadata.py --json --asset <path>`
- Pipeline report: `python Scripts/pipeline.py inspect --asset <path>`
- **Limitation**: Fast mode scans binary strings (heuristic, sliced); do not rely on it for exact widget slot properties or Blueprint graph wiring. Avoid `--deep` (`UnrealEditor-Cmd.exe`) when live `wildcard-mcp` is already available.

## When to Execute
- Before implementing or editing C++ code that references Widget Blueprint components, slots, or animations.
- Before binding or refactoring `UMVVMViewModelBase` properties used by `WBP_*`.
- When verifying component hierarchies, parent classes, or replicated variables in Blueprints (`BP_*`).
- When checking animation slot names (`DefaultGroup`, `UpperBody`) or notify events (`AN_*`).
