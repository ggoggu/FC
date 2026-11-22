"""Project-specific Unreal Editor AI Toolset for FC (UE 5.8).

Registered automatically via Content/Python/init_unreal.py into UToolsetRegistry
and exposed over unreal-mcp (`list_toolsets` -> `fc_toolset.FCTools`).
"""

from __future__ import annotations

import json
import unreal
import toolset_registry


def _normalize_game_path(asset_path: str) -> str:
    """Normalizes 'Content/.../*.uasset' or '/Game/...' into a UE package path."""
    clean = asset_path.strip().replace("\\", "/")
    if clean.startswith("Content/"):
        clean = "/Game/" + clean[len("Content/"):]
    elif clean.startswith("./Content/"):
        clean = "/Game/" + clean[len("./Content/"):]
    if clean.endswith(".uasset"):
        clean = clean[:-len(".uasset")]
    if "." in clean.rsplit("/", 1)[-1]:
        clean = clean.split(".", 1)[0]
    return clean


def _load_asset_checked(asset_path: str) -> tuple[str, unreal.Object]:
    game_path = _normalize_game_path(asset_path)
    asset = unreal.EditorAssetLibrary.load_asset(game_path)
    if not asset:
        raise ValueError(f"Asset not found or failed to load: {game_path}")
    return game_path, asset


def _collect_widget_tree(widget_tree: unreal.WidgetTree) -> tuple[str | None, list[dict]]:
    root = widget_tree.get_editor_property("root_widget") if widget_tree else None
    if not root:
        return None, []

    entries: list[dict] = []

    def _walk(widget: unreal.Widget, parent_name: str | None) -> None:
        slot = getattr(widget, "slot", None)
        entries.append({
            "name": widget.get_name(),
            "class": widget.get_class().get_name(),
            "parent": parent_name,
            "slot_class": slot.get_class().get_name() if slot else None,
            "is_variable": bool(widget.get_editor_property("is_variable")),
        })
        if hasattr(widget, "get_all_children"):
            for child in widget.get_all_children():
                _walk(child, widget.get_name())

    _walk(root, None)
    return root.get_name(), entries


def _collect_widget_mvvm(widget_bp: unreal.WidgetBlueprint) -> dict:
    info: dict = {"viewmodels": [], "bindings_count": 0}
    if hasattr(unreal, "MVVMToolset"):
        try:
            vms = unreal.MVVMToolset.list_widget_view_models(widget_bp)
            info["viewmodels"] = [vm.get_path_name() for vm in vms]
            bindings = unreal.MVVMToolset.list_widget_view_bindings(widget_bp)
            info["bindings_count"] = len(bindings)
            return info
        except Exception:
            pass
    return info


def _summarize_widget_blueprint(widget_bp: unreal.WidgetBlueprint) -> dict:
    parent_cls = widget_bp.get_blueprint_parent_class()
    widget_tree = widget_bp.get_editor_property("widget_tree")
    root_name, widgets = _collect_widget_tree(widget_tree)
    return {
        "parent_class": parent_cls.get_path_name() if parent_cls else None,
        "root_widget": root_name,
        "widget_count": len(widgets),
        "widgets": widgets,
        "mvvm": _collect_widget_mvvm(widget_bp),
    }


def _summarize_blueprint(bp: unreal.Blueprint) -> dict:
    parent_cls = bp.get_blueprint_parent_class()
    gen_cls = bp.generated_class()
    graphs = list(unreal.BlueprintEditorLibrary.list_graph_names(bp))
    variables = list(unreal.BlueprintEditorLibrary.list_member_variable_names(bp, False))
    return {
        "parent_class": parent_cls.get_path_name() if parent_cls else None,
        "generated_class": gen_cls.get_path_name() if gen_cls else None,
        "graphs": [str(g) for g in graphs],
        "variables": [str(v) for v in variables],
    }


