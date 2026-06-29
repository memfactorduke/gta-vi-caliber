"""
build_gtc_pausemenu.py  —  GT CALIBER full-screen tabbed pause menu (original, NOT a GTA clone)

WHY THIS FILE EXISTS
  The live Unreal MCP can restyle/re-wire EXISTING widgets but cannot ADD widget-tree
  nodes. Building a brand-new tabbed menu therefore needs the in-editor Python console.
  Trigger it with:   py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/build_gtc_pausemenu.py"

  Everything is wrapped in try/except with sentinel log lines (GTC_BUILD_START /
  GTC_BUILD_DONE / GTC_BUILD_FAIL) so the result is readable from the Output Log over MCP.
  All struct construction uses set_editor_property (NOT positional constructors) because
  those are version-sensitive in 5.8.

DESIGN (user-approved): left vertical tab rail + WORLD sandbox tab.
  - Top status header:  GT CALIBER wordmark (left)  |  handle . in-game clock . cash (right)
  - Left rail (cyan highlight on active):  MAP GARAGE ARSENAL STATS WORLD SETTINGS SYSTEM
  - WORLD = sandbox time/weather/teleport  (the "more freedom" hook vs GTA)
  - Big content panel (right): live world map by default (minimap render-target wired later)
  - Palette: dark-glass panels + cyan accent; full-screen blurred game behind.
  Identity is ours throughout — no GTA names/layout/art.
"""
import traceback
import unreal

PKG_PATH   = "/Game/UI"
ASSET_NAME = "WBP_GTC_PauseMenu"
FULL_PATH  = PKG_PATH + "/" + ASSET_NAME

# ---- palette ---------------------------------------------------------------
GLASS  = unreal.LinearColor(0.035, 0.042, 0.055, 0.97)   # dark-glass panel
GLASS2 = unreal.LinearColor(0.09,  0.10,  0.125, 0.90)   # rail / tiles
HOVER  = unreal.LinearColor(0.20,  0.30,  0.36, 0.95)
CYAN   = unreal.LinearColor(0.16,  0.78,  0.92, 1.0)     # accent (active tab)
INK    = unreal.LinearColor(0.92,  0.95,  0.98, 1.0)     # primary text
DARK   = unreal.LinearColor(0.02,  0.03,  0.04, 1.0)     # text on cyan
DIM    = unreal.LinearColor(0.62,  0.68,  0.74, 1.0)     # inactive text
SCRIM  = unreal.LinearColor(0.0,   0.0,   0.0,  0.55)    # behind-everything dim

TABS   = ["MAP", "GARAGE", "ARSENAL", "STATS", "WORLD", "SETTINGS", "SYSTEM"]
ACTIVE = "MAP"
ROBOTO = unreal.load_asset("/Engine/EngineFonts/Roboto.Roboto")


def font(size, typeface="Bold", spacing=0):
    f = unreal.SlateFontInfo()
    f.set_editor_property("font_object", ROBOTO)
    f.set_editor_property("typeface_font_name", typeface)
    f.set_editor_property("size", int(size))
    f.set_editor_property("letter_spacing", int(spacing))
    return f


def brush(color, draw="rounded"):
    b = unreal.SlateBrush()
    b.set_editor_property("tint_color", unreal.SlateColor(color))
    try:
        b.set_editor_property(
            "draw_as",
            unreal.SlateBrushDrawType.ROUNDED_BOX if draw == "rounded"
            else unreal.SlateBrushDrawType.IMAGE)
    except Exception:
        pass
    return b


def anchors(minx, miny, maxx, maxy):
    a = unreal.Anchors()
    a.set_editor_property("minimum", unreal.Vector2D(minx, miny))
    a.set_editor_property("maximum", unreal.Vector2D(maxx, maxy))
    return a


def child_size_fill(value=1.0):
    s = unreal.SlateChildSize()
    s.set_editor_property("value", float(value))
    s.set_editor_property("size_rule", unreal.SlateSizeRule.FILL)
    return s


def acquire_tree(bp):
    """UE5.8 made UWidgetBlueprint.WidgetTree protected for Python. Try several
    documented routes and log which one works so a single run is definitive."""
    errs = []
    try:
        t = bp.get_editor_property("WidgetTree")
        if t is not None:
            unreal.log("TREE_VIA get_editor_property"); return t
    except Exception as e:
        errs.append("get_editor_property: " + repr(e))
    try:
        t = getattr(bp, "widget_tree", None)
        if t is not None:
            unreal.log("TREE_VIA attr widget_tree"); return t
    except Exception as e:
        errs.append("attr: " + repr(e))
    # The runtime CDO's WidgetTree (UUserWidget.WidgetTree) is BlueprintReadOnly,
    # not protected — readable, and for a fresh WBP it is the editable archetype.
    try:
        gc = bp.get_editor_property("GeneratedClass")
        cdo = unreal.get_default_object(gc)
        t = cdo.get_editor_property("WidgetTree")
        if t is not None:
            unreal.log("TREE_VIA generated-class CDO"); return t
    except Exception as e:
        errs.append("cdo: " + repr(e))
    # Last resort: construct a new tree and assign it.
    try:
        t = unreal.new_object(unreal.WidgetTree, bp)
        bp.set_editor_property("WidgetTree", t)
        unreal.log("TREE_VIA new_object+set"); return t
    except Exception as e:
        errs.append("new+set: " + repr(e))
    raise Exception("TREE_ACQUIRE_FAILED: " + " | ".join(errs))


