"""Auto-registers project-specific AI Toolsets when Unreal Editor starts."""

import unreal


def _register_project_toolsets() -> None:
    if not hasattr(unreal, "ToolsetDefinition"):
        return
    try:
        from toolset_registry.registration import Registration
        import fc_toolset
    except Exception as exc:
        unreal.log_warning(f"[FCTools] Failed to register project toolsets: {exc}")
        return

    Registration([fc_toolset.FCTools]).register()


_register_project_toolsets()
