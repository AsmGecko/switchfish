# Faster loading on the Switch

Done so far: step 1 (directory indexes in fcaseopen, misses answered from memory), step 2 (MO3 type up
front), step 0 (boot lines with opens, misses, directories read and time per resource type), the RegEmu log
spam. Hardware went from 38.6 s to 11.8 s before misses were trusted; Ryubing from 7.1 s to 1.1 s. Then 7.7 s on
hardware, of which images 4.5 s, nearly all GIF decoding. Step 4 (decode ahead on cores 0 and 2,
framework/imagelib/ImagePrefetch.cpp) halved the image time on the Mac and in Ryubing. Not doing step 3
(CPU boost): the remaining time should come out of the data handling, not the clock.

Research by an agent on 2026-11-21 from the hardware boot log (38.6 s to "click here to play"). Work through
it in order; check off what is done.


## Summary

Two bugs account for roughly 30 of the 38 seconds, and neither is decoding work.

1. Every image and sound is probed under several extensions, and each probe that misses triggers a full
   directory scan in `fcaseopen`. About 1,860 misses, about 785,000 directory entries read.
2. SDL-Mixer-X's MP3 detection reads each `.mo3` one byte at a time with a "tell" after every byte. On newlib
   that throws away the 32 KB stdio buffer each time: about 29,000 `fsFileRead` calls for 400 KB of music.

Expected after fixing both: about 38 s down to 8 to 11 s. With CPU boost and parallel decoding: 4 to 5 s.
The numbers below come from simulating the code paths against `gameData/` on the Mac and from timing
libopenmpt there. Nothing was measured on hardware, so step 0 is to confirm.

## 0. Instrument first (half an hour, no behaviour change)

- Counters in `framework/fcaseopen/fcaseopen.c`: fopen calls, failed fopens, opendir calls, dirents read.
  Print them on every `boot:` line.
- Accumulated ms per resource type in `ResourceManager::LoadNextResource` (framework/misc/ResourceManager.cpp:913):
  image, sound, font. Inside `ImageLib::GetImage` split "open/probe" from "decode".
- One `boot: music <file> <ms>` line per `LoadMusic`.

Prediction to check: for the loading thread about 3,700 fopens, 1,680 scans, 725,000 dirents for images,
plus 383 fopens and 177 scans for sounds.

## 1. Directory scans on every missing file: about 20 to 24 s (plus about 1 s of the Init group)

Evidence:
- `ImageLib::GetImage` tries `.png .jpg .gif .tga` in that order (framework/imagelib/ImageLib.cpp:1290, 1383),
  then does the same for the mask `_name` and, if that is missing, `name_` (1461-1473). resources.xml has no
  extensions, and almost everything is `.gif`. An image without a mask costs 10 misses, one with a mask 4.
- `SDLSoundManager::LoadSound` tries `.wav .mp3 .ogg` and then `.au` (framework/sound/SDLSoundManager.cpp:362-385).
- Each miss goes `fcaseopenat` (fcaseopen.c:313-321): failed fopen, `casepathat` (140-222) opens and scans the
  game folder, opens and scans all 427 entries of `images/`, then a second failed fopen.
- Simulated totals for the 326 resources: 4,314 fopen calls for 441 real files, 1,947 scans.
- Calibration: the Init group has 72 scans and took 1.2 s, so about 16 ms per miss. 1,858 misses in the
  loading thread at that rate is the measured 27.6 s almost exactly (a few seconds of that is really decoding).

Fix: a directory index in fcaseopen. On first use of a directory, read it once into a map of lowercased name
to real name. Read-mode opens resolve through it: a miss returns NULL with no syscall, a hit opens the real
name directly. Write-mode opens bypass it and drop that directory's index (saves in `userdata/`, the log).
Guard with a mutex. This keeps the case-insensitive rule from CLAUDE.md and stays in `framework/`; `game/` is
untouched.

Risk: low. A file created at runtime in an indexed folder would be invisible, hence the invalidation on write.
Verify: scans drop to one per directory (about 5), failed fopens to 0. Ryubing (7 s, also IPC bound) should
drop too, which makes it a usable proxy before testing on hardware.

## 2. MP3 detection on the MO3 files: about 7.5 s of the 8.7 s

Evidence:
- `Mix_LoadMUS` (framework/sound/SDLMusicInterface.cpp:58) runs `detect_music_type`, which falls through to
  `detect_mp3` (third_party/SDL-Mixer-X/src/music.c:1683, 1384-1455). Its loop at 1414-1418 reads 1 byte and
  calls `SDL_RWtell` twice, for up to 10,240 bytes.
