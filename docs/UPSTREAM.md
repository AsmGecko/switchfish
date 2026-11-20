# Upstream code and licenses

## Vendored code
| Folder | Source | Commit | License |
|---|---|---|---|
| `game/` | https://github.com/kyle-sylvestre/WinFish `develop` (fork of vindirect/winfish) | `97cfa39aeb` (2026-10-31) | decompiled game code "as-is", modifications MIT (winfish README) |
| `framework/` | https://github.com/kyle-sylvestre/PvZ-Portable `insaniquarium`, `src/SexyAppFramework` | `11fe5bc67a` (2026-10-30) | LGPL-3.0-or-later + PopCap Games Framework License 1.1 (`framework/LICENSE`, `framework/LICENSE-LGPL-3.0.txt`, `framework/COPYING-GPL-3.0.txt`) |
| `third_party/SDL-Mixer-X/` | vendored by that framework | same | zlib for the parts we build (LGPL/GPL codecs are disabled) |

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
