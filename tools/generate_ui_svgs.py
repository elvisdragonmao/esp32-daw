#!/usr/bin/env python3
from pathlib import Path
from xml.sax.saxutils import escape


OUT_DIR = Path("poster/ui-svg")

SCREEN_W = 160
SCREEN_H = 80
SCREEN_VIEW_Y = 24

VIEW_X = 0
VIEW_Y = 24
VIEW_W = 160
VIEW_H = 80

MENU_W = 44

GRID_X = VIEW_X + 47
GRID_Y = VIEW_Y + 3

CELL_W = 7
CELL_H = 9
ROW_H = 13

STATUS_Y = VIEW_Y + 56
STATUS_H = 24

TRACK_COUNT = 4
STEP_COUNT = 16

def screen_color(r, g, b):
    return f"#{r:02x}{g:02x}{b:02x}"


COL_BG = "#000000"
COL_PANEL = screen_color(8, 35, 45)
COL_PANEL_DARK = screen_color(4, 20, 28)
COL_TEXT = screen_color(220, 245, 255)
COL_MUTED_TEXT = screen_color(120, 140, 145)
COL_SELECT = screen_color(255, 150, 0)
COL_GRID_EMPTY = screen_color(25, 90, 115)
COL_GRID_BORDER = screen_color(70, 150, 170)
COL_NOTE = screen_color(80, 210, 240)
COL_MUTED_NOTE = screen_color(70, 80, 85)
COL_PLAYHEAD = "#ffffff"

OSC_NAMES = ["Sin", "Tri", "Sqr", "Saw"]
NOTE_NAMES = ["C", "D", "E", "F", "G", "A", "B", "C+"]


def demo_tracks():
    tracks = [
        {"mute": False, "volume": 100, "osc": 0, "steps": [-1] * STEP_COUNT},
        {"mute": False, "volume": 100, "osc": 1, "steps": [-1] * STEP_COUNT},
        {"mute": False, "volume": 100, "osc": 2, "steps": [-1] * STEP_COUNT},
        {"mute": False, "volume": 100, "osc": 3, "steps": [-1] * STEP_COUNT},
    ]

    tracks[0]["steps"][0] = 0
    tracks[0]["steps"][4] = 2
    tracks[0]["steps"][8] = 4
    tracks[0]["steps"][12] = 7

    tracks[1]["steps"][2] = 4
    tracks[1]["steps"][6] = 5
    tracks[1]["steps"][10] = 4
    tracks[1]["steps"][14] = 2

    tracks[2]["steps"][0] = 0
    tracks[2]["steps"][8] = 0

    tracks[3]["steps"][3] = 7
    tracks[3]["steps"][7] = 6
    tracks[3]["steps"][11] = 5
    tracks[3]["steps"][15] = 4
    return tracks


class Svg:
    def __init__(self, title):
        self.title = title
        self.items = []

    def rect(self, x, y, w, h, fill="none", stroke=None, sw=1):
        if stroke:
            if fill != "none":
                self.rect(x, y, w, h, fill)
            self.outline_rect(x, y, w, h, stroke, sw)
            return

        attrs = [
            f'x="{x}"',
            f'y="{y}"',
            f'width="{w}"',
            f'height="{h}"',
            f'fill="{fill}"',
        ]
        self.items.append(f'<rect {" ".join(attrs)}/>')

    def outline_rect(self, x, y, w, h, color, sw=1):
        # Adafruit_GFX drawRect draws the border inside the target pixel box.
        # Filled strips avoid SVG's centered stroke clipping at the right edge.
        if w <= 0 or h <= 0:
            return
        sw = max(1, min(sw, w, h))
        self.rect(x, y, w, sw, color)
        self.rect(x, y + h - sw, w, sw, color)
        if h > sw * 2:
            self.rect(x, y + sw, sw, h - sw * 2, color)
            self.rect(x + w - sw, y + sw, sw, h - sw * 2, color)

    def text(self, x, y, value, fill=COL_TEXT, size=7):
        self.items.append(
            f'<text x="{x}" y="{y}" fill="{fill}" font-size="{size}" '
            f'font-family="Courier New, Menlo, monospace" '
            f'dominant-baseline="text-before-edge">{escape(str(value))}</text>'
        )

    def write(self, path):
        path.write_text(
            "\n".join([
                f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 {SCREEN_VIEW_Y} {SCREEN_W} {SCREEN_H}" '
                f'width="{SCREEN_W}" height="{SCREEN_H}" shape-rendering="crispEdges" '
                f'text-rendering="geometricPrecision" overflow="hidden">',
                f'<title>{escape(self.title)}</title>',
                '<desc>Vector recreation of the ESP32 WROVER mini DAW TFT UI.</desc>',
                '<style>text { letter-spacing: 0; }</style>',
                *self.items,
                '</svg>',
                "",
            ]),
            encoding="utf-8",
        )


