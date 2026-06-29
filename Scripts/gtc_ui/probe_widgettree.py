import unreal

bp = unreal.load_asset("/Game/UI/WBP_GTC_PauseMenu")
unreal.log("PROBE_START " + str(type(bp).__name__))

def attempt(label, fn):
    try:
        v = fn()
        unreal.log("PROBE_OK %s -> %s" % (label, (type(v).__name__ if v is not None else "None")))
        return v
    except Exception as e:
        unreal.log("PROBE_ERR %s -> %s" % (label, repr(e)))
        return None

attempt("get_editor_property WidgetTree", lambda: bp.get_editor_property("WidgetTree"))
attempt("attr widget_tree", lambda: bp.widget_tree)
attempt("get_editor_property generated_class", lambda: bp.get_editor_property("GeneratedClass"))

attrs = [a for a in dir(bp) if any(k in a.lower() for k in ("tree", "widget", "root", "panel"))]
unreal.log("PROBE_ATTRS " + ",".join(attrs))

libs = [a for a in dir(unreal) if ("Widget" in a or "UMG" in a) and ("Library" in a or "Subsystem" in a or "Editor" in a)]
unreal.log("PROBE_LIBS " + ",".join(libs))

# Does WidgetBlueprintEditorSubsystem or a library expose a way in?
for s in ("WidgetBlueprintEditorSubsystem", "UMGEditorSubsystem", "WidgetBlueprintLibrary", "EditorWidgetUtilities"):
    unreal.log("PROBE_HAS unreal.%s = %s" % (s, hasattr(unreal, s)))

unreal.log("PROBE_DONE")