def _summarize_anim_montage(montage: unreal.AnimMontage) -> dict:
    notifies = []
    for notify in montage.get_editor_property("notifies"):
        notifies.append({
            "name": str(notify.get_editor_property("notify_name")),
            "trigger_time": round(float(notify.get_editor_property("trigger_time_offset")), 3),
        })
    slots = [str(track.get_editor_property("slot_name")) for track in montage.get_editor_property("slot_anim_tracks")]
    return {
        "sequence_length": round(float(montage.get_editor_property("sequence_length")), 3),
        "slots": slots,
        "notifies": notifies,
    }


def _summarize_datatable(table: unreal.DataTable) -> dict:
    row_struct = table.get_row_struct()
    row_names = [str(name) for name in table.get_row_names()]
    return {
        "row_struct": row_struct.get_path_name() if row_struct else None,
        "row_count": len(row_names),
        "row_names_preview": row_names[:50],
    }


@unreal.uclass()
class FCTools(unreal.ToolsetDefinition):
    """Project-specific live inspection tools for FC (WBP, BP, MVVM, DataTables)."""

    @toolset_registry.tool_call
    @staticmethod
    def inspect_asset_summary(asset_path: str) -> str:
        """Returns a live-editor Ground Truth JSON summary for a WBP, BP, AnimMontage, or DataTable.

        Args:
            asset_path: Asset path as '/Game/...' or 'Content/.../*.uasset'.

        Returns:
            JSON string with live reflection metadata (widgets, MVVM, graphs, variables, notifies, or rows).
        """
        game_path, asset = _load_asset_checked(asset_path)
        payload: dict = {
            "asset_name": asset.get_name(),
            "game_path": game_path,
            "asset_class": asset.get_class().get_name(),
        }

        if isinstance(asset, unreal.WidgetBlueprint):
            payload.update(_summarize_widget_blueprint(asset))
        elif isinstance(asset, unreal.Blueprint):
            payload.update(_summarize_blueprint(asset))
        elif isinstance(asset, unreal.AnimMontage):
            payload.update(_summarize_anim_montage(asset))
        elif isinstance(asset, unreal.DataTable):
            payload.update(_summarize_datatable(asset))

        return json.dumps(payload, indent=2, ensure_ascii=False)

    @toolset_registry.tool_call
    @staticmethod
    def list_project_viewmodels() -> list[str]:
        """Lists native C++ and Blueprint MVVM ViewModel classes in the FC project.

        Returns:
            Sorted list of class path strings derived from UMVVMViewModelBase.
        """
        vm_base = unreal.load_class(None, "/Script/ModelViewViewModel.MVVMViewModelBase")
        if not vm_base:
            raise RuntimeError("ModelViewViewModel plugin class MVVMViewModelBase is not loaded.")
        derived = unreal.ToolsetLibrary.get_derived_classes(vm_base)
        paths = [c.export_text() for c in derived]
        project_paths = [
            p for p in paths
            if "/Script/FC" in p or p.startswith("/Game/")
        ]
        return sorted(project_paths)

    @toolset_registry.tool_call
    @staticmethod
    def read_datatable_rows(
        table_path: str = "/Game/Card/DT_CardCatalog",
        row_filter: str = "",
        limit: int = 30,
    ) -> str:
        """Reads rows from a project DataTable (e.g. DT_CardCatalog) as JSON.

        Args:
            table_path: DataTable asset path ('/Game/...' or 'Content/.../*.uasset').
            row_filter: Optional case-insensitive substring filter on row Name.
            limit: Maximum number of rows to return (clamped to 1..100).

        Returns:
            JSON string containing row_struct, total_rows, and matching rows.
        """
        game_path, asset = _load_asset_checked(table_path)
        if not isinstance(asset, unreal.DataTable):
            raise TypeError(f"Expected DataTable at {game_path}, got {asset.get_class().get_name()}")

        raw_json = asset.export_to_json_string()
        rows = json.loads(raw_json) if raw_json else []
        if row_filter:
            needle = row_filter.lower()
            rows = [r for r in rows if needle in str(r.get("Name", "")).lower()]

        safe_limit = max(1, min(int(limit), 100))
        row_struct = asset.get_row_struct()
        return json.dumps({
            "table": game_path,
            "row_struct": row_struct.get_path_name() if row_struct else None,
            "matched_rows": len(rows),
            "rows": rows[:safe_limit],
        }, indent=2, ensure_ascii=False)
