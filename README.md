# switchfish

Insaniquarium Deluxe as native Nintendo Switch homebrew, built from the source code the community
decompiled, with a widescreen tank that fills the whole Switch screen and controls made for a console.

**You need your own copy of Insaniquarium Deluxe** (for example from Steam). This repo and its builds contain
no game assets.

Status: plays on the Mac, boots in the Ryubing emulator, not yet run on a real Switch. `docs/STATUS.md` says
honestly how solid things are, `docs/ROADMAP.md` what comes next.

## What switchfish adds

### A widescreen tank you can play in

The game was made for 640x480. switchfish turns on the widescreen mode PopCap left in its own framework and
widens the tank itself to 854x480, so on a 16:9 screen the whole picture is aquarium: fish swim, food falls,
coins drop and aliens chase you into the new sides. Menus, dialogs and the HUD stay as they were.

- The extra sides of the six tank backdrops were drawn by an image model, with the glass walls and floor
  corners in the original's perspective. They hold no PopCap pixels: the game pastes the original middle
  back from your own files when it loads, pixel exact, and blends the seams.
- The main menu gets wider art the same way. Other screens that stay 4:3 sit on the water from the game's
  own help screen, with slow bubbles and a soft shadow, instead of black bars.
- **Widescreen** is a checkbox in Options (where Fullscreen used to be, which did nothing on a Switch).
  Turned off you get the original 4:3 game, every number as it was.
- The picture is scaled with a sharp-bilinear filter, so the pixel art stays crisp at 720p and 1080p.

Details: `docs/WIDESCREEN.md`.

### Controls made for the Switch

Designed from what the game actually does (`docs/CONTROLS.md`):

| | |
|---|---|
| Left stick | cursor, with a deadzone and a speed curve; it slows down over coins and buttons so you can stop on them |
| ZL / right stick | slow and precise / nudge |
| A or ZR | click, hold to keep feeding |
| B | right click (tap the glass) |
| D-pad | jump to the next button that way |
| X / Y | feed the hungriest fish / grab the money (Quick Aim in Options); hold to keep going, ZL + X/Y only aims |
| R (hold) | fast-forward a level at double speed (Fast-Forward in Options, off by default) |
| + / − | menu / pause |
| Touch | point and tap directly in handheld; ZL + tap is a right click |
| USB mouse | through the dock |
| Rumble | when an alien arrives or goes down, for every hit, a tap on the glass, a fish dying |

The cursor is the game's own, and Options > Controls explains the buttons on a Help page in the game's style. Home or sleep pauses and mutes the game like alt-tab does on a PC. The Switch
keyboard opens for player names.

### Small things

- Screens in the game's own dialog style when the game files are missing or when you start from the album.
- The title music waits until there is a picture.
- Loading on the Switch went from 38 s to a few: files are read whole, images decode on the idle cores, and
  after the first boot the decoded images come from `imagecache.bin` in the game folder (about 19 MB, made
  from your own files; delete it to rebuild it).
- Fixes: a highlighted button could stay lit after a click, holding the mouse to feed didn't work in the
  wide tank's sides.

## On the Switch

Copy the game's folders (`properties`, `images`, `sounds`, `music`, `data`, `fishsongs`, ...) and the NRO to:

```
sdmc:/switch/switchfish/
├── switchfish.nro
├── properties/resources.xml
└── images/ sounds/ music/ data/ fishsongs/ ...
```

Start it from hbmenu through title takeover (hold R while starting any game), not from the album: the album
runs homebrew with too little memory. If the game crashes it writes its last log lines and the crash
address to `sdmc:/switch/switchfish/crash.txt`; with a `debug.txt` in that folder it logs everything to
`log.txt` (slower, every line is a write to the SD card).

Copying the game files over USB works best through hekate's USB mass storage (the SD card shows up as a
normal disk); MTP tools on a Mac are slow and unreliable with hundreds of small files. After that only the
NRO changes, and `./sf.sh send <switch-ip>` puts a new build on the Switch over wifi in seconds: open hbmenu,
press Y, run the command. Arguments after the IP go to the game (`./sf.sh send 192.168.1.20 --level 3-2`),
and its log shows in the terminal until Ctrl-C.

## Building

Switch builds only need Docker; the toolchain (devkitA64, libnx, SDL2, mesa, libopenmpt) lives in an image.