def draw_menu_item(svg, row, text, selected, muted=False):
    y = VIEW_Y + row * 10
    bg = COL_SELECT if selected else COL_PANEL
    fg = COL_MUTED_TEXT if muted else COL_TEXT
    svg.rect(0, y, MENU_W, 10, bg)
    svg.text(2, y + 1, text, fg)


def draw_left_panel(svg, state):
    svg.rect(VIEW_X, VIEW_Y, MENU_W, VIEW_H, COL_PANEL)

    mode = state["mode"]
    tracks = state["tracks"]

    if mode == "main":
        for t in range(TRACK_COUNT):
            text = f"{t + 1} {OSC_NAMES[tracks[t]['osc']]}"
            draw_menu_item(svg, t, text, state["main_index"] == t, tracks[t]["mute"])

        draw_menu_item(svg, 4, "Pause" if state["playing"] else "Play", state["main_index"] == 4)
        draw_menu_item(svg, 5, "Vol", state["main_index"] == 5)
        draw_menu_item(svg, 6, "BPM", state["main_index"] == 6)

    elif mode == "track":
        selected_track = state["selected_track"]
        draw_menu_item(svg, 0, "Unmut" if tracks[selected_track]["mute"] else "Mute", state["track_menu_index"] == 0)
        draw_menu_item(svg, 1, "Record", state["track_menu_index"] == 1)
        draw_menu_item(svg, 2, "Vol", state["track_menu_index"] == 2)
        draw_menu_item(svg, 3, "OSC", state["track_menu_index"] == 3)
        draw_menu_item(svg, 6, f"T{selected_track + 1}", False)

    elif mode == "master_vol":
        draw_menu_item(svg, 0, "M Vol", True)
        draw_menu_item(svg, 2, "Up +", False)
        draw_menu_item(svg, 3, "Dn -", False)
        draw_menu_item(svg, 6, "L Back", False)

    elif mode == "track_vol":
        selected_track = state["selected_track"]
        draw_menu_item(svg, 0, f"T{selected_track + 1}Vol", True)
        draw_menu_item(svg, 2, "Up +", False)
        draw_menu_item(svg, 3, "Dn -", False)
        draw_menu_item(svg, 6, "L Back", False)

    elif mode == "bpm":
        draw_menu_item(svg, 0, "BPM", True)
        draw_menu_item(svg, 2, "Up +", False)
        draw_menu_item(svg, 3, "Dn -", False)
        draw_menu_item(svg, 6, "L Back", False)

    elif mode == "osc":
        for i, name in enumerate(OSC_NAMES):
            draw_menu_item(svg, i, name, state["osc_menu_index"] == i)
        draw_menu_item(svg, 6, f"T{state['selected_track'] + 1}OSC", False)

    elif mode == "rec_armed":
        draw_menu_item(svg, 0, "ARM", True)
        draw_menu_item(svg, 1, f"T{state['record_track'] + 1}", False)
        draw_menu_item(svg, 3, "Wait", False)

    elif mode == "recording":
        draw_menu_item(svg, 0, "REC", True)
        draw_menu_item(svg, 1, f"T{state['record_track'] + 1}", False)
        draw_menu_item(svg, 3, f"{state['record_count'] + 1:02d}/16", False)