def build():
    atools = unreal.AssetToolsHelpers.get_asset_tools()
    if unreal.EditorAssetLibrary.does_asset_exist(FULL_PATH):
        unreal.EditorAssetLibrary.delete_asset(FULL_PATH)

    if unreal.EditorAssetLibrary.does_asset_exist(FULL_PATH):
        ok = unreal.EditorAssetLibrary.delete_asset(FULL_PATH)
        unreal.log("BP_DELETE_EXISTING -> " + str(ok))

    fac = unreal.WidgetBlueprintFactory()
    fac.set_editor_property("parent_class", unreal.UserWidget)
    bp = atools.create_asset(ASSET_NAME, PKG_PATH, unreal.WidgetBlueprint, fac)
    if bp is None:
        unreal.log("BP_CREATE_NONE -> loading existing")
        bp = unreal.load_asset(FULL_PATH)
    if bp is None:
        raise Exception("BP_UNAVAILABLE: create_asset returned None and load_asset failed")
    unreal.log("BP_OK " + str(type(bp).__name__))

    # Compile once so GeneratedClass / CDO exist (needed for the CDO tree route).
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as e:
        unreal.log("BP_PRECOMPILE_WARN " + repr(e))

    tree = acquire_tree(bp)
    unreal.log("TREE_TYPE " + str(type(tree).__name__))

    def mk(cls):
        return tree.construct_widget(cls)

    # ---- root canvas (full screen) -----------------------------------------
    root = mk(unreal.CanvasPanel)
    tree.set_editor_property("root_widget", root)

    def fill(child, l=0, t=0, r=0, b=0):
        slot = root.add_child_to_canvas(child)
        slot.set_editor_property("anchors", anchors(0, 0, 1, 1))
        slot.set_editor_property("offsets", unreal.Margin(l, t, r, b))
        return slot

    # 1) blurred game behind + scrim
    blur = mk(unreal.BackgroundBlur)
    blur.set_editor_property("blur_strength", 16.0)
    fill(blur)

    scrim = mk(unreal.Image)
    sb = unreal.SlateBrush()
    sb.set_editor_property("tint_color", unreal.SlateColor(SCRIM))
    scrim.set_editor_property("brush", sb)
    fill(scrim)

    # 2) top status header ----------------------------------------------------
    header = mk(unreal.HorizontalBox)
    hs = root.add_child_to_canvas(header)
    hs.set_editor_property("anchors", anchors(0, 0, 1, 0))
    hs.set_editor_property("offsets", unreal.Margin(56, 34, 56, 0))
    hs.set_editor_property("size", unreal.Vector2D(0, 60))
    hs.set_editor_property("auto_size", False)

    wordmark = mk(unreal.TextBlock)
    wordmark.set_text("GT CALIBER")
    wordmark.set_font(font(34, "Bold", 220))
    wordmark.set_color_and_opacity(unreal.SlateColor(INK))
    header.add_child_to_horizontal_box(wordmark)

    spacer = mk(unreal.Spacer)
    sp = header.add_child_to_horizontal_box(spacer)
    sp.set_editor_property("size", child_size_fill(1.0))

    status = mk(unreal.TextBlock)
    status.set_text("KANE        SUN 07:07        $1,056")
    status.set_font(font(20, "Regular", 60))
    status.set_color_and_opacity(unreal.SlateColor(DIM))
    header.add_child_to_horizontal_box(status)

    # 3) left tab rail --------------------------------------------------------
    rail_bg = mk(unreal.Border)
    rail_bg.set_brush_color(GLASS)
    rs = root.add_child_to_canvas(rail_bg)
    rs.set_editor_property("anchors", anchors(0, 0, 0, 1))
    rs.set_editor_property("offsets", unreal.Margin(56, 118, 0, 64))
    rs.set_editor_property("size", unreal.Vector2D(300, 0))
    rs.set_editor_property("auto_size", False)

    rail = mk(unreal.VerticalBox)
    rail_bg.set_content(rail)
    try:
        rail_bg.set_editor_property("padding", unreal.Margin(16, 18, 16, 18))
    except Exception:
        pass

    for name in TABS:
        active = (name == ACTIVE)
        btn = mk(unreal.Button)
        sty = btn.get_editor_property("widget_style")
        sty.set_editor_property("normal",  brush(CYAN if active else GLASS2))
        sty.set_editor_property("hovered", brush(HOVER))
        sty.set_editor_property("pressed", brush(CYAN))
        btn.set_editor_property("widget_style", sty)

        lbl = mk(unreal.TextBlock)
        lbl.set_text(name)
        lbl.set_font(font(24, "Bold", 90))
        lbl.set_color_and_opacity(unreal.SlateColor(DARK if active else INK))
        btn.set_content(lbl)

        bs = rail.add_child_to_vertical_box(btn)
        bs.set_editor_property("padding", unreal.Margin(0, 0, 0, 10))

    # 4) content panel (right) ------------------------------------------------
    content = mk(unreal.Border)
    content.set_brush_color(GLASS)
    cs = root.add_child_to_canvas(content)
    cs.set_editor_property("anchors", anchors(0, 0, 1, 1))
    cs.set_editor_property("offsets", unreal.Margin(372, 118, 56, 64))

    ph = mk(unreal.TextBlock)
    ph.set_text("WORLD  MAP")
    ph.set_font(font(46, "Bold", 240))
    ph.set_color_and_opacity(unreal.SlateColor(DIM))
    content.set_content(ph)
    # TODO live: swap placeholder for an Image bound to RT_Minimap; add a WidgetSwitcher
    #            driven by the rail buttons' OnClicked for per-tab content.

    # ---- compile + save -----------------------------------------------------
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_asset(FULL_PATH)
    return FULL_PATH


unreal.log("GTC_BUILD_START")
try:
    path = build()
    unreal.log("GTC_BUILD_DONE " + path)
except Exception as exc:
    unreal.log_error("GTC_BUILD_FAIL " + repr(exc))
    unreal.log_error(traceback.format_exc())
