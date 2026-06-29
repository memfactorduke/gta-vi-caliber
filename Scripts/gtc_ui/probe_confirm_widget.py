"""
probe_confirm_widget.py — dump the WB_Confirmation_Widget tree (names + types + key style
hints) so the confirmation dialog can be restyled to match the dark-glass/cyan pause menu.
Probe only. Run: py "<this path>"
Sentinels: GTC_CW_*
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Confirmation_Widget"
OBJ = PKG + ".WB_Confirmation_Widget:WidgetTree"

unreal.log("GTC_CW_START")
try:
    tree = unreal.load_object(None, OBJ)
    if tree is None:
        raise Exception("no WidgetTree at " + OBJ)
    root = tree.get_editor_property("root_widget")
    unreal.log("GTC_CW_ROOT %s (%s)" % (root.get_name() if root else None,
               type(root).__name__ if root else None))

    def walk(w, depth):
        if w is None:
            return
        line = "GTC_CW_W %s%s : %s" % ("  " * depth, w.get_name(), type(w).__name__)
        # surface a couple of style-relevant bits for backgrounds/borders/text
        try:
            if isinstance(w, unreal.TextBlock):
                line += " text='%s'" % str(w.get_text())
        except Exception:
            pass
        unreal.log(line)
        try:
            n = w.get_children_count()
        except Exception:
            n = 0
        for i in range(n):
            try:
                walk(w.get_child_at(i), depth + 1)
            except Exception as ce:
                unreal.log("GTC_CW_CHILDERR %s" % repr(ce))

    walk(root, 0)
    unreal.log("GTC_CW_DONE")
except Exception as e:
    unreal.log_error("GTC_CW_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
