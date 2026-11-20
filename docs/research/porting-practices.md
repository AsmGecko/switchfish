# How established Switch ports are built (researched 2026-11-20)

## Build and packaging
- CMake with `$DEVKITPRO/cmake/Switch.cmake` (or `aarch64-none-elf-cmake`), then
  `nx_generate_nacp(...)` + `nx_create_nro(<target> NACP ... ICON icon.jpg [ROMFS dir])`, target `<name>_nro`.
  devilutionX, Julius, OpenJazz and SoH all do this. Old hand-written nacptool/elf2nro rules (OpenRCT2,
  NXEngine) are not worth copying.
- CI: `devkitpro/devkita64` container, pin a dated tag. Keep the `.elf` next to the `.nro` for crash addresses.
- All Switch code in one `platform/switch/` folder.
- Game data the user owns goes on SD (`sdmc:/switch/<app>/`), romfs only for our own extras.
- Ship an NRO; forwarders are the user's business, so always find data from a fixed SD path.

## Mouse-driven games (the relevant ones)
| Port | Cursor | Touch | Text |
|---|---|---|---|
| devilutionX | right stick, time-based accumulator with sub-pixel remainder, radial deadzone (`Source/controls/plrctrls.cpp`) | SDL touch-to-mouse | blocking swkbd, injected as SDL_TEXTINPUT |
| Julius/Augustus | left stick + D-pad, L faster, R slower, radial deadzone 2000 (`src/input/joystick.c`) | 3 modes: original, touchpad (default), direct. Disables SDL touch-to-mouse and ignores `SDL_TOUCH_MOUSEID` | swkbd on button |
| fheroes2 (640x480!) | left stick, `|axis|^1.03 * dt * speed`, R doubles | tap = left, second finger = right, long press 850 ms, own coordinate mapping through the viewport | own on-screen keyboard |
| ScummVM | left stick, R slows | direct (default) or touchpad, second finger right click | swkbd on Minus |
| OpenRCT2 (rsn8887) | left stick, tick-based, pushes synthetic SDL_MOUSEMOTION | 3 touch modes | swkbd on Plus |
| insaniquarium_nx | left stick + gyro, B right click | direct in handheld | n/a |

Common pattern to adopt:
1. Direct touch (pointer jumps to finger, tap = left click). Disable SDL touch-to-mouse
   (`SDL_HINT_TOUCH_MOUSE_EVENTS=0`) and map fingers through our own presentation rect.
2. Analog cursor: radial deadzone, time-based speed, sub-pixel accumulator, fast/slow modifier.
3. A = left click (repeat allowed, the game wants rapid clicking), B = right click, Plus = pause/menu.
4. SDL GameController reports Xbox positions: south button is Nintendo's B.
5. Text entry (player name) via `SDL_StartTextInput()` (inline swkbd) or blocking `swkbdShow`.

## Video
- Fixed 1280x720 surface and let the hardware scaler handle docked 1080p (PvZ-Portable, insaniquarium_nx), or
  resize on dock change (devilutionX `docking.cpp`, SDL2 does it when the window is RESIZABLE).
- 640x480 to 720p is 1.5x: use bilinear or a sharp-bilinear shader, not nearest.

## Lifecycle (SoH `SwitchImpl.cpp` is the best example)
- `appletMainLoop()` / `SDL_QUIT`, pause on focus loss, restart audio after resume.
- Detect applet mode (album) and tell the user to use title takeover.
- `socketInitializeDefault(); nxlinkStdio();` in debug builds.
