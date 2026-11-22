# Development

How to build, run and test switchfish, and where things are. Why things are the way they are:
`docs/ANALYSIS.md`; how solid it is: `docs/STATUS.md`; what's next: `docs/ROADMAP.md`.

## Building

Switch builds only need Docker; the toolchain (devkitA64, libnx, SDL2, mesa, libopenmpt) lives in an image.

```bash
./sf.sh setup          # once: Docker image, Homebrew packages, Ryubing
./sf.sh build switch   # build/switch/switchfish.nro
./sf.sh build          # Mac build in build/host/
./sf.sh run            # Mac build with the game files in gameData/ (or --data <dir>)
./sf.sh emu            # the NRO in the Ryubing emulator
./sf.sh send <ip>      # over wifi to hbmenu's netloader (press Y): installs, starts, shows the log here
./sf.sh widen          # redraw the widescreen sides with Codex (needs Codex)
./sf.sh upstream       # clone or update the reference projects in upstream/
```

On the Mac a game controller drives the same cursor as on the Switch.

## On a Switch

After the game files are on the SD card only the NRO changes, and `./sf.sh send <switch-ip>` puts a new
build on the Switch over wifi in seconds: open hbmenu or Sphaira, start the netloader (Y in hbmenu), run the
command. Arguments after the IP go to the game (`./sf.sh send 192.168.1.20 --level 3-2`), and its log shows
in the terminal until Ctrl-C.

Logging: with `send` everything goes live to the terminal. Started normally the game keeps only its last 256
log lines in memory and writes them, with the crash address, to `crash.txt` if it crashes. A `debug.txt` in
the game folder makes it log everything to `log.txt` (slower, every line is a write to the SD card;
`./sf.sh emu` makes one for the emulator).

## Releases

Every push builds the NRO in CI (`.github/workflows/switch.yml`), in the same Docker image as `sf.sh`. A
published GitHub release gets `switchfish.nro` attached, with the release tag as its version
(`CMakeLists.txt` has the default) and the PopCap acknowledgment appended to the notes.

## Testing

- **Test plans** (`tests/plans/`, format in `port/TestPlan.h`) drive the game hands-free: clicks, keys,
  controller input and screenshots at fixed logic ticks. A run is silent, stays in the background and
  always ends by itself:
  `./build/host/switchfish --plan tests/plans/menus.txt --out /tmp/shots --timeout 45`.
  Screenshots read the canvas; the sides beside 4:3 screens and the pad cursor are added when the frame is
  presented, so they only show in a window capture.
- **Quickplay**: `./sf.sh run --quickplay` lets N finish the current level the normal way, and
  `--level 3-2` starts a new adventure at that level. On the Switch, a `quickplay.txt` in the game folder
  (optionally with `3-2` in it) does the same. It changes your profile, so use a test profile.
- **Screens deep in the game**: `--screen story` opens the pet stories and `--screen credits` the end
  credits once the main menu is up (also through `./sf.sh send <ip> --screen credits`).
- Boot lines (`boot: ...`) show where loading time goes: file opens, misses, directories read, time per
  resource type and per image format.

## Layout

| Folder | What |
|---|---|
| `game/` | the game (WinFish decompilation, kyle-sylvestre's portable fork) |
| `framework/` | SexyAppFramework on SDL2 + GLES2, with `platform/switch` and `platform/default` |
| `port/` | our own code: entry point, notice screens, test plans, Quick Aim targets, wide screen art, `romfs/` for the NRO |
| `scripts/` | how the widescreen sides are made |
| `third_party/` | SDL-Mixer-X, LZ4 |
| `docker/` | the Switch build image |
| `.github/workflows/` | CI |
| `docs/` | analysis, status, roadmap, controls, widescreen, loading, quality of life, where everything came from |
| `upstream/`, `tools/`, `gameData/` | reference projects, the emulator, your own game files (all gitignored) |
