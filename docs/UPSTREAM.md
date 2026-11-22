# Upstream code and licenses

## Vendored code
| Folder | Source | Commit | License |
|---|---|---|---|
| `game/` | https://github.com/kyle-sylvestre/WinFish `develop` (fork of vindirect/winfish) | `97cfa39aeb` (2026-10-31) | decompiled game code "as-is", modifications MIT (winfish README) |
| `framework/` | https://github.com/kyle-sylvestre/PvZ-Portable `insaniquarium`, `src/SexyAppFramework` | `11fe5bc67a` (2026-10-30) | LGPL-3.0-or-later + PopCap Games Framework License 1.1 (`framework/LICENSE`, `framework/LICENSE-LGPL-3.0.txt`, `framework/COPYING-GPL-3.0.txt`) |
| `port/romfs/fonts/Fredoka-SemiBold.ttf` | Fredoka (google/fonts `ofl/fredoka`), cut to weight 600 width 100 with fonttools | 2026-11-20 | SIL OFL 1.1 (`Fredoka-OFL.txt`) |
| `port/romfs/widescreen/aquarium1-6.png` | sides for the wide tank, drawn by an image model (Codex) with `./sf.sh widen`; the original middle is cut out | 2026-11-20 | ours (MIT, `LICENSE`) |
| `port/romfs/widescreen/selectorback.png`, `titlescreen.png` | sides for the main menu and the title screen, same way | 2026-11-22 | ours (MIT, `LICENSE`) |
| `switch/icon.jpg` | the fallback NRO icon (release builds, no game files): the game's guppy redrawn by an image model (Codex) from a screenshot, the one deliberate exception to "no PopCap art" (see CLAUDE.md) | 2026-11-22 | redrawn fan art of a PopCap character, not covered by our MIT license |
| `third_party/SDL-Mixer-X/` | vendored by that framework | same | zlib for the parts we build (LGPL/GPL codecs are disabled) |
| `third_party/lz4/` | https://github.com/lz4/lz4 `lib/lz4.c`, `lib/lz4.h` | v1.10.0 | BSD 2-clause (`third_party/lz4/LICENSE`) |

The PopCap license requires the acknowledgment "This product includes portions of the PopCap Games
Framework, © 2005-2009 PopCap Games, Inc. All rights reserved. (http://popcapframework.sourceforge.net/)"
in end-user documentation, and forbids "PopCap" in the product name.

No game assets are in this repo or in any build. Players supply their own copy of Insaniquarium Deluxe.

## Reference clones (`upstream/`, gitignored)
Listed in `docs/upstream.txt`; `./sf.sh upstream` clones or updates them. Commits when this repo was set up:

| Clone | Commit |
|---|---|
| winfish | `f919b3c241` (2026-10-08) |
| Insaniquarium-Decompilation | `8140ae981b` (2026-10-03) |
| OpenInsaniquarium | `1cbe6d47bb` (2026-11-13) |
| insaniquarium_nx | `dc3930612e` (2026-09-06) |
| kyle-WinFish | `97cfa39aeb` (2026-10-31) |
| kyle-PvZ-Portable | `11fe5bc67a` (2026-10-30) |
| PvZ-Portable | `0c2c42c634` (2026-11-20) |
| headshot-re-pvz | `cfabf8b1b8` (2026-01-28) |

## Pulling upstream changes
The vendored trees diverge from upstream on purpose (include paths, entry point, trimmed platforms). To
take a fix, diff the upstream commit against our file and apply it by hand, e.g.
`git -C upstream/kyle-WinFish show <commit> -- source/WinFish/Fish.cpp`.