- SDL's stdio tell is `fseek(fp, 0, SEEK_CUR)`, and newlib's fseek flushes first, which discards the read
  buffer. BUFSIZ is 32768 in devkitA64 newlib 4.6, so every byte costs a fresh 32 KB read from sdmc.
- Simulated: 10,271 + 10,255 + 8,545 buffer refills, about 700 MB copied through IPC.
- libopenmpt itself loads all three files in 58 ms on the Mac (43 ms for Insaniq2 with its 109 samples), so
  maybe 1 to 1.2 s on the Switch. The rest is the scan.

Fix: in `SDLMusicInterface::LoadMusic`, read the file into memory with one fread and call
`Mix_LoadMUSType_RW(SDL_RWFromConstMem(...), MUS_MOD, 1)`. A given type skips detection, and a memory RWops
makes any remaining seek free. No change in `third_party/`.

Risk: low. Verify with the per-file music timing: expect about 0.1 s, 0.9 s, 0.05 s.

## 3. CPU boost while loading: about 40% of what is left (2 to 4 s)

After 1 and 2 the loading thread is CPU bound: 45 M GIF pixels plus 3.4 M JPEG pixels, alpha merging
(ImageLib.cpp:1396), `CommitBits` scanning every pixel again (framework/graphics/MemoryImage.cpp:824), and 50
OGG files decoded and resampled up front. Homebrew runs at 1020 MHz by default and nothing in the repo
changes that.

Fix: `appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad)` (1785 MHz) in `port/Main.cpp` before `app.Init()`,
back to Normal when the loading thread finishes and on exit.
Risk: the GPU clock drops during boost, which is fine for the title screen. Verify with the per-type timings.

## 4. Decode ahead on the idle cores: another 2 to 3 s

The main thread only draws the title screen and cores 1 and 2 are free (core 3 belongs to the system).
Keep `LoadNextResource` and its order as they are, and add one or two workers that walk the group's image
list ahead and decode into a small bounded map of path to `ImageLib::Image*`; `SexyAppBase::GetImage`
(framework/SexyAppBase.cpp:3593) takes from it when present. Progress counting and resource order stay
identical.

Needs: `ImageLib::gAlphaComposeColor` is a global set per resource (ResourceManager.cpp:710-712) and must
become a parameter. The index from step 1 needs its mutex.
Risk: medium (threading in decompiled-adjacent code). Do it only if step 0 numbers after 1 to 3 still justify it.

## 5. Music off the critical path: about 1 s

Load the song the title screen plays first (`PlayMusic(2, 45)`, game/WinFishApp.cpp:2367, after
`RemapMusicTrack`), and the other two after the resource groups or on a second thread.
`SDLMusicInterface` already locks its map. Risk: low, as long as nothing plays those songs before loading ends.

## 6. Small things: about 0.3 to 0.5 s

- Logging: each line is fprintf plus fflush to the SD card (port/Main.cpp:122-123), about 12 ms per line. The
  15 "RegEmu: Value does not exist" lines (framework/misc/RegEmu.cpp:215) cost 0.2 s before `boot: init`.
  Flush only on `boot:` lines and at exit, and drop that RegEmu line to verbose.
- `PakInterface::NormalizePakPath` builds `std::filesystem::path` objects on every open
  (framework/paklib/PakInterface.cpp:51-104, 216) although no pak is loaded. Skip it when the record map is empty.
- Whole-file reads: sounds already do this; images would save a few buffer refills per file. Low priority.

## Considered and not recommended now

- Decoded RGBA cache: 48 M pixels is 193 MB raw. Reading that from SD is slower than decoding 7 MB of GIFs.
  Only viable with fast compression (LZ4, not in the toolchain image today). Revisit if 1 to 4 leave more than 5 s.
- Decoded audio cache: about 12 MB of PCM to save about 1 s of OGG decoding. Step 4 can cover sounds instead.
- A pak built on first run from the player's own files (PakInterface already reads paks in one fread) would
  replace 441 opens with one read, about 1 s. It must live in the save folder and never be shipped.
- Loading resources lazily after the title screen changes the classic flow (loader bar, ids extracted per
  group), so not without a deliberate opt-in.
- Texture upload happens at first draw (GLInterface `CreateImageTexture`), not during loading. It causes
  in-game hitches at most, so it is out of scope here.

## Order of work

0, 1, 2 on the host build with the counters, check in Ryubing, then hardware. Then 3, 6, 5. Decide on 4 from
the new numbers.