def draw_grid_cell(svg, state, t, s):
    tracks = state["tracks"]
    x = GRID_X + s * CELL_W
    y = GRID_Y + t * ROW_H

    has_note = tracks[t]["steps"][s] >= 0
    muted = tracks[t]["mute"]

    if has_note and muted:
        fill = COL_MUTED_NOTE
    elif has_note:
        fill = COL_NOTE
    else:
        fill = COL_GRID_EMPTY

    svg.rect(x, y, CELL_W - 1, CELL_H, fill)
    svg.rect(x, y, CELL_W - 1, CELL_H, "none", COL_GRID_BORDER)

    if state["playing"] and s == state["current_step"]:
        svg.rect(x - 1, y - 1, CELL_W + 1, CELL_H + 2, "none", COL_PLAYHEAD)


def draw_selected_track_outline(svg, state):
    mode = state["mode"]
    if mode not in {"track", "track_vol", "osc", "rec_armed", "recording"}:
        return

    t = state["record_track"] if mode in {"rec_armed", "recording"} else state["selected_track"]
    y = GRID_Y + t * ROW_H
    svg.rect(GRID_X - 2, y - 2, CELL_W * STEP_COUNT + 3, CELL_H + 4, "none", COL_SELECT)


def draw_grid(svg, state):
    svg.rect(GRID_X - 2, VIEW_Y, VIEW_W - GRID_X + 2, STATUS_Y - VIEW_Y - 1, COL_BG)
    for t in range(TRACK_COUNT):
        for s in range(STEP_COUNT):
            draw_grid_cell(svg, state, t, s)
    draw_selected_track_outline(svg, state)


def draw_bar(svg, x, y, w, h, value, max_value):
    value = max(0, min(value, max_value))
    fill_w = (w * value) // max_value
    svg.rect(x, y, w, h, "none", COL_GRID_BORDER)
    svg.rect(x + 1, y + 1, w - 2, h - 2, COL_GRID_EMPTY)
    if fill_w > 2:
        svg.rect(x + 1, y + 1, fill_w - 2, h - 2, COL_SELECT)


def draw_status(svg, state):
    svg.rect(GRID_X - 2, STATUS_Y, VIEW_W - GRID_X + 2, STATUS_H, COL_PANEL_DARK)

    mode = state["mode"]

    if mode == "master_vol":
        svg.text(GRID_X, STATUS_Y + 2, "Master Volume")
        draw_bar(svg, GRID_X, STATUS_Y + 13, 100, 8, state["master_volume"], 127)
        svg.text(GRID_X + 104, STATUS_Y + 13, f"{state['master_volume']:03d}")
        return

    if mode == "track_vol":
        selected_track = state["selected_track"]
        svg.text(GRID_X, STATUS_Y + 2, f"T{selected_track + 1} Volume")
        draw_bar(svg, GRID_X, STATUS_Y + 13, 100, 8, state["tracks"][selected_track]["volume"], 127)
        svg.text(GRID_X + 104, STATUS_Y + 13, f"{state['tracks'][selected_track]['volume']:03d}")
        return

    if mode == "bpm":
        svg.text(GRID_X, STATUS_Y + 2, "BPM")
        svg.text(GRID_X, STATUS_Y + 13, str(state["bpm"]), COL_SELECT)
        return

    if mode in {"recording", "rec_armed"}:
        note = state["current_rec_note"]
        if mode == "rec_armed":
            svg.text(GRID_X, STATUS_Y + 2, "ARM: wait loop")
        else:
            svg.text(GRID_X, STATUS_Y + 2, f"REC T{state['record_track'] + 1} {state['record_count'] + 1:02d}/16")

        if note >= 0:
            svg.text(GRID_X, STATUS_Y + 13, f"Note: {NOTE_NAMES[note]}")
        else:
            svg.text(GRID_X, STATUS_Y + 13, "Note: --")
        return

    svg.text(GRID_X, STATUS_Y + 2, f"BPM{state['bpm']:03d} V{state['master_volume']:03d} S{state['current_step'] + 1:02d}")
    prefix = "PLAY" if state["playing"] else "STOP"
    if mode == "main":
        label = "MAIN"
    elif mode == "track":
        label = f"T{state['selected_track'] + 1} MENU"
    elif mode == "osc":
        label = "OSC"
    else:
        label = ""
    svg.text(GRID_X, STATUS_Y + 13, f"{prefix}  {label}".rstrip())


