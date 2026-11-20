# switchfish

Insaniquarium Deluxe on the Nintendo Switch, as a native homebrew NRO. Built from the WinFish decompilation on
an SDL2/GLES2 port of PopCap's SexyAppFramework. Goal: a stable port that eventually plays widescreen across
the whole Switch screen.

**You need your own copy of Insaniquarium Deluxe** (for example from Steam). This repo and its builds contain
no game assets.

Status: the NRO and a macOS build compile; nothing has run on hardware yet. See `docs/ROADMAP.md`.

## Daily use

```bash
./sf.sh setup          # once: Docker image with the Switch toolchain
./sf.sh build switch   # build/switch/switchfish.nro
./sf.sh build          # host build in build/host/
./sf.sh run            # host build with the game files in gameData/ (or --data <dir>)
./sf.sh emu            # NRO in the Ryubing emulator (installed in tools/ on first use)
./sf.sh widen          # widescreen tank pack from your backdrops via Codex (stays out of the repo)
./sf.sh send <ip>      # send the NRO to hbmenu's netloader (press Y on the Switch)
./sf.sh upstream       # clone/update the reference repos in upstream/
```

Host builds need `brew install cmake ninja sdl2 sdl2_ttf libopenmpt jpeg-turbo libpng libvorbis mpg123`.
Switch builds only need Docker.

## Test plans

`tests/plans/*.txt` drive the game hands-free: clicks, keys and screenshots at fixed logic ticks, in the
game's 640x480 design coordinates (see `port/TestPlan.h`). A run always quits by itself:

```bash
./build/host/switchfish --plan tests/plans/widetank.txt --out /tmp/shots --timeout 45
```

## On the Switch

Copy the game's folders (`properties`, `images`, `sounds`, `music`, `data`, `fishsongs`, ...) and the NRO to:

```
sdmc:/switch/switchfish/
├── switchfish.nro
├── properties/resources.xml
└── images/ sounds/ music/ data/ fishsongs/ ...
```

Start it from hbmenu through title takeover (hold R while starting any game), not from the album: the album
runs homebrew with too little memory. A log goes to `sdmc:/switch/switchfish/log.txt`.

## Layout

| Folder | What |
|---|---|
| `game/` | the game (WinFish decompilation, kyle-sylvestre's portable fork) |
| `port/` | our own code: entry point, notice screens, `romfs/` (fonts) that goes inside the NRO |
| `framework/` | SexyAppFramework on SDL2 + GLES2, with `platform/switch` and `platform/default` |
| `third_party/` | SDL-Mixer-X |
| `docker/` | Switch build image (devkitA64, libnx, SDL2, mesa, libopenmpt) |
| `docs/` | `ANALYSIS.md` (approach and decisions), `ROADMAP.md`, `UPSTREAM.md`, `research/` |
| `upstream/` | reference repos, not part of the build (gitignored) |
| `tools/` | Ryubing emulator in portable mode, its SD card is `tools/ryubing/portable/sdcard` (gitignored) |
| `gameData/` | your own Insaniquarium files; `run` and `emu` use them (gitignored) |

## Credits and licenses

Game code from [vindirect/winfish](https://github.com/vindirect/winfish) and
[kyle-sylvestre/WinFish](https://github.com/kyle-sylvestre/WinFish). Framework from
[kyle-sylvestre/PvZ-Portable](https://github.com/kyle-sylvestre/PvZ-Portable), based on
[wszqkzqk/PvZ-Portable](https://github.com/wszqkzqk/PvZ-Portable) and headshot2017's work (LGPL-3.0-or-later).
Controls and fixes informed by [insaniquarium_nx](https://github.com/ChanseyIsTheBest/insaniquarium_nx).
Details in `docs/UPSTREAM.md`.

This product includes portions of the PopCap Games Framework, © 2005-2009 PopCap Games, Inc. All rights
reserved. (http://popcapframework.sourceforge.net/)

Insaniquarium is a trademark of its owners. This project is not affiliated with or endorsed by PopCap Games or
Electronic Arts.
