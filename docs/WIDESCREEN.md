# Widescreen design

Status 2026-11-20: design only, nothing implemented. Expands `ANALYSIS.md` section 7 and roadmap M5.
Line numbers refer to the tree as of this date. Claims marked *(guess)* are not measured.

Goal: the game fills the whole 16:9 Switch screen (1280x720 handheld, 1920x1080 docked). Classic 4:3 stays
available and plays exactly like the original. We ship no art; everything wider than 640 is drawn by our
code or generated at load time from the player's own files.

Hard rule for every visual change: **it must not look worse than the original.** That means no stretched
pixel art, no upscaled blurry sprites, no visible seams or mirror images. Every widened component in
section 3 says how it is built and whether we are sure, or have to prototype and compare screenshots first.

## Status (2026-11-20)

Built:
- Canvas 854x480 with the 640x480 design space centred (`mScreenBounds`), drawn into a persistent FBO and
  put on screen with a sharp-bilinear shader (`GLInterface::Present`). Screens that don't paint the whole
  canvas get blurred, darkened sides. Settings `Widescreen` and `WideTank` (registry), both on by default.
- Wide tank: `TankLeft()/TankRight()` through every bound, spawn range and food zone (0..640 in the classic
  tank, -107..747 wide), save fix-up on load (`FixLoadedXBounds`), Board draws and takes clicks across the
  canvas, waves and light bands continue, tank mask widened.
- Backdrops: instead of quilting, an image model (Codex) fills the sides of each backdrop
  (`./sf.sh widen`, `scripts/widescreen/`), with the whole original attached as a reference so it draws the
  glass side walls and floor corners at the new edges in the original's perspective, with at most a few
  small, quiet details in the outer parts (no new structures, no copies). A plain version without details
  was kept as `gameData/widescreen-alt/plain/`. The game keeps the original middle (inside the old glass
  walls) pixel exact, feathered over 12 px, with the colour difference across each seam faded into the new
  part.
- The pack (`port/romfs/widescreen/`, in the repo and the NRO) only stores what the model drew: `fit.py`
  makes the middle transparent except a 24 px strip at each seam, which the game blends and colour matches
  against. The game rebuilds the middle from the player's own files, and the result is pixel identical to a
  full pack (checked for all six backdrops and tank masks). A `widescreen/` folder next to the game files
  wins over the built-in one, for experiments. Without any pack the sides are a blurred mirror of the edge.

Not yet: HUD wings (the bar stays 640 and floats over the water), native wide backgrounds for menu screens,
PetDialog/FoodDialog and the starfield cheat at canvas width, an options toggle, a 4:3 determinism check.

## 0. Key finding: the framework already knows how to do this

PopCap's later SexyAppFramework had a "widescreen aware" mode, and our framework still carries the hooks:

- `SexyAppBase.h:341,344` `mWidescreenAware`, `mWindowAspect` (set at `SexyAppBase.cpp:308-310`, unused).
- `SexyAppBase.cpp:2888-2892`: `mScreenBounds.mX = (mWidth - mGLInterface->mWidth) / 2`, then
  `mWidgetManager->Resize(mScreenBounds, mPresentationRect)`.
- `WidgetManager.cpp:186,474`: every frame is drawn with `g.Translate(-mMouseDestRect.mX, ...)`, and
  `RemapMouse` (`WidgetManager.cpp:230-234`) applies the inverse to mouse input.