def draw_screen(title, state):
    svg = Svg(title)
    svg.rect(0, SCREEN_VIEW_Y, SCREEN_W, SCREEN_H, COL_BG)
    draw_left_panel(svg, state)
    draw_grid(svg, state)
    draw_status(svg, state)
    svg.rect(0, SCREEN_VIEW_Y, SCREEN_W, SCREEN_H, "none", "#222222")
    return svg


def base_state():
    return {
        "tracks": demo_tracks(),
        "playing": False,
        "current_step": 0,
        "bpm": 120,
        "master_volume": 100,
        "main_index": 0,
        "selected_track": 0,
        "track_menu_index": 0,
        "osc_menu_index": 0,
        "record_track": 0,
        "record_count": 0,
        "current_rec_note": -1,
        "mode": "main",
    }


def make_states():
    states = []

    s = base_state()
    s.update({"mode": "main", "playing": False, "main_index": 0, "current_step": 0})
    states.append(("01-main-stopped.svg", "Main Stopped", s))

    s = base_state()
    s.update({"mode": "main", "playing": True, "main_index": 4, "current_step": 4})
    states.append(("02-main-playing.svg", "Main Playing", s))

    s = base_state()
    s.update({"mode": "track", "selected_track": 0, "track_menu_index": 1})
    states.append(("03-track-menu.svg", "Track Menu", s))

    s = base_state()
    s.update({"mode": "master_vol", "master_volume": 100})
    states.append(("04-master-volume.svg", "Master Volume", s))

    s = base_state()
    s["tracks"][1]["volume"] = 85
    s.update({"mode": "track_vol", "selected_track": 1})
    states.append(("05-track-volume.svg", "Track Volume", s))

    s = base_state()
    s.update({"mode": "bpm", "bpm": 120})
    states.append(("06-bpm.svg", "BPM", s))

    s = base_state()
    s.update({"mode": "osc", "selected_track": 2, "osc_menu_index": 2})
    states.append(("07-osc-menu.svg", "OSC Menu", s))

    s = base_state()
    s.update({"mode": "rec_armed", "playing": True, "record_track": 0, "selected_track": 0, "current_step": 12})
    states.append(("08-record-armed.svg", "Record Armed", s))

    s = base_state()
    s.update({
        "mode": "recording",
        "playing": True,
        "record_track": 0,
        "selected_track": 0,
        "record_count": 7,
        "current_rec_note": 4,
        "current_step": 8,
    })
    states.append(("09-recording.svg", "Recording", s))

    return states


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    manifest = []

    for filename, title, state in make_states():
        path = OUT_DIR / filename
        draw_screen(title, state).write(path)
        manifest.append(f"- `{filename}`: {title}")

    (OUT_DIR / "README.md").write_text(
        "\n".join([
            "# UI SVG Exports",
            "",
            "Poster-ready SVG recreations of the current TFT UI states.",
            "",
            "Generated by `tools/generate_ui_svgs.py`.",
            "",
            'Each SVG uses `viewBox="0 24 160 80"` to match the TFT visible area.',
            "Colors represent the intended firmware UI colors without red/blue compensation.",
            "",
            *manifest,
            "",
        ]),
        encoding="utf-8",
    )

    print(f"Wrote {len(manifest)} SVG files to {OUT_DIR}")


if __name__ == "__main__":
    main()
