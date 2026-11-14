# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""(Re)build every WBP_TD_* layout on top of the C++ widget classes (Source/TouchdownMars/UI/TDWidgets.h).

Run inside the editor through the MCP tool execute_python_code, with PIE stopped (poll is_pie_running):
    exec(open(r"<ProjectDir>/Scripts/build_greybox_ui.py", encoding="utf-8").read())
Widget names must match the C++ BindWidget names. Colors and font sizes are applied by C++ from TDTokens;
this script builds the hierarchy, layout and panel colors. Re-running deletes and rebuilds the widgets, so run
Scripts/setup_test_map.py afterwards (PC_TD must point at the new WBP_TD_HUD class).
"""

import unreal

UI = "/Game/TouchdownMars/UI"
WS = unreal.WidgetService
log = []

# UI tokens as linear colors (sRGB hex converted).
CORE_BACKGROUND = "(R=0.0044,G=0.0086,B=0.0194,A=0.92)"
CORE_PRIMARY = "(R=0.0080,G=0.0176,B=0.0409,A=1.0)"
SCRIM = "(R=0.0044,G=0.0086,B=0.0194,A=0.55)"


def make_wbp(name, parent_class):
    path = f"{UI}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, UI, unreal.WidgetBlueprint, factory)
    return path


def add(path, kind, name, parent, variable=True):
    if not WS.add_component(path, kind, name, parent, variable):
        log.append(f"FAILED add {kind} {name} -> {parent} in {path}")


def prop(path, widget, key, value):
    if not WS.set_property(path, widget, key, str(value)):
        log.append(f"FAILED set {widget}.{key}={value} in {path}")


def props(path, widget, pairs):
    for key, value in pairs:
        prop(path, widget, key, value)


def text(path, name, parent, wrap=False, bottom=8):
    add(path, "TextBlock", name, parent)
    if wrap:
        prop(path, name, "AutoWrapText", "True")
    if bottom:
        prop(path, name, "Padding Bottom", bottom)


def button(path, name, label, parent, right=16):
    add(path, "Button", name, parent)
    add(path, "TextBlock", label, name)
    if right:
        prop(path, name, "Padding Right", right)


def sized(path, name, parent, width=None, height=None):
    add(path, "SizeBox", name, parent, False)
    if width is not None:
        props(path, name, (("bOverride_WidthOverride", "true"), ("WidthOverride", width)))
    if height is not None:
        props(path, name, (("bOverride_HeightOverride", "true"), ("HeightOverride", height)))


def canvas(path, widget, ax0, ay0, ax1, ay1, alx, aly, x, y, w, h):
    props(path, widget, (("Anchor Min X", ax0), ("Anchor Min Y", ay0), ("Anchor Max X", ax1), ("Anchor Max Y", ay1),
                         ("Alignment X", alx), ("Alignment Y", aly), ("Position X", x), ("Position Y", y), ("Size X", w), ("Size Y", h)))


def overlay_panel(path, root, box, color=CORE_BACKGROUND):
    """Full-screen backdrop Border with a centred VerticalBox."""
    add(path, "Border", root, "", False)
    props(path, root, (("BrushColor", color), ("Padding", 48), ("HorizontalAlignment", "HAlign_Center"), ("VerticalAlignment", "VAlign_Center")))
    add(path, "VerticalBox", box, root, False)


def finish(path):
    bp = unreal.load_asset(path)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    log.append(f"{path.split('/')[-1]}: {len(WS.get_hierarchy(path))} widgets")


# Delete every existing widget first, parents before children (deleting a child that a loaded parent still uses
# crashed the editor on 13.11). Then build leaves first: a parent WBP must find its child WBPs.
ALL = ["WBP_TD_HUD", "WBP_TD_PowerPanel", "WBP_TD_Settings", "WBP_TD_SourcePanel", "WBP_TD_SunToday", "WBP_TD_BackToLander",
       "WBP_TD_MissionReport", "WBP_TD_MessageTrack", "WBP_TD_Capcom", "WBP_TD_EventCard", "WBP_TD_PhaseStrip",
       "WBP_TD_NextPhaseButton", "WBP_TD_SystemSlot", "WBP_TD_PowerCell"]
for name in ALL:
    if unreal.EditorAssetLibrary.does_asset_exist(f"{UI}/{name}"):
        unreal.EditorAssetLibrary.delete_asset(f"{UI}/{name}")


p = make_wbp("WBP_TD_PowerCell", unreal.TDPowerCellWidget)
add(p, "Border", "CellBorder", "")
sized(p, "CellSize", "CellBorder", 72, 72)
finish(p)

p = make_wbp("WBP_TD_SystemSlot", unreal.TDSystemSlotWidget)
add(p, "VerticalBox", "SlotRoot", "", False)
add(p, "Border", "HintBar", "SlotRoot")
prop(p, "HintBar", "Padding Bottom", 6)
sized(p, "HintSize", "HintBar", None, 10)
text(p, "MarkText", "SlotRoot", bottom=4)
add(p, "Button", "SlotButton", "SlotRoot")
add(p, "VerticalBox", "SlotContent", "SlotButton", False)
prop(p, "SlotContent", "Padding", 16)
text(p, "NameText", "SlotContent")
text(p, "StateText", "SlotContent")
add(p, "HorizontalBox", "SegmentBox", "SlotContent")
prop(p, "SegmentBox", "Padding Bottom", 12)
add(p, "HorizontalBox", "CellBox", "SlotContent")
add(p, "Button", "RemoveButton", "SlotRoot")
props(p, "RemoveButton", (("Padding Top", 8), ("Horizontal Alignment", "Center")))
add(p, "TextBlock", "RemoveLabel", "RemoveButton", False)
prop(p, "RemoveLabel", "Text", "-")
finish(p)

p = make_wbp("WBP_TD_NextPhaseButton", unreal.TDNextPhaseButtonWidget)
add(p, "SizeBox", "ButtonSize", "", False)
props(p, "ButtonSize", (("bOverride_MinDesiredWidth", "true"), ("MinDesiredWidth", 240), ("bOverride_MinDesiredHeight", "true"), ("MinDesiredHeight", 72)))
add(p, "Button", "Button", "ButtonSize")
add(p, "TextBlock", "Label", "Button")
finish(p)

p = make_wbp("WBP_TD_PhaseStrip", unreal.TDPhaseStripWidget)
add(p, "HorizontalBox", "StripRoot", "", False)
add(p, "HorizontalBox", "DotBox", "StripRoot")
props(p, "DotBox", (("Vertical Alignment", "Center"), ("Padding Right", 16)))
add(p, "VerticalBox", "PhaseTextBox", "StripRoot", False)
text(p, "PhaseNameText", "PhaseTextBox", bottom=0)
text(p, "SolText", "PhaseTextBox", bottom=0)
finish(p)

p = make_wbp("WBP_TD_EventCard", unreal.TDEventCardWidget)
add(p, "Border", "CardRoot", "")
props(p, "CardRoot", (("BrushColor", CORE_PRIMARY), ("Padding", 24)))
add(p, "Overlay", "CardSides", "CardRoot", False)
add(p, "VerticalBox", "FrontBox", "CardSides")
text(p, "QuestionText", "FrontBox", wrap=True, bottom=16)
add(p, "HorizontalBox", "ChoiceRow", "FrontBox", False)
prop(p, "ChoiceRow", "Padding Bottom", 16)
button(p, "ChoiceAButton", "ChoiceALabel", "ChoiceRow")
button(p, "ChoiceBButton", "ChoiceBLabel", "ChoiceRow", right=0)
text(p, "PriorityText", "FrontBox", wrap=True, bottom=0)
add(p, "VerticalBox", "BackBox", "CardSides")
text(p, "BackTitleText", "BackBox")
text(p, "RevealText", "BackBox", wrap=True, bottom=12)
text(p, "FactsText", "BackBox", wrap=True)
text(p, "MeasureText", "BackBox", wrap=True)
text(p, "SourceText", "BackBox", wrap=True, bottom=12)
button(p, "CloseButton", "CloseLabel", "BackBox", right=0)
prop(p, "CloseButton", "Horizontal Alignment", "Right")
finish(p)

p = make_wbp("WBP_TD_Capcom", unreal.TDCapcomWidget)
add(p, "HorizontalBox", "CapcomRow", "", False)
add(p, "Border", "Portrait", "CapcomRow")
props(p, "Portrait", (("Padding", 12), ("Padding Right", 16), ("Vertical Alignment", "Top")))
text(p, "PortraitText", "Portrait", bottom=0)
text(p, "LineText", "CapcomRow", wrap=True, bottom=0)
prop(p, "LineText", "Size Rule", "Fill")
finish(p)

p = make_wbp("WBP_TD_MessageTrack", unreal.TDMessageTrackWidget)
add(p, "VerticalBox", "TrackRoot", "", False)
sized(p, "TrackSize", "TrackRoot", None, 16)
add(p, "ProgressBar", "TrackBar", "TrackSize")
text(p, "StampText", "TrackRoot", bottom=0)
prop(p, "StampText", "Padding Top", 4)
finish(p)

p = make_wbp("WBP_TD_MissionReport", unreal.TDMissionReportWidget)
overlay_panel(p, "ReportBg", "ReportBox")
for name in ("TitleText", "ResultText", "CrewText", "BaseText", "EmergencyText", "BonusText", "PatchText", "SeedText", "CompareText", "MessageText"):
    text(p, name, "ReportBox", wrap=True, bottom=12)
add(p, "HorizontalBox", "ReportButtons", "ReportBox", False)
prop(p, "ReportButtons", "Padding Top", 16)
button(p, "FlyAgainButton", "FlyAgainLabel", "ReportButtons")
button(p, "NewMissionButton", "NewMissionLabel", "ReportButtons", right=0)
finish(p)

p = make_wbp("WBP_TD_BackToLander", unreal.TDBackToLanderWidget)
overlay_panel(p, "LanderBg", "LanderBox", SCRIM)
text(p, "LineText", "LanderBox", wrap=True, bottom=24)
add(p, "HorizontalBox", "LanderButtons", "LanderBox", False)
button(p, "RetryButton", "RetryLabel", "LanderButtons")
button(p, "NewButton", "NewLabel", "LanderButtons", right=0)
finish(p)

p = make_wbp("WBP_TD_SunToday", unreal.TDSunTodayWidget)
add(p, "VerticalBox", "SunRoot", "", False)
button(p, "BadgeButton", "BadgeText", "SunRoot", right=0)
props(p, "BadgeButton", (("Horizontal Alignment", "Left"), ("Padding Bottom", 8)))
add(p, "Border", "SunPanel", "SunRoot")
props(p, "SunPanel", (("BrushColor", CORE_PRIMARY), ("Padding", 24), ("Padding Bottom", 12)))
add(p, "VerticalBox", "SunBox", "SunPanel", False)
for name in ("SunTitleText", "StatusText", "SummaryText", "RowsText", "DisclaimerText"):
    text(p, name, "SunBox", wrap=True, bottom=10)
button(p, "SunCloseButton", "SunCloseLabel", "SunBox", right=0)
prop(p, "SunCloseButton", "Horizontal Alignment", "Right")
finish(p)

p = make_wbp("WBP_TD_SourcePanel", unreal.TDSourcePanelWidget)
overlay_panel(p, "SourcesBg", "SourcesBox")
text(p, "SourcesTitleText", "SourcesBox", bottom=16)
sized(p, "ListSize", "SourcesBox", 1500, 700)
add(p, "ScrollBox", "ListScroll", "ListSize", False)
text(p, "ListText", "ListScroll", wrap=True, bottom=0)
button(p, "SourcesCloseButton", "SourcesCloseLabel", "SourcesBox", right=0)
props(p, "SourcesCloseButton", (("Horizontal Alignment", "Right"), ("Padding Top", 16)))
finish(p)

p = make_wbp("WBP_TD_Settings", unreal.TDSettingsWidget)
overlay_panel(p, "SettingsBg", "SettingsBox")
text(p, "SettingsTitleText", "SettingsBox", bottom=24)
for stem in ("Resume", "Restart", "NewMission", "Band", "Language", "Sound", "Motion", "Screen", "Quit"):
    button(p, f"{stem}Button", f"{stem}Label", "SettingsBox", right=0)
    prop(p, f"{stem}Button", "Padding Bottom", 12)
finish(p)

p = make_wbp("WBP_TD_PowerPanel", unreal.TDPowerPanelWidget)
add(p, "Border", "PanelBorder", "")
prop(p, "PanelBorder", "Padding", 24)
add(p, "HorizontalBox", "PanelRow", "PanelBorder", False)
add(p, "HorizontalBox", "CellsBox", "PanelRow")
props(p, "CellsBox", (("Vertical Alignment", "Center"), ("Padding Right", 40)))
for slot_name in ("SlotAir", "SlotGreenhouse", "SlotShelter"):
    add(p, "WBP_TD_SystemSlot", slot_name, "PanelRow")
    props(p, slot_name, (("Padding Right", 24), ("Vertical Alignment", "Bottom")))
add(p, "WBP_TD_NextPhaseButton", "NextPhase", "PanelRow")
props(p, "NextPhase", (("Vertical Alignment", "Center"), ("Padding Left", 24)))
finish(p)
panel_class = unreal.load_asset(p).generated_class()
cell_class = unreal.load_asset(f"{UI}/WBP_TD_PowerCell").generated_class()
unreal.get_default_object(panel_class).set_editor_property("power_cell_class", cell_class)
unreal.EditorAssetLibrary.save_asset(p, only_if_is_dirty=False)

# HUD (1920x1080 reference; bottom third is the power panel).
p = make_wbp("WBP_TD_HUD", unreal.TDHudWidget)
add(p, "CanvasPanel", "HudCanvas", "", False)

add(p, "Border", "StripPanel", "HudCanvas", False)
props(p, "StripPanel", (("BrushColor", CORE_BACKGROUND), ("Padding", 16)))
canvas(p, "StripPanel", 0, 0, 0, 0, 0, 0, 24, 24, 760, 112)
add(p, "WBP_TD_PhaseStrip", "PhaseStrip", "StripPanel")

add(p, "Border", "CapcomPanel", "HudCanvas", False)
props(p, "CapcomPanel", (("BrushColor", CORE_BACKGROUND), ("Padding", 16)))
canvas(p, "CapcomPanel", 1, 0, 1, 0, 1, 0, -24, 24, 780, 236)
add(p, "VerticalBox", "CapcomStack", "CapcomPanel", False)
add(p, "WBP_TD_Capcom", "Capcom", "CapcomStack")
prop(p, "Capcom", "Padding Bottom", 12)
add(p, "WBP_TD_MessageTrack", "MessageTrack", "CapcomStack")
prop(p, "MessageTrack", "Padding Bottom", 12)
button(p, "CallHomeButton", "CallHomeLabel", "CapcomStack", right=0)
prop(p, "CallHomeButton", "Horizontal Alignment", "Left")

add(p, "Border", "DebugPanel", "HudCanvas", False)
props(p, "DebugPanel", (("BrushColor", CORE_BACKGROUND), ("Padding", 12)))
canvas(p, "DebugPanel", 1, 0, 1, 0, 1, 0, -24, 276, 780, 140)
text(p, "DebugText", "DebugPanel", wrap=True, bottom=0)

add(p, "Border", "ResultPanel", "HudCanvas", False)
props(p, "ResultPanel", (("BrushColor", CORE_BACKGROUND), ("Padding", 12)))
canvas(p, "ResultPanel", 0.5, 1, 0.5, 1, 0.5, 1, 0, -436, 1100, 64)
text(p, "ResultText", "ResultPanel", wrap=True, bottom=0)

add(p, "Border", "DosePanel", "HudCanvas", False)
props(p, "DosePanel", (("BrushColor", CORE_BACKGROUND), ("Padding", 8)))
canvas(p, "DosePanel", 0, 0, 0, 0, 0, 0, 24, 216, 420, 52)
text(p, "DoseText", "DosePanel", bottom=0)

add(p, "WBP_TD_SunToday", "SunToday", "HudCanvas")
canvas(p, "SunToday", 0, 0, 0, 0, 0, 0, 24, 148, 640, 600)

add(p, "HorizontalBox", "CornerRow", "HudCanvas", False)
canvas(p, "CornerRow", 1, 1, 1, 1, 1, 1, -24, -356, 980, 64)
for stem in ("Settings", "Language", "Mute", "Sources"):
    button(p, f"{stem}Button", f"{stem}Label", "CornerRow", right=12)

add(p, "WBP_TD_PowerPanel", "PowerPanel", "HudCanvas")
canvas(p, "PowerPanel", 0, 1, 1, 1, 0, 1, 0, 0, 0, 340)

add(p, "WBP_TD_EventCard", "EventCard", "HudCanvas")
canvas(p, "EventCard", 0.5, 0.36, 0.5, 0.36, 0.5, 0.5, 0, 0, 1180, 660)

for name in ("MissionReport", "BackToLander", "SourcePanel", "SettingsPanel"):
    kind = {"MissionReport": "WBP_TD_MissionReport", "BackToLander": "WBP_TD_BackToLander",
            "SourcePanel": "WBP_TD_SourcePanel", "SettingsPanel": "WBP_TD_Settings"}[name]
    add(p, kind, name, "HudCanvas")
    canvas(p, name, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0)
finish(p)

print("\n".join(log))
