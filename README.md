# switchfish

Insaniquarium Deluxe on the Nintendo Switch as native homebrew: the whole 16:9 screen is aquarium, and it
plays with a controller, touch or a USB mouse. Built from the source code the community decompiled, so it
runs the real game, not an emulator.

**You need your own copy of Insaniquarium Deluxe** (for example from Steam). switchfish contains no game
assets; it runs on the files from your copy.

## Getting started

1. Download `switchfish.nro` from the [latest release](https://github.com/AsmGecko/switchfish/releases).
2. Copy the folders from your Insaniquarium Deluxe install (`properties`, `images`, `sounds`, `music`, `data`,
   `fishsongs`, ...) and the NRO to the SD card:

   ```
   sdmc:/switch/switchfish/
   ├── switchfish.nro
   ├── properties/resources.xml
   └── images/ sounds/ music/ data/ fishsongs/ ...
   ```

   With hundreds of small files, hekate's USB mass storage (the SD card shows up as a normal disk) is much
   faster and more reliable than MTP.
3. Start it from hbmenu or Sphaira through title takeover (hold R while starting any game). Started from the
   album the Switch gives homebrew too little memory; switchfish tells you so.

The first boot takes a little longer: it builds an image cache from your files. After that it loads in a
few seconds. To update, replace `switchfish.nro`; your saves stay in `userdata/`.

## Features

### The whole screen is aquarium

The game was made for 640x480. In switchfish the tank is 854x480, so on a 16:9 screen fish swim, food falls,
coins drop and aliens chase you all the way to the edges.

- The extra sides of the six tank backdrops were drawn to match, glass walls and floor corners included, and
  the original art from your files sits in the middle, pixel exact.
- The title screen and the main menu are widened the same way. Hall of Fame, Help, the pet stories and the
  credits draw their water, bubbles and fish across the whole screen.
- Screens that stay 4:3 (the pet selection, the Virtual Tank store) sit on the water of the game's help
  screen, with slow bubbles and a soft shadow, instead of black bars.
- Sharp scaling keeps the pixel art crisp at 720p and 1080p.
- **Widescreen** in Options turns it off for the original 4:3 game, every number as it was.

### Made for a controller

| Button | What it does |
|---|---|
| Left stick | move the cursor; it speeds up the further you push and slows down over coins and buttons |
| ZL (hold) / right stick | precise aiming / small nudges |
| A or ZR | click; hold to keep dropping food |
| B | tap the glass; closes menus and dialogs |
| D-pad | jump from button to button |
| X | feed the hungriest fish (Quick Aim); hold to keep feeding, one after another |
| Y | grab the money: bombs first, then the coins about to vanish (Quick Aim) |
| ZL + X / Y | jump there without clicking |
| R (hold) | fast-forward at double speed (turn it on in Options) |
| + / − | menu / pause |
| Touch | tap directly in handheld mode; ZL + tap taps the glass |
| USB mouse | works in the dock |

- The controller **rumbles** when an alien arrives or goes down, for every hit, a tap on the glass, a fish
  dying and a pet hatching.
- **Options > Controls** opens a page in the game's own Help that explains all of this.
- Home or sleep pauses and mutes the game. The Switch keyboard opens for player names.

### Quick Aim and fast-forward

- **Quick Aim** (on by default): X and Y move the cursor to what needs you and click there, so the game plays
  with buttons alone if you like. It only aims at fish the game already shows as hungry and never clicks a
  pet that happens to be in the way. Turn it off in Options for the classic game.
- **Fast-forward** (off by default): hold R to run a level at double speed. The game logic stays exactly the
  same, it just runs twice as many steps. Not in the Virtual Tank.

### New in Options

| Setting | Default | |
|---|---|---|
| Widescreen | on | the 16:9 tank and screens; off is the original 4:3 game |
| Quick Aim (X/Y) | on | X feeds the hungriest fish, Y grabs the money |
| Rumble | on | controller rumble |
| Fast-Forward (hold R) | off | double speed while R is held |
| Controls | | the button guide |

### Loads fast, saves safely

- About a second to the first picture and a few to "Click here to play" (early builds took over 30).
- Saves and settings are written so that a crash or a flat battery can never leave half a save behind.
- Closing the game from the Home menu saves the level you are playing, like quitting does.

## Known issues

- **Quitting shows an error** on real hardware ("The software was closed because an error occurred",
  2001-0106). It happens after the game has finished and saved; your progress is safe. Being worked on.
- Not everything has had a long run on a real Switch yet: rumble strengths and touch may need tuning.

## Help

- **The game says it can't find its files:** the folders must be directly in `sdmc:/switch/switchfish/`,
  next to the NRO (so `sdmc:/switch/switchfish/properties/resources.xml` exists).
- **Something looks wrong after changing game files:** delete `imagecache.bin` in that folder; it is rebuilt
  on the next start.
- **It crashed:** switchfish writes `crash.txt` with its last log lines in its folder. Please attach it to an
  issue.

## Building it yourself

A build of your own carries the fish from your own title screen as its icon. You need Docker; the Switch
toolchain lives in an image:

```bash
./sf.sh setup          # once
./sf.sh build switch   # build/switch/switchfish.nro
```

Building, testing, sending builds to a Switch over wifi and how the repo is laid out: `docs/DEVELOPMENT.md`.

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
  **SDL-Mixer-X**, **libopenmpt**, **LZ4** and the **Fredoka** font.

Where every vendored folder came from and at which commit: `docs/UPSTREAM.md`.

## Credits and license

switchfish is made by [AsmGecko](https://github.com/AsmGecko).

Our own code is MIT; the framework stays LGPL-3.0-or-later and the PopCap Games Framework License, the game
code is the WinFish decompilation's. `LICENSE` lists which license covers which folder.

This product includes portions of the PopCap Games Framework, © 2005-2009 PopCap Games, Inc. All rights
reserved. (http://popcapframework.sourceforge.net/)

Insaniquarium is a trademark of its owners. This project is not affiliated with or endorsed by PopCap Games or
Electronic Arts.