```bash
./sf.sh setup          # once: Docker image, Homebrew packages, Ryubing
./sf.sh build switch   # build/switch/switchfish.nro (icon: the guppy from your own title screen)
./sf.sh build          # Mac build in build/host/
./sf.sh run            # Mac build with the game files in gameData/ (or --data <dir>)
./sf.sh emu            # the NRO in the Ryubing emulator
./sf.sh send <ip>      # over wifi to hbmenu's netloader (press Y): installs, starts, shows the log here
./sf.sh widen          # redraw the widescreen sides with Codex (needs Codex)
./sf.sh upstream       # clone or update the reference projects in upstream/
```

On the Mac a game controller drives the same cursor as on the Switch.

### Testing

- **Test plans** (`tests/plans/`, format in `port/TestPlan.h`) drive the game hands-free: clicks, keys,
  controller input and screenshots at fixed logic ticks. A run is silent, stays in the background and
  always ends by itself:
  `./build/host/switchfish --plan tests/plans/menus.txt --out /tmp/shots --timeout 45`
- **Quickplay**: `./sf.sh run --quickplay` lets N finish the current level the normal way, and
  `--level 3-2` starts a new adventure at that level. On the Switch, a `quickplay.txt` in the game folder
  (optionally with `3-2` in it) does the same. It changes your profile, so use a test profile.

### Layout

| Folder | What |
|---|---|
| `game/` | the game (WinFish decompilation, kyle-sylvestre's portable fork) |
| `framework/` | SexyAppFramework on SDL2 + GLES2, with `platform/switch` and `platform/default` |
| `port/` | our own code: entry point, notice screens, test plans, wide tank art, `romfs/` for the NRO |
| `scripts/widescreen/` | how the widescreen sides are made |
| `third_party/` | SDL-Mixer-X |
| `docker/` | the Switch build image |
| `docs/` | analysis, status, roadmap, controls, widescreen, where everything came from |
| `upstream/`, `tools/`, `gameData/` | reference projects, the emulator, your own game files (all gitignored) |

## Thanks

switchfish stands on a lot of other people's work:

- **Flying Bear Entertainment and PopCap Games** for the game.
- **[vindirect/winfish](https://github.com/vindirect/winfish)**, the decompilation that made a native port
  possible.
- **[kyle-sylvestre](https://github.com/kyle-sylvestre)** for the portable fork of
  [WinFish](https://github.com/kyle-sylvestre/WinFish) and the `insaniquarium` branch of
  [PvZ-Portable](https://github.com/kyle-sylvestre/PvZ-Portable) that this port is built on.
- **[wszqkzqk/PvZ-Portable](https://github.com/wszqkzqk/PvZ-Portable)** and
  **[headshot2017](https://github.com/headshot2017/re-plants-vs-zombies)** for bringing PopCap's
  framework to SDL2 and OpenGL ES.
- **[elgreams/OpenInsaniquarium](https://github.com/elgreams/OpenInsaniquarium)** for the reverse engineering
  notes on timing, randomness, file formats and the music.
- **[5N00Z3R/Insaniquarium-Decompilation](https://github.com/5N00Z3R/Insaniquarium-Decompilation)**, the most
  accurate decompilation, our reference for fixes.
- **[ChanseyIsTheBest/insaniquarium_nx](https://github.com/ChanseyIsTheBest/insaniquarium_nx)**, the first
  Switch port, which proved it could be done and showed how to handle a USB mouse and the input conflicts.
- **devkitPro, libnx and switchbrew** for the Switch toolchain, **Ryubing** for the emulator, **SDL**,
  **SDL-Mixer-X**, **libopenmpt** and the **Fredoka** font.

Where every vendored folder came from and at which commit: `docs/UPSTREAM.md`.

## Credits and license

switchfish is made by [AsmGecko](https://github.com/AsmGecko).

Our own code is MIT; the framework stays LGPL-3.0-or-later and the PopCap Games Framework License, the game
code is the WinFish decompilation's. `LICENSE` lists which license covers which folder.

This product includes portions of the PopCap Games Framework, © 2005-2009 PopCap Games, Inc. All rights
reserved. (http://popcapframework.sourceforge.net/)

Insaniquarium is a trademark of its owners. This project is not affiliated with or endorsed by PopCap Games or
Electronic Arts.