In PopCap's DirectDraw version (`upstream/headshot-re-pvz/SexyAppFramework/graphics/unused/DDInterface.cpp:
310-345, 406-430`) a widescreen-aware app keeps its design size (`app->mWidth` = 640), the *draw buffer*
gets `mHeight * aspect` wide (853), and `mScreenBounds.mX` comes out negative (-107). The WidgetManager
then centres the 640x480 design space inside the wide buffer, and anything drawn at x < 0 or x >= 640 lands
in the side areas. Mouse coordinates come back in design space.

Today `GLInterface::mWidth == app->mWidth`, so the offset is 0 and nothing happens. Adopting this convention
is the core of the design:

- **Design space stays 640x480** for every screen, widget, dialog, HUD element and harness plan. About 100
  hard-coded UI coordinates keep working unchanged.
- **The canvas is 854x480.** In design coordinates it spans x = -107 .. 747. Code that wants to fill the
  screen draws over that range; code that doesn't is centred automatically.
- A wide tank is the same trick for the simulation: tank bounds go from 0..640 to -107..747, so art-anchored
  positions (Niko, bubbulator, alien attractor, the HUD money counter) keep their numbers.

## 1. Inventory: where 640x480 lives

Measured with grep over `game/` (excluding the generated `Res.cpp/h`) and `framework/`.

**Refined count.** Lines with a literal `640|480|639|479`: 81 lines in 27 files. 12 of them are sprite-sheet
source offsets (`Rect(aHungr ? 480 : 720, ...)` in Fish/Gekko/Grubber/Oscar/Breeder/Ultra/DeadFish/Missle,
a commented line in Beetlemuncher, a comment in Board), which leaves 69 real screen-geometry sites. The
earlier "about 185" also counted derived values (540, 550, 520, 320, ...). Grouped by what they mean:

| Group | Sites | Must change for |
|---|---|---|
| Framework presentation and input | ~20 | every widescreen mode |
| Tank simulation bounds, spawn ranges, input zones | ~95 lines | wide tank only |
| Tank rendering (backdrop, waves, lighting, fills) | ~20 | widescreen view and wide tank |
| HUD | ~25 | nothing (stays in design space); new wing drawing only |
| Dialogs | 6 | wide tank only (2 dialogs cover the tank) |
| Menus and other screens | ~45 literals, 143 `Resize` calls | nothing; 9 screens get an optional wide background |

With the design-space approach only the first three groups need edits, around 135 lines, and the wide-tank
group is mechanical.

### 1.1 Framework presentation and input

| Where | What |
|---|---|
| `SexyAppBase.cpp:186-187` | `mWidth = 640; mHeight = 480` (design size; stays) |
| `SexyAppBase.cpp:308-310`, `.h:341-344` | unused `mWidescreenAware`, `mWindowAspect(4,3)` |
| `SexyAppBase.cpp:789` | dialogs centred with `(mWidth - w) / 2` (design space, fine) |
| `SexyAppBase.cpp:1730,1733` | FPS and demo overlays at `mWidth - ...` |
| `SexyAppBase.cpp:2736-2737, 2888-2892, 3441-3444` | `mScreenBounds` and WidgetManager resize |
| `GLInterface.cpp:1058-1063` | `GLInterface::mWidth/mHeight` copied from the app: **becomes canvas size** |
| `GLInterface.cpp:1122-1149` | `UpdateViewport`: hard-coded 1280x720 on Switch and hard-coded 4:3 letterbox |
| `GLInterface.cpp:1210` | ortho matrix from `GLInterface::mWidth` |
| `GLInterface.cpp:82, 727-739, 1206` | per-draw `GL_NEAREST`/`GL_LINEAR`. Today the game draws straight to the 960x720 backbuffer, so 1.5x `NEAREST` doubles every other pixel unevenly |
| `GLInterface.cpp:1240-1243` | screen image sized to `GLInterface::mWidth` (so the clip covers the whole canvas) |
| `WidgetManager.cpp:186-187, 430, 474, 490` | global translate by `-mMouseDestRect.mX` |
| `WidgetManager.cpp:230-234, 351-357` | `RemapMouse`, `Resize` |
| `WidgetContainer.cpp:49, 621` | `mClip` (widgets clip to their bounds by default) |
| `WidgetContainer.cpp:127-161` | hit test via `GetInsetRect()`, so negative `mMouseInsets` widen a widget's click area |
| `platform/switch/Window.cpp:81` | EGL surface from `nwindowGetDefault()`, always 1280x720 (docked is upscaled by the system) |
| `platform/switch/Input.cpp:139-184`, `platform/default/Input.cpp:461-473` | all mouse/touch goes through `RemapMouse` |
| `platform/default/Window.cpp:70,106,161-162` | host window sized from `mWidth` |

### 1.2 Tank simulation (only for the wide tank)

Left-edge values (10, 15, 20, -10, 0) and right-edge values (490..587) both move, because in design space
the wide tank runs from -107 to 747.

**A. Bounds and clamps (66 lines):**

| File:lines | Values | Notes |
|---|---|---|
| `Fish.cpp:1521-1522` | `mXMin = 10, mXMax = 540` | inherited by guppies, carnivores, all FishTypePets, BallFish, BiFish, Gekko, Oscar |
| `Fish.cpp:517` | soft left wall `mXD < 15` | line 515 already uses `mXMax - 5`; 136-137 and 460-462 use the members |
| `Breeder.cpp:706-707, 396` | same as Fish | 363-365, 394 use the members |
| `FishTypePet.cpp:116, 419` | Amp `mXMax = 460`, soft wall 15 | 396-398, 417 use the members |
| `SylvesterFish.cpp:497,499,510,512` | 10/540 and 0/480 | |
| `Ultra.cpp:493-494` | 0/480 | |
| `Alien.cpp:253-256, 264-267` | mini-Sylv 10..540, others -10..490 | |
| `DeadAlien.cpp:85-88` | -10..490 | |
| `DeadFish.cpp:161-164` | 10..540 | |
| `Food.cpp:141-144` | 20..550 | |
| `Larva.cpp:50-51` | 530 | |
| `Grubber.cpp:70, 79-82, 103, 105` | 10/540, 10..560, 15/535 | |
| `Penta.cpp:72, 79-82, 103, 105` | 10/540, 10..550, 15/535 | |
| `OtherTypePet.cpp:213, 232-236, 240-241, 267, 269` | 10/540, cap 540/550/560, 15/535 | Stinky, Rufus, Clyde, Rhubarb |
| `Missle.cpp:117-120, 130` | clamp 10..550, removal x > 580 or x < -20 | the removal is a classic "off screen" rule |
| `BilaterusBone.cpp:32`, `BilaterusHead.cpp:39` | `= 540` | |

**B. Spawn ranges (24 lines, `Board.cpp`):** `Next() % 520 + 20` at 491, 543, 554, 2295, 2310, 2325, 3538,
5338, 5354, 5381, 5395, 5411, 5427, 5441, 5455, 5471; `Next() % 450 + 20/50` at 579, 1037, 1039, 1065, 1067,
3291, 5512; bubble bounds `Rect(0, 82, 640, 398)` at 146.

**C. Input zones (4):** food drop `x > 30 && x < 587` at `Board.cpp:1637, 1688`, `Fish.cpp:598`,
`Breeder.cpp:475`.

**D. Serialized bounds (6):** `SyncLong(mXMin/mXMax)` at `Fish.cpp:913,915`, `Breeder.cpp:659,661`;
`BilaterusBone.cpp:101`, `BilaterusHead.cpp:138`. Mid-level saves (`Board::SaveGame` 2406, `LoadGame`
2712) and the virtual tank store these.

**E. Keep as they are (design-space anchors and AI targets):**

- art-anchored per backdrop: `OtherTypePet::UpdateNikoPosition` (395-440), `Board::GetBubbleSpawnCoords`
  (4873), `Board::GetAlienAttractorSpawnCoords` (4905)
- coin/food/larva fly to the money counter at (550, 30): `Coin.cpp:332-339`, `Food.cpp:85-93`,
  `Larva.cpp:55-61`. Only the Y convergence decides when the item is removed (`mY < 40`), so X is cosmetic
- swim band 175..250: `Fish.cpp:288,302`, `Breeder.cpp:282,296`, `FishTypePet.cpp:306,320`
- `DeadFish.cpp:27` (`theX < 115` velocity tweak), every `mYD > 240` (vertical)

The left/right pairs are not all symmetric (Alien -10..490 is "640 minus the 150 px sprite"), so each site
gets converted on its own. A blanket replace of 540 would repeat PvZWidescreen's bug, where patching a
shared screen constant also changed Zomboni speed thresholds (HenryJk/PvZWidescreen issue #40).

### 1.3 Tank rendering

| Where | What | Wide handling |
|---|---|---|
| `Board.cpp:5770-5831` `DrawTankBackground` | backdrop at (0,0), fills `FillRect(0,0,640,480)` at 5776/5791 (white "void" cheat, shake) | canvas rect |
| `Board.cpp:5815-5826` | 3 light bands `IMAGE_TANKLIGHTING` (640x80) drawn at `pos` and `pos - 640` | draw enough copies to cover the canvas; keep the 640 wrap (it is the image period) |
| `Board.cpp:167, 899-902` | light band positions: `mSeed->Next() % 640` and the 0..640 wrap | **leave alone**: `mSeed` is the sim RNG |
| `Board.cpp:5828` | `IMAGE_TANKMASKn` at (0,365); built at load from AQUARIUMn rows 365..445 plus `_maskN.jpg` (`WinFishApp.cpp:561-577`) | built from the extended backdrop |
| `Board.cpp:5833-5843` `DrawTankWaves` | WAVESIDE at 0, WAVE at 160 and 320, mirrored WAVESIDE at 480 | tile WAVE with a clip between the two caps |
| `StarField.cpp:33-34, 88, 98` | "space" cheat, `Rand() % 640` | canvas width (same call count) |
| `Board.cpp:5705-5765` | Windows screensaver banners | dead code on Switch (M4) |

### 1.4 HUD (stays in design space)

`Board::Draw` (1204) draws `IMAGE_MENUBAR`/`IMAGE_TROPHYBAR` (640x75) at (0,0) at 1209-1211, the money
flash at (545,39) at 1213, the help arrows and texts at 1218-1252. HUD widgets: menu button 200, money label
205, message widget 4372, slot buttons 4382-4428 and 4686, Presto button `GetPrestoButton()` 75-86. None of
these move. The only new thing is drawing bar wings at x = -107..0 and 640..747 (section 3.3).

### 1.5 Dialogs

All dialogs are 9-slice `IMAGE_DIALOG` panels positioned in design space (`SexyAppBase.cpp:789`,
`WinFishApp.cpp:1475` centres one with `320 - w/2`). They need no change. Two cover the tank and need
widening for the wide tank only: `PetDialog.cpp:21, 57-58, 63` (full-screen dim with 70 px side bands and a
border at 639x479) and `FoodDialog.cpp:17` (a 640x50 strip under the HUD).

### 1.6 Menus and other screens

All are top-level widgets created with `Resize(0, 0, mWidth, mHeight)` (`WinFishApp.cpp:475, 1913, 1931,
2173, 2343-2394`) and lay out their children in 640x480. With the design-space approach they stay exactly as
they are; what matters is what fills the sides.

| Screen | Background | 640/480 literals | Side fill |
|---|---|---|---|
| Title (`TitleScreen.cpp:95`) | `titlescreen.jpg`, full-bleed painting | 0 | ambient |
| Game selector (`GameSelector.cpp:230`) | `selectorback.jpg`, painting with brass panel | 1 | ambient |
| Store (`StoreScreen.cpp:166`) | `Store.jpg`, stone wall painting | 2 | ambient |
| Hatch, Bonus (`HatchScreen.cpp:146`, `BonusScreen.cpp:188`) | `hatchscreen.gif`, brass panels and pipes | 3, 1 | ambient (SCREENBACK side panels as an option) |
| Help page 0, Pets, Virtual tank fish (SimFish), Tank select, Sim setup | `IMAGE_SCREENBACK` 9-slice at `Rect(-5,-5,650,490)` (`HelpScreen.cpp:137`, `PetsScreen.cpp:51`, `SimFishScreen.cpp:786`, `TankScreen.cpp:111`) | 5, 3, 3, 3, 0 | native: widen the 9-slice |
| Help pages, High scores (`HelpScreen.cpp:141`, `HighScoreScreen.cpp:90`) | horizontal stripes (`FillRect(0, i, mWidth, 4)`), bubbles, `DrawWaves` | 5, 4 | native |
| Interlude, Story (`InterludeScreen.cpp:164`, `StoryScreen.cpp:190`) | solid `Color(25,105,195)`, bubbles | 11, 1 | native |
| Tank (Board) | AQUARIUMn backdrop + HUD | 21 | section 3.2 |

Things that rely on "off screen" there: Angie enters at x = 640 in the interlude (`InterludeScreen.cpp:195,
201`), bubble fish spawn at the bounds' edges. Screens keep `mClip = true` unless they draw a native wide
background, so nothing that was hidden off screen becomes visible. That is the PvZ "clouds jump into view"
bug (PvZWidescreen issue #23), and the clip avoids it.

## 2. Options

| | What the player sees | Sim | Art risk | Work |
|---|---|---|---|---|
| (a) Pillarbox, nice sides | 4:3 sharp in the middle, sides an ambient blur of the current frame | identical | none | S-M |
| (b) Widescreen view | tank backdrop and HUD extend to the edges; fish stay in the original 640 tank (glass walls where they always were); menus centred | identical | low | M |
| (c) Wide tank | the tank itself is 854 wide, fish/coins/aliens use all of it | changed (opt-in) | high (backdrops must extend sharply) | L |
| (d) Hybrid | (b) or (c) for the tank, menus 4:3 with native wide backgrounds where cheap and ambient elsewhere | per tank choice | as above | (sum) |

(d) isn't a separate mode, it is how (b) and (c) handle the non-tank screens. So the player-facing settings
come down to two:

- **Screen:** `Classic 4:3` (black bars or soft sides) / `Widescreen`.
- **Tank size:** `Classic` / `Wide (changes gameplay)`, only offered with Widescreen.

What others did (research summary, details in the notes at the end):

- PvZ: Replanted (2025, official) extended the backgrounds and kept all gameplay in the 4:3 area.
- PvZWidescreen (hor+, 1066x600) needed about 18 patch modules and still has open UI and gameplay bugs.
- PvZ-Portable refuses widescreen altogether.
- Diablo II Resurrected dropped ultrawide because the screen edge was acting as a game rule.
- Nobody has made Insaniquarium widescreen yet (OpenInsaniquarium lists it as unstarted).

That supports (b) as the default and (c) as the opt-in that makes this port unique.

### 2.1 Logical resolution

Height stays 480: every backdrop, the HUD and the tank floor are 480 tall, and adding rows means inventing
a taller tank. That rules out 960x540 (its 2x integer scale at 1080p is tempting, but it needs 60 new rows).

- 16:9 at 480 rows is 853.33 wide. **854x480**: (854-640)/2 = 107 exactly, so the design space sits on
  whole pixels. At 720p the scale is 1280/854 = 1.4988 horizontally against 1.5 vertically, a 0.08%
  squeeze nobody can see. (852 gives 106 px margins and a 1 px black column on each side at 720p.)
- Height-based scale factors: 1.5 at 720p, 2.25 at 1080p. Neither is an integer, so integer scaling is
  impossible on either Switch mode without bars or cropping.
- The canvas width should be computed from the output aspect, not hard-coded: `clamp(round(480 * w / h)
  to even, 640, 854)`. That makes the host window, a 16:10 screen, or the Classic mode (640) all the same
  code path. The *tank* width is not dynamic (balance must not depend on window shape): it is 640 or 854,
  and with the wide tank on a narrower screen the canvas is forced to 854 and letterboxed vertically.

### 2.2 Scaling

Render the canvas into an FBO at logical size (854x480), then present it in one pass with a
**sharp-bilinear** shader: nearest-neighbour to the integer multiple above the target (2x for 720p, 3x for
1080p), then bilinear down. Pixels stay crisp, only pixel edges get one blended column, and every pixel ends
up the same size. This fixes today's uneven 1.5x `NEAREST`, which is roadmap M3 anyway. The FBO also gives
us the ambient side fill, harness screenshots at logical resolution, and fades without touching game code.
Cost on Tegra X1: one 854x480 RGBA target plus a full-screen quad, negligible *(guess, but standard)*.

Bilinear-only presentation is softer and plain nearest has uneven pixels; both are worse than sharp-bilinear.

### 2.3 Docked 1080p

Today the surface is always 1280x720 and the system upscales when docked, so the image is resampled twice
(1.5x by us, 1.5x by the display pipeline). Better: on docked mode (`appletGetOperationMode`) resize the
`NWindow` to 1920x1080 and present the FBO at 2.25x directly. That needs `UpdateViewport` to stop
hard-coding 1280x720 (`GLInterface.cpp:1127`), the EGL surface to be recreated or the window resized on mode
change, and the stick cursor clamp to read the current output size (insaniquarium_nx `INTEGRATION.md`
"Resolution" warns about exactly this). Optional and independent of the rest.

## 3. Filling the extra width without regressions

All generated images are built in the existing load-time "derived images" step (`WinFishApp.cpp:540-600`
already builds creases, tank masks and pet icons there) from the player's own files, kept in memory, and at
most cached on the SD card next to the game data. Nothing goes in the repo.

### 3.1 What the art looks like (checked against the real files)

- **Tank backdrops** (`aquarium1..6.jpg`, 640x480): each paints the *inside of a glass tank*. There is a
  translucent glass wall about 30 px wide along both sides (lighter band with a highlight line), water
  gradient and caustics on top, a sand/gravel floor, and big decorations that touch or cross the glass bands
  (coral at x 20..130 and seaweed at 480..620 in tank 1; a pillar at the left of tank 2; a castle from x 20
  in tank 3; a bonsai on the left and raked sand in tank 4; a treasure chest against the right wall in tank
  5; tank 6 is a symmetric temple face spanning the whole width). Plain mirroring would double the glass
  wall and produce mirror-image coral.
- **HUD bars** (`menubar.gif`, `Trophybar.gif`, 640x75 + alpha): riveted brass plate in segments, seven
  slot domes at a 72-74 px pitch joined by pipes, a vertical pipe entering at the far left, menu and money
  boxes at the right, a soft shadow on the bottom 7 rows (alpha).
- **Menu art**: title, selector and store are full-bleed paintings with content at the edges. The hatch
  screen is brass panels whose pipe frames and fittings are cut by the image edge. `screenback.gif`
  (240x240) is the same brass-panel style as a 9-slice.
- `DrawImageBox` (`Graphics.cpp:1016-1060`) **tiles** its middle slices (it never stretches them), so any
  9-slice can be drawn wider pixel-exactly. The wave cel is already tiled side by side
  (`WinFishCommon.cpp:124-132` draws it four times from x = 0), and the light band wraps at 640, so both are
  seamless repeats by design.

### 3.2 Per component

| Component | How | Why it won't look worse | Confidence |
|---|---|---|---|
| SCREENBACK screens (help p0, pets, virtual tank fish, tank select, sim setup) | widen the `DrawImageBox` rect to the canvas, `mClip = false` | tiles of the original art, as designed | sure |
| Help/high-score stripes, interlude/story fill | `FillRect` over the canvas | solid colour / horizontal stripes have no horizontal detail | sure |
| Waves (tank and menus) | WAVESIDE caps at the canvas edges, WAVE cels tiled between, last one clipped (additive blend, so no overlap) | the cel already tiles with itself in the original | sure |
| Bubbles | wider `mBubbleBounds` | particles; same sprites | sure |
| Tank light bands | extra copies at `pos + 640` | the image is a 640 period, already wrapped | sure |
| Title, selector, store, hatch/bonus | **ambient fill**: the design area blurred heavily (downsample to ~1/16, bilinear up), darkened ~40%, drawn into the margins every frame | the 4:3 art is untouched; sides only carry colour and light, like a bezel | sure (tune strength) |
| Hatch/bonus alternative | one SCREENBACK 9-slice panel per side, on the screen's blue base colour | tiles of an existing panel in the same brass style | **prototype**: the cut pipe fittings at the image edge may look like they lead nowhere |
| Tank backdrop, widescreen view (b) | extension built from the backdrop's own edge (mirror plus feathered seam is fine here), then a strong blur and darkening; the original glass walls stay at x = 0 and 640 | the playfield is untouched and sharp. Outside the glass reads as "out of focus", and blur hides any mirroring | fairly sure, **prototype** to tune blur radius, darkness and whether the light bands continue into the wings |
| Tank mask | generated from the extended backdrop and an extended `_maskN` alpha (extension area = plain floor) | same construction as the original | follows the backdrop |
| HUD wings, H1 "module repeat" | cut one dome-plus-pipe module (one slot pitch) from the bar and repeat it outwards; the outermost runs off the screen edge; domes in the wings drawn empty/dimmed | pixel-exact tiles of the bar | **prototype**: the cut must land on matching plate seams and rivets; empty domes may read as locked slots |
| HUD wings, H2 "plate" | plain plate synthesized from the bar's own plain columns (top rows above the domes are plain plate across all 640 px) plus rivets and seam lines copied from the bar at its own spacing | only copied pixels, no scaling | **prototype**: may look flatter than the textured original |
| HUD wings, H3 "end caps" | bar stays 640, both ends get a procedural bevel and drop shadow, backdrop visible beside it | nothing of the original changes | **prototype**: a floating bar may look unfinished |
| Tank backdrop, wide tank (c) | section 3.3 | | **prototype per backdrop** |
| Glass walls, wide tank (c) | procedural: per-backdrop tint sampled from the original band (band colour minus neighbour colour, per row band), drawn as a vertical gradient with a highlight line at the new edges | matches the original's look without copying a band that has decorations baked into it | **prototype** |

The ambient fill is generic: it works for any screen, dialog or fade without per-screen code, so it is the
default for every screen that doesn't draw its own wide background. That keeps the long tail of PvZ-style
per-screen fixes optional.

### 3.3 Sharp tank extension for the wide tank (the hard part)

The wide tank needs 107 new px per side, plus about 32 px per side to replace the old glass bands (they
have decorations baked in, so they can't simply move). So about 139 px of new interior per side.

Ruled out:

- **mirror** (decorations get a mirror twin at the seam)
- **edge-column stretch** (streaks)
- **scaling** (blurry, and changes the art's proportions)

Proposed: **recipe-guided image quilting.**

1. Per backdrop, a small hand-written recipe (our own data, a dozen numbers): the y ranges of the layers
   (water, decoration band, floor), and source rectangles that contain *no decorations* for each layer.
   Example for tank 1 *(guess, to be measured)*: water and floor from x 140..300.
2. At load, fill each layer of the extension with patch quilting (Efros and Freeman: overlapping patches
   from the source rectangles, joined along a minimum-error cut). Water and sand are texture-like, which is
   where quilting does well. Deterministic seed, so the result is the same every boot.
3. Blend into the original interior over a 16-24 px seam, then draw the procedural glass walls at the new
   edges. Light bands, waves, mask and bubbles extend as in 3.2.
4. Cache the result (and the extended mask) on the SD card, keyed by a hash of the source files.

Per backdrop *(judged by eye from the files)*:

| Backdrop | Edges | Expected |
|---|---|---|
| 1 blue, coral/helmet | water with light rays, plain sand; coral and seaweed cross the glass bands | good; risk where coral is cut at the old band |
| 2 pink ruins | pillar at the left edge, rocks right; smooth floor | medium |
| 3 castle | castle from x 20, rocks to the right edge; gravel floor (quilts well) | medium |
| 4 zen garden | bonsai at the left edge; raked sand has curved patterns that quilting will break | low, may need a plain-sand recipe that hides the rake lines |
| 5 shipwreck | dark, rocks left, chest against the right wall | medium (dark art hides seams) |
| 6 boss temple | symmetric architecture across the whole width, no texture to quilt | low; fallback: the temple fades into a dark purple vignette at the sides (procedural, fits the mood) |

Things that were cut off at the image edge (coral, chest) will end at the blend seam. Whether that reads as
natural is exactly what the prototype has to answer. If a backdrop fails, the fallback for that tank is the
(b) treatment with the playfield still wide (blurred wings behind sharp fish). That looks odd, so the
alternative fallback is to not offer the wide tank on that backdrop.

## 4. Simulation impact

### 4.1 Keeping 4:3 bit-identical

- One runtime value pair, `TankLeft()` and `TankRight()` (0/640 by default, -107/747 for the wide tank),
  in `WinFishCommon.h`, set when a level or the virtual tank starts and never changed mid-level.
- Each site in 1.2 A-C is rewritten so it *evaluates to the same number* at 0/640: `mXMax = 540` becomes
  `TankRight() - 100`, `Next() % 520 + 20` becomes `Next() % (TankWidth() - 120) + TankLeft() + 20`,
  `mXD > 490.0` becomes `mXD > TankRight() - 150.0`. Integer and double results are identical, so are the
  RNG call counts. Runtime rather than compile time, because it is a setting and we want one binary; the
  cost is a load per check *(negligible)*.
- **RNG discipline:** `Rand()` and `mApp->mSeed` are both simulation RNGs (`Rand()` feeds fish hunger and
  coin timers *and* bubbles). Nothing new may draw from them. Cosmetic extras (wing animation, quilting)
  use their own generator. Wider bubble bounds and the starfield only change the modulus, not the number of
  calls, so (b) stays identical to 4:3 too.
- The light band positions keep `% 640` and the 640 wrap even in the wide tank (the image period), so that
  RNG use doesn't depend on width at all.

### 4.2 What changes in a wide tank

- **Fish:** swim range grows from 530 to 744 px (+40%). Fish spread wider, so food has to travel further to
  reach the nearest fish and more pellets reach the floor. *(guess: the economy gets harder, because food
  costs money and the food limit (pellets on screen) stays the same)*
- **Coins:** drop where the fish are and sink, so the cursor travels more. With the stick that is slower,
  with touch it barely matters *(guess)*. Collection timing is unchanged (the fly-to-counter X is cosmetic,
  1.2 E).
- **Aliens:** spawn range and clamps widen. They chase the nearest fish, so with fish spread out they cover
  more ground between kills *(guess: slightly easier)*, but the player also has to chase them further to
  shoot. Missiles must use the new bounds for the "gone off screen" removal, or they vanish mid-screen
  (`Missle.cpp:130`).
- **Pets:** bottom crawlers (Stinky, Rufus, Rhubarb) cover a smaller share of the floor at the same speed,
  so they're weaker coin collectors. Clyde, Zorf (feeds), Itchy (fights) and the rest just get more room.
  Niko and the bubbulator stay on their decoration (design-space anchors). Amp's `mXMax = 460` becomes
  `TankRight() - 180`.
- **Food zone:** clicks across the new width drop food (four input sites).
- We don't rebalance (no hidden multipliers). The harness measures the difference (section 6) and the
  option says it changes gameplay. If it turns out much harder, the first lever to look at is the food
  limit in wide mode *(guess)*.

### 4.3 Saves

The save format stays as it is (compatible with the original files). After `LoadGame` and after loading
the virtual tank, a fix-up sets `mXMin/mXMax` and the Bilaterus caps (1.2 D) from the current tank bounds
(derived from each class's original margin) and clamps positions. A wide mid-level save continued in 4:3
just pulls the fish inside; a 4:3 save continued wide works unchanged. The tank-size setting is greyed
out during a level.

## 5. Architecture

### 5.1 Framework

- `GLInterface` gets a canvas size separate from the app's design size: `mWidth` = canvas width (854 or
  640, from 2.1), `mHeight` = 480. The app keeps `mWidth = 640`. `InitGLInterface` then produces
  `mScreenBounds.mX = -107` and the existing WidgetManager translate and `RemapMouse` centre everything.
  Set `mWidescreenAware = true` to document it; the rest is already there.
- Rendering goes to an FBO of canvas size; `Flush` presents it with the sharp-bilinear pass into a viewport
  that fits the canvas aspect (`UpdateViewport` stops assuming 4:3 and 1280x720).
- **Ambient margins:** the app tells the framework per frame whether the current screen paints the full
  canvas (`SetFullCanvas(bool)`). If not, the framework draws the blurred, darkened design area into the
  margins before presenting. Classic 4:3 with black bars is canvas width 640, which is today's behaviour.
- `SexyAppBase::GetCanvasRect()` returns the canvas in design coordinates (`Rect(-107, 0, 854, 480)` or
  `Rect(0, 0, 640, 480)`) for code that fills the screen.
- **Input:** touch and mouse already go through `RemapMouse` and arrive in design coordinates, so nothing
  changes there. The stick cursor is clamped to the canvas when the screen is full-canvas, and to the
  design rect otherwise (insaniquarium_nx found that a cursor in a dead margin "reads as a freeze").

There's no anchor system and no per-widget layout flags. The design-space convention makes them
unnecessary: the HUD is centred because design space is centred, and the wide Board reaches the edges by
drawing at negative x.

### 5.2 Game

- Screens that paint natively wide (3.2): `mClip = false` plus `GetCanvasRect()` in their background draw,
  and `SetFullCanvas(true)`. Two or three lines per screen.
- Board in both wide modes: `mClip = false`, background, waves, light bands and HUD wings drawn over
  `GetCanvasRect()`. In the wide tank also `mMouseInsets.mLeft = mRight = -107` so clicks in the margins
  reach the Board, with no change to its local coordinates (hit test uses `GetInsetRect()`). Game objects
  are top-level widgets in design coordinates, so negative `mX` just works. In (b) the playfield never
  leaves 0..640.
- Tank bounds via `TankLeft()/TankRight()` (4.1). `PetDialog`/`FoodDialog` take `GetCanvasRect()` in the
  wide tank.
- Generated art (wide backdrops, masks, wing images) is built in the loading thread next to the existing
  mask generation.

Every `game/` change is opt-in at runtime and evaluates to the original numbers when the setting is off,
per the CLAUDE.md rule for decomp code.

## 6. Phased plan

Effort: S = under a day, M = 2-5 days, L = 1-3 weeks *(all guesses)*.

| # | Phase | Effort | Verified by |
|---|---|---|---|
| 0 | Prerequisites: FBO present with sharp-bilinear (M3), harness with per-tick state hash and FBO screenshots (M2) | M | screenshots match the current renderer except for scaling; hash stable across runs |
| 1 | Canvas: GL canvas size, negative `mScreenBounds`, viewport fit, stick clamp, ambient margins, Screen setting (Classic black / Classic soft / Widescreen) | M | **4:3 hash trace identical** to the pre-change build on the M2 plans; touch/stick hit tests at the margins on hardware; screenshots of every screen |
| 2 | Native wide backgrounds: SCREENBACK screens, stripes, solid fills, waves, bubbles | S-M | screenshot review per screen; same plans pass (coordinates unchanged) |
| 3 | Widescreen view (b): blurred backdrop wings, tank mask, light bands and waves across, HUD wing prototypes H1/H2/H3 side by side | M | **hash trace identical to 4:3** (it is presentation only); screenshot A/B review of wing variants; FPS on hardware |
| 4 | Wide tank sim (c): `TankLeft/Right` through the ~95 lines, Board insets, missile removal, dialogs, save fix-up | M | 4:3 hash still identical; wide-tank plans (spawn, feed at the far edges, coin pickup at x < 0, alien chase) pass; mid-level save round trips in both directions |
| 5 | Sharp backdrop extension: quilting, recipes for 6 tanks, procedural glass walls, SD cache | L | per-backdrop screenshot review against the original at 100% and 200%; decide per tank whether it ships |
| 6 | Balance pass for the wide tank | M | bot plans play tank 1-1..1-5 in both sizes; compare time to finish, money curve, fish deaths, missed food |
| 7 | Native 1080p docked | S-M | docked screenshots via sys-botbase, cursor clamp after dock/undock |
| 8 | Optional: hatch/bonus side panels, other per-screen niceties | S each | screenshot A/B against ambient |

Phases 1-3 deliver a full-screen game with an unchanged simulation, which is already more than any other
port has. Phase 4 can land without phase 5 behind a hidden setting, so the gameplay work isn't blocked on
the art.

**Harness specifics:**

- Plans use design coordinates, which don't change between modes, so one plan runs in all of them. Wide-
  tank plans additionally click at x < 0 and x > 640.
- The determinism check records a per-tick hash of every object's `mXD/mYD`, money, level state and both RNG
  states. Classic and Widescreen view must match the reference build tick for tick; the wide tank only has
  to finish and not get stuck.
- Screenshots are read back from the FBO at 854x480 and compared against approved images with a small
  tolerance (shader rounding differs between mesa and the host GPU).

## 7. Risks

| Risk | Mitigation |
|---|---|
| Sharp backdrop extension can't meet the no-regression bar for some tanks (4 and 6 most likely) | per-tank decision; (b) is always available; wide tank only on tanks that pass review |
| HUD wings look worse than black margins | three variants prototyped; H3 changes nothing of the original |
| A converted bound changes 4:3 behaviour by accident | per-site conversion that keeps the same arithmetic, hash trace on every plan, review the diff against the 1.2 list |
| Something "off screen" becomes visible in the wide tank (missiles, entries) | listed in 1.2 A and 1.6; clip stays on for centred screens; plans that look at the margins |
| New cosmetic code consumes `Rand()`/`mSeed` | separate RNG; hash trace catches it immediately |
| EGL surface resize on dock/undock is fragile | phase 7 is optional; 720p plus system upscale still works |
| Blur/FBO cost on Switch | tiny targets (1/16 size); measure in phase 1 |
| Saves moved between modes | load fix-up (4.3); setting locked during a level |

## 8. Recommendation

Use PopCap's own widescreen-aware convention that the framework still carries: keep the 640x480 design
space, make the GL canvas 854x480, and let the existing `mScreenBounds` translate centre everything. Render
through an FBO with sharp-bilinear scaling. That gives a full-screen picture with almost no game changes.
Screens with 9-slice or procedural backgrounds go natively wide, and every other screen gets an ambient
blur at the sides.

Ship **Widescreen view** first and make it the default. The tank backdrop and HUD reach the screen edges,
and fish stay in the original tank, so the simulation is identical to 4:3 and the harness proves it. Then
add **Wide tank** as an opt-in that changes gameplay: about 95 bound and spawn sites move to a runtime
`TankLeft/TankRight` that evaluates to the original numbers in 4:3, with sharp backdrop extension by
recipe-guided quilting that ships only for the tanks that pass screenshot review. Classic 4:3 stays one
setting away, with black bars or soft sides, and stays bit-identical.

## Notes: sources

- `upstream/headshot-re-pvz/SexyAppFramework/graphics/unused/DDInterface.cpp`: PopCap's widescreen-aware
  presentation logic.
- PvZWidescreen (HenryJk), https://github.com/HenryJk/PvZWidescreen: 1066x600, PAD = 133, separate
  patches for almanac, store, credits, title, zen garden, cursor; issues #19, #21-24, #38, #40 (Zomboni
  speed changed by a reused 400 constant), fog moved because the screen edge was a mechanic.
- PvZ-Portable (wszqkzqk) keeps 4:3 by policy (issues #437, #520, #531, #533).
- PvZ: Replanted keeps gameplay in 4:3 and extends the backgrounds (PC Gamer review; not checked first hand).
- Diablo II Resurrected removed ultrawide because visibility was a gameplay rule (PC Gamer).
- OpenInsaniquarium `ROADMAP`: "resolution and widescreen, tank size" as unstarted opt-ins;
  `docs/re/sim/board.md` lines 231-242 for the light band and tank mask details used above.
- insaniquarium_nx `platform/INTEGRATION.md` "Letterboxing" and "Resolution": 960x720 pillarbox, cursor in
  the bars, 1080p warning.
- Several pages (PCGamingWiki, WSGF, TCRF) refused automated access; nothing here depends on them.
