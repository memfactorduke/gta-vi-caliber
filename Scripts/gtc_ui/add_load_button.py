"""
add_load_button.py — add a styled "LOAD GAME" button to the kit pause menu, matching the
existing secondary buttons, inserted right after Resume, and re-space the column to fit 7
buttons inside the existing card (no card resize).

Uses the UE5.8 protected-WidgetTree bypass (load_object + new_object). The new button is
self-styled by CLONING an existing secondary button's WidgetStyle + its label's font/color,
so it looks identical without hand-authoring an FButtonStyle. It is marked is_variable so the
EventGraph can later bind its OnClicked (-> BPBlueprintFunctions.LoadPlayersProgress).

Self-discovering: finds the button container at runtime and handles both a CanvasPanel
(absolute offsets -> manual respace) and a VerticalBox (auto-stack -> just insert).

Run from the in-editor console:
  py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/add_load_button.py"

Idempotent: re-running reuses the existing GTC_Load_button and just re-styles/re-positions.
Sentinels: GTC_LOAD_START / _TREE / _CONTAINER / _CLONE / _NEW|_FOUND / _RESPACE / _DONE / _FAIL
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree"
BTN_NAME = "GTC_Load_button"
TXT_NAME = "GTC_Load_text"
LABEL = "LOAD GAME"


def first_textblock(widget):
    """Return the first TextBlock descendant of a content/panel widget, or None."""
    try:
        n = widget.get_children_count()
    except Exception:
        # UButton is a content widget: get_child_at(0)
        try:
            c = widget.get_child_at(0)
            return c if isinstance(c, unreal.TextBlock) else None
        except Exception:
            return None
    for i in range(n):
        c = widget.get_child_at(i)
        if isinstance(c, unreal.TextBlock):
            return c
    return None


def button_top(btn):
    """Best-effort Y position of a button in a CanvasPanel (via the slot getter)."""
    try:
        s = btn.get_editor_property("slot")
        return s.get_position().y
    except Exception:
        return 0.0


unreal.log("GTC_LOAD_START")
try:
    tree = unreal.load_object(None, OBJ)
    if tree is None:
        raise Exception("could not load WidgetTree")
    unreal.log("GTC_LOAD_TREE ok")

    # --- discover all buttons in the tree and their shared parent container ---
    buttons = []
    def walk(w):
        try:
            cnt = w.get_children_count()
        except Exception:
            return
        for i in range(cnt):
            c = w.get_child_at(i)
            if isinstance(c, unreal.Button):
                buttons.append(c)
            walk(c)

    root = unreal.load_object(None, OBJ + ".Pause_menu_canvas")
    if root is None:
        # fall back: WidgetTree root widget
        root = tree.get_editor_property("root_widget")
    walk(root)
    unreal.log("GTC_LOAD_BTNS found=%d" % len(buttons))
    if not buttons:
        raise Exception("no existing buttons found to clone/anchor")

    # exclude any prior GTC_Load_button from the clone-source pool
    src_pool = [b for b in buttons if b.get_name() != BTN_NAME]
    # prefer a non-Resume secondary button as the style donor
    donor = None
    for b in src_pool:
        if "esume" not in b.get_name():
            donor = b
            break
    if donor is None:
        donor = src_pool[0]
    container = donor.get_parent()
    unreal.log("GTC_LOAD_CONTAINER donor=%s parent=%s type=%s" %
               (donor.get_name(), container.get_name(), type(container).__name__))

    donor_text = first_textblock(donor)
    unreal.log("GTC_LOAD_CLONE donor_text=%s" % (donor_text.get_name() if donor_text else None))

    # --- find or create the Load button + its label ---
    # search ALL widgets in the tree (catches an orphan from a prior half-run, not just
    # buttons currently parented under the canvas)
    existing = None
    # 1) already parented under the canvas?
    for b in buttons:
        if b.get_name() == BTN_NAME:
            existing = b
            break
    # 2) an orphan subobject from a prior half-run? fetch it by object path so we reuse
    #    it instead of spawning yet another duplicate (no WidgetTree.get_all_widgets in 5.8)
    if existing is None:
        try:
            cand = unreal.load_object(None, OBJ + "." + BTN_NAME)
            if isinstance(cand, unreal.Button):
                existing = cand
                unreal.log("GTC_LOAD_ORPHAN_REUSE")
        except Exception:
            pass

    if existing is not None:
        load_btn = existing
        unreal.log("GTC_LOAD_FOUND reuse")
    else:
        load_btn = unreal.new_object(unreal.Button, tree, BTN_NAME)
        unreal.log("GTC_LOAD_NEW button")

    for _vn in ("is_variable", "b_is_variable", "bIsVariable"):
        try:
            load_btn.set_editor_property(_vn, True)
            unreal.log("GTC_LOAD_ISVAR ok via %s" % _vn)
            break
        except Exception as _ve:
            unreal.log("GTC_LOAD_ISVARWARN %s %s" % (_vn, repr(_ve)))

    # clone the donor's button style so it looks identical
    try:
        load_btn.set_editor_property("WidgetStyle", donor.get_editor_property("WidgetStyle"))
        unreal.log("GTC_LOAD_CLONE style ok")
    except Exception as se:
        unreal.log("GTC_LOAD_CLONE_STYLEWARN " + repr(se))

    # label child
    lbl = first_textblock(load_btn)
    if lbl is None:
        lbl = unreal.new_object(unreal.TextBlock, tree, TXT_NAME)
        load_btn.add_child(lbl)
    lbl.set_text(unreal.Text(LABEL))
    if donor_text is not None:
        for prop in ("Font", "ColorAndOpacity", "Justification"):
            try:
                lbl.set_editor_property(prop, donor_text.get_editor_property(prop))
            except Exception as pe:
                unreal.log("GTC_LOAD_LBLWARN %s %s" % (prop, repr(pe)))
    unreal.log("GTC_LOAD_LABEL ok")

    # --- place into the container ---
    if isinstance(container, unreal.CanvasPanel):
        # parent into canvas if not already there
        if load_btn.get_parent() != container:
            container.add_child_to_canvas(load_btn)
        # clone donor slot anchors/alignment via the slot GETTERS (offsets/anchors are not
        # get_editor_property-able on CanvasPanelSlot; size/position/anchors use methods)
        d_slot = donor.get_editor_property("slot")
        l_slot = load_btn.get_editor_property("slot")
        try:
            l_slot.set_anchors(d_slot.get_anchors())
        except Exception as ae:
            unreal.log("GTC_LOAD_ANCHORWARN " + repr(ae))
        try:
            l_slot.set_alignment(d_slot.get_alignment())
        except Exception as ae2:
            unreal.log("GTC_LOAD_ALIGNWARN " + repr(ae2))

        d_size = d_slot.get_size()
        W = d_size.x if d_size.x > 1.0 else 520.0
        H = d_size.y if d_size.y > 1.0 else 74.0
        dx = d_slot.get_position().x

        # build visual order: Resume first, then Load, then the remaining buttons by current Y
        ordered_existing = sorted(src_pool, key=button_top)
        resume = None
        for b in ordered_existing:
            if "esume" in b.get_name():
                resume = b
                break
        rest = [b for b in ordered_existing if b is not resume]
        order = ([resume] if resume else []) + [load_btn] + rest

        # geometry from existing column: keep first top + total span, re-divide for N buttons
        tops = [button_top(b) for b in ordered_existing]
        t0 = min(tops)
        span = (max(tops) + H) - t0
        n = len(order)
        pitch = (span - H) / (n - 1) if n > 1 else 0.0
        unreal.log("GTC_LOAD_RESPACE t0=%.1f span=%.1f H=%.1f W=%.1f n=%d pitch=%.1f" %
                   (t0, span, H, W, n, pitch))
        for i, b in enumerate(order):
            s = b.get_editor_property("slot")
            x = dx if b is load_btn else s.get_position().x
            s.set_position(unreal.Vector2D(x, t0 + i * pitch))
            s.set_size(unreal.Vector2D(W, H))
    elif isinstance(container, (unreal.VerticalBox, unreal.ScrollBox)):
        # auto-stacking: just insert after Resume (index 1)
        if load_btn.get_parent() != container:
            try:
                container.add_child(load_btn)
            except Exception:
                pass
        unreal.log("GTC_LOAD_RESPACE vertical-box auto-stack (insert handled by container)")
    else:
        unreal.log("GTC_LOAD_RESPACE unknown container type %s — added without respace"
                   % type(container).__name__)
        if load_btn.get_parent() is None:
            try:
                container.add_child(load_btn)
            except Exception:
                pass

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.log("GTC_LOAD_DONE name=%s var=%s" % (load_btn.get_name(),
               load_btn.get_editor_property("is_variable")))
except Exception as e:
    unreal.log_error("GTC_LOAD_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
