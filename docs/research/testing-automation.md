# Hands-off testing on Switch (researched 2026-11-20)

The question: is there something like Felk's Dolphin (Python inside the emulator, per-frame input, memory reads,
screenshots) for Switch? No. But because we own the source, we can do better than that.

## What exists
| Tool | Input | Memory | Screenshots | Headless | Notes |
|---|---|---|---|---|---|
| Ryubing (Ryujinx fork) | none scripted | GDB stub (`--enable-gdb-stub`, port 55555) | GUI only | `--no-gui` still opens a window | macOS universal build, loads `.nro`, no argv passthrough. Active (2026-11-14) |
| Eden (yuzu fork) | TAS-nx `script0-1.txt`, buttons and sticks only, Qt GUI hotkey | GDB stub | GUI only | `eden-cli` with Null renderer | `eden-cli -g x.nro -p "args"` passes argv. macOS experimental. Releases DMCA'd Feb 2026, source self-hosted |
| sys-botbase / sbb3 (real hardware) | buttons, sticks, **touch** over TCP 6000 | peek/poke in the application process | `pixelPeek` JPEG | n/a | NRO must run via title takeover (application mode) |
| Atmosphère gdbstub | no | full GDB incl. hardware watchpoints | no | n/a | `enable_standalone_gdbstub`, port 22225 |
| nxlink | no | no | no | n/a | uploads + starts NRO, `--args`, `-s` for stdout |
| nxbt / joycontrol | Bluetooth controller from Linux | no | no | | no macOS, stale |

No CI anywhere runs a Switch NRO in an emulator; devkitPro builds them and nothing runs them. devilutionX
tests gameplay with a headless **host** demo replay (`test/timedemo_test.cpp`) and compares the resulting save.

## Our plan
1. **In-game test harness** (compiled in behind an option), identical on host and Switch:
   - JSON plan: `{tick, click:[x,y]}`, `{key}`, `{wait_until: "money>=200", timeout}`, `{screenshot}`, `{assert}`.
   - Inject input as Sexy-level mouse/key events, so it is independent of SDL, touch or stick mapping.
   - Named probes read from game objects (money, tank, level, fish count). No address tables like in Dolphin.
   - JSONL log, PNG frame dumps, exit code / sentinel line for the outer judge.
   - Host: SDL `dummy`/offscreen video and an uncapped update loop, so a whole level runs in seconds.
2. **Demo replays**: the framework's `.dmo` record/playback is still in the code (`-record`, `-play`,
   format documented in `upstream/OpenInsaniquarium/docs/re/framework/demo-recording.md`). Playback
   survived in the SDL framework, recording must be re-attached to the SDL event path. Use for same-build
   regressions (x86 vs arm64 float/RNG differences make cross-build replays unreliable).
3. **Switch-side validation** on real hardware: same harness in the NRO, launched by `nxlink --args`,
   stdout over nxlink, sys-botbase for real touch/HID input and JPEG checkpoints, gdbstub for crashes,
   `/atmosphere/crash_reports/` via ftpd.
4. Emulator smoke test (boots, no crash): Ryubing canary 1.3.354 in portable mode, `./sf.sh emu` (add
   `--no-gui` for a bare window). Do not judge rendering or perf there.

## Switch setup (when we get there)
1. Atmosphère 1.12.2, hbmenu via title takeover (hold R on a game).
2. One of: sys-botbase v2.5 (olliz0r) or sys-botbase-cpp 3.33 (PokemonAutomation, Wi-Fi or USB).
3. `/atmosphere/config/system_settings.ini`: `[atmosphere] enable_standalone_gdbstub = u8!0x1`, `enable_htc = u8!0x0`.
4. `./sf.sh send <ip>` (nxlink). For `-s` stdio, run nxlink natively since UDP broadcast doesn't cross Docker.
