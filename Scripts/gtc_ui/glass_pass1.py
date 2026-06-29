"""
glass_pass1.py — glassmorphism pass + left-nav/right-container layout.

Per user direction (2026-06-29): frosted glass + settings-on-left / content-on-right.
1. Reveal MAIN pause for verification (Options_canvas HIDDEN, Main_window_canvas VISIBLE).
2. GTC_BackdropBlur -> stronger blur (the "frost").
3. GTC_NavGlass -> NEW frosted rail behind the left buttons (reads as the nav container).
4. Frosted glass on the right content frames: GTC_RightPanel, GTC_StatusFrame,
   GTC_SettingsFrame, GTC_LoadFrame, Options_background.
Compiles the pause widget (no save; reveal toggles still applied -> revert before save).

Run:  py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/glass_pass1.py"
"""
import json, traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
TREE = PKG + ".WB_Pause_menu_widget:WidgetTree"
BASE = TREE + "."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/glass_pass1.json"

GLASS_FILL = (0.09, 0.12, 0.16, 0.45)     # translucent dark glass (blurred world shows through)
GLASS_BORDER = (0.60, 0.74, 0.85, 0.35)   # soft cool-white hairline
NAV_FILL = (0.06, 0.08, 0.11, 0.52)       # slightly more opaque for the nav rail
FRAMES = ["GTC_RightPanel", "GTC_StatusFrame", "GTC_SettingsFrame", "GTC_LoadFrame", "Options_background"]

res = {"frames": {}, "notes": []}


def lin(c):
    return unreal.LinearColor(c[0], c[1], c[2], c[3])


def frost(img, fill, radius=14.0, bw=1.3):
    b = img.get_editor_property("brush")
    try:
        b.set_editor_property("draw_as", unreal.SlateBrushDrawType.ROUNDED_BOX)
        out = unreal.SlateBrushOutlineSettings()
        out.set_editor_property("corner_radii", unreal.Vector4(radius, radius, radius, radius))
        out.set_editor_property("color", unreal.SlateColor(lin(GLASS_BORDER)))
        out.set_editor_property("width", bw)
        out.set_editor_property("rounding_type", unreal.SlateBrushRoundingType.FIXED_RADIUS)
        b.set_editor_property("outline_settings", out)
    except Exception as e:
        res["notes"].append("outline_err: " + repr(e))
    b.set_editor_property("tint_color", unreal.SlateColor(lin(fill)))
    img.set_editor_property("brush", b)


def canvas_anchor(w, ax, ay, bx, by, z):
    s = w.slot
    a = unreal.Anchors()
    a.set_editor_property("minimum", unreal.Vector2D(ax, ay))
    a.set_editor_property("maximum", unreal.Vector2D(bx, by))
    s.set_anchors(a)
    try:
        s.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
    except Exception:
        s.set_editor_property("offsets", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    s.set_alignment(unreal.Vector2D(0.0, 0.0))
    s.set_z_order(z)


try:
    tree = unreal.load_object(None, TREE)
    mwc = unreal.load_object(None, BASE + "Main_window_canvas")

    # (1) reveal MAIN pause for verification
    try:
        unreal.load_object(None, BASE + "Options_canvas").set_visibility(unreal.SlateVisibility.HIDDEN)
        mwc.set_visibility(unreal.SlateVisibility.VISIBLE)
        res["notes"].append("revealed main pause (options hidden, main visible)")
    except Exception as e:
        res["notes"].append("reveal_err: " + repr(e))

    # (2) stronger backdrop blur
    try:
        blur = unreal.load_object(None, BASE + "GTC_BackdropBlur")
        if blur:
            blur.set_editor_property("blur_strength", 22.0)
            res["blur"] = 22.0
        else:
            res["notes"].append("GTC_BackdropBlur not found")
    except Exception as e:
        res["notes"].append("blur_err: " + repr(e))

    # (3) left frosted nav rail (behind buttons at z=7)
    try:
        nav = None
        for i in range(mwc.get_children_count()):
            ch = mwc.get_child_at(i)
            if ch and ch.get_name() == "GTC_NavGlass":
                nav = ch
                break
        if nav is None:
            nav = unreal.new_object(unreal.Image, tree, "GTC_NavGlass")
            mwc.add_child(nav)
        frost(nav, NAV_FILL, radius=16.0, bw=1.2)
        canvas_anchor(nav, 0.035, 0.145, 0.402, 0.875, 2)
        res["nav_rail"] = "ok"
    except Exception as e:
        res["notes"].append("nav_err: " + repr(e))

    # (4) frosted glass on content frames
    for fn in FRAMES:
        try:
            img = unreal.load_object(None, BASE + fn)
            if img is None:
                res["frames"][fn] = "missing"
                continue
            frost(img, GLASS_FILL)
            res["frames"][fn] = "frosted"
        except Exception as e:
            res["frames"][fn] = "err " + repr(e)

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    res["_ok"] = True
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_GLASS_PASS1_DONE " + json.dumps(res.get("frames", {})))
