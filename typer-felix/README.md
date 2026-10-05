# Typing with Typer Felix

this is an edutainment game in the style of ones from the early 80s designed to teach kids how to type this game was designed for the Apple IIe (with an 80-column card installed) and the idea is that typer Felix (an orange cat) has hired you as his apprentice and now you must complete tasks around the village while learning how to type.

note: no AI was used to code this game! it was all written in C and then compiled with cc65 and applecommader.

## What is included

- **Home Row Fish Market:** twelve drifting, letter-labelled fish; practice
  `A S D F J K L ;`. A correct key catches the fish. A wrong key or timeout
  loses that fish and plays a soft splash/buzzer.
- **Mouse Chaser:** twelve word-carrying mice and a 24-word pool. Type the word
  before the mouse reaches its hole. Wrong keys leave you on the same letter.
  After each group of four targets, an error rate strictly below 5% raises
  movement speed by 15%, rounded to the nearest whole percent, up to 240%.
- **Morning Gazette:** eight case-sensitive sentences with spaces, numbers,
  punctuation and Shift practice. Accepted sentences scroll through a six-line
  printed edition in full-screen 80-column text.
- Orange artifact-color Felix with idle, paw-swipe, sleepy, typo/ear-scratch
  and typing poses; bitmapped fish, mice and labels.
- Speaker key clicks and short rising/falling tones.
- Accuracy, approximate WPM and session badges. Complete eight of twelve
  catches (or all eight Gazette lines) with at least 80% accuracy for a badge.
  All lessons are available from the menu; the suggested order is 1, 2, 3.

## Hardware

- An Apple IIe, original or enhanced, at **normal 1 MHz speed**.
- 64 KB main RAM. **80-column card required for the Gazette**; a 128 KB IIe
  with an extended 80-column card is recommended.
- NTSC 60 Hz by default. Press `V` at the menu for PAL 50 Hz WPM calculation.
- A color composite display/emulator color mode to see orange artifact color.
  A monochrome monitor still displays the shapes.
- ProDOS 2.4.3 is included in the bootable game disk. No separate boot disk
  or BASIC.SYSTEM is needed.

This version deliberately uses **standard hi-res**, not double hi-res:
280×160 visible graphics plus the four-line text window in arcade modes.
That is one of the rendering options in the brief and avoids auxiliary-memory
graphics requirements. The Gazette is pure 80-column text, with a small
text-character Felix/typewriter animation in the masthead.

## Build

Install cc65, GNU Make and Node.js (Node is only needed for disk packaging).
The required commands are:

```sh
cd native/typer-felix
make
make disk
make check
```

`make` calls:

```sh
cl65 -t apple2 -C apple2-hgr.cfg -Oirs -I src \
  -m build/felix.map -Ln build/felix.lbl -o build/FELIX \
  src/main.c src/video.c src/hardware.c src/lessons.c src/timing.s
```

On a non-Nix installation, set `CC65_HOME` to the cc65 data directory if
the default discovery does not find `target/apple2/util/loader.system`:

```sh
make disk CC65_HOME=/usr/share/cc65
```

Outputs:

| File | Purpose |
| --- | --- |
| `build/FELIX` | AppleSingle executable; includes ProDOS BIN type/load metadata |
| `build/FELIX.bin` | Raw payload, load and entry address **$0803** |
| `build/FELIX.SYSTEM` | cc65 ProDOS launcher, named to load the adjacent `FELIX` |
| `build/felix.po` | 140 KB **bootable ProDOS-order game disk**, distributed as `typer-felix-bootable.po` |
| `build/felix.map`, `build/felix.lbl` | Link map and debugger symbols |

The compiler target is `apple2` rather than `apple2enh` intentionally: the C
runtime emits 6502 instructions, so an **unenhanced IIe** can run it. The
graphics and keyboard code still depends on IIe hardware (not an Apple II+).

## Run in an emulator

Use an Apple IIe-capable emulator, such as AppleWin or MAME, configured for
an Apple IIe with an 80-column card and normal speed.

1. Insert `typer-felix-bootable.po` (or locally built `build/felix.po`) in drive 1.
2. Cold boot / power cycle the emulated Apple IIe.
3. Wait for ProDOS to load; the Felix menu starts automatically.
4. Select `1`, `2` or `3` at the menu.

If you prefer to use this as a second disk after booting your own ProDOS
environment, launch `FELIX.SYSTEM` from its launcher. From BASIC.SYSTEM:

   ```text
   -/TYPER.FELIX/FELIX.SYSTEM
   ```

If using another disk utility instead of the provided image, import `FELIX`
as **AppleSingle**, preserving its BIN type and auxiliary load address $0803,
then add `FELIX.SYSTEM` with ProDOS type SYS ($FF) alongside it. Do not
import the AppleSingle header as part of the raw program. The raw `.bin`
alternative needs type BIN ($06) and aux $0803 explicitly.

No Apple ROM is bundled. The builder copies the boot blocks and operating-system
file unchanged from the official ProDOS 2.4.3 release, verifies the source
disk's SHA-256, and places `FELIX.SYSTEM` first for automatic startup.
It preserves ProDOS file metadata and includes the cc65-provided loader.
See `THIRD_PARTY.md` and `vendor/README.md` for attribution and provenance.

## Controls

Before every lesson, choose **1 Easy**, **2 Medium**, or **3 Challenge**,
then press Return on the instructions screen to start. Escape on either
screen returns to the menu. The chosen difficulty appears during play and
on the lesson report.

Release the level-selection key before pressing a difficulty number. The
difficulty and instruction screens wait for key release so a held key cannot
carry over from the previous screen.

| Difficulty | Fish / initial mouse speed | Mouse words | Gazette |
| --- | --- | --- | --- |
| 1 Easy | 65% | 3 letters | Short lowercase phrases |
| 2 Medium | 100% | 4 letters | Simple sentences and punctuation |
| 3 Challenge | 140% | 5–7 letters | Longer sentences, numbers and symbols |

Mouse speed still adapts upward by 15% for accurate typing. Badge requirements
are the same at every difficulty.

| Where | Keys |
| --- | --- |
| Menu | `1` fish, `2` mice, `3` Gazette, `S` sound, `V` 50/60 Hz, `Q` quit |
| Instructions | `RETURN` start; `ESC` menu |
| During lessons | `ESC` stop and view report, `TAB` pause/resume, `CTRL-S` sound |
| Fish/mice | Case-insensitive target typing; no Return required |
| Gazette | Exact case and punctuation; Return prints a completed line |
| Report | `RETURN` menu |

Turn **CAPS LOCK off** for the Gazette. Incorrect characters are rejected,
not inserted, so Backspace/Delete is not necessary and is ignored. There
is no score-file write or disk access during lessons. Badges reset on quit.

## Source map and implementation

- `src/main.c`: menus, lesson loops, feedback and reports.
- `src/lessons.c`: words, sentences, accuracy/WPM and adaptive-speed arithmetic.
- `src/video.c`: direct text/hi-res rendering, 5×7 font, sprite backing stores.
- `src/hardware.c`: keyboard latch and sound feedback.
- `src/timing.s`: 6502 routines for VBL synchronization, tones and keyboard latch access.
- `tools/make-disk.mjs`: dependency-free AppleSingle-to-ProDOS disk packaging.
- `tools/verify-build.mjs`: static executable-layout and disk-integrity checks.
- `tools/verify-keyboard.mjs`: checks the linked keyboard instructions, including
  the strobe reset that prevents one keypress from selecting two screens.

The game logic and renderer are C. Assembly is restricted to timing-sensitive
hardware operations. The stock `apple2-hgr.cfg` keeps hi-res page 1
($2000–$3FFF) clear of executable code; regular code begins at $4000. The
runtime startup is at $0803. HGR page 2 is **not** available because it holds
code. Sprites save/restore only their byte-aligned rectangles; the scene is
not fully redrawn every frame.

80-column text writes use 80STORE and PAGE2 to select the auxiliary/main
text banks without switching the code, stack, or zero page. Switching back
to the menu resets 80STORE, 80COL, PAGE2 and the graphics soft switches.

WPM is **approximate**, derived from correct keystrokes / five and active
video-wait frames. Pauses, instructions and between-target feedback are
excluded. Rendering or sound that spans a frame can cause undercounting of
elapsed time. This is a training indicator, not a benchmark-grade timer.
The 50/60 Hz option adjusts WPM; PAL movement naturally takes longer in
wall-clock time because movement is frame-based.

## Verification and limits

The native executable is built with cc65. Static checks verify its memory
layout, disk allocation, file metadata, first .SYSTEM entry, and byte-for-byte
agreement of the boot blocks and kernel with the official ProDOS release.
Actual Apple IIe hardware and emulator gameplay have **not** been verified
in this environment.

This release does not implement double hi-res, disk-persistent profiles,
custom text entry, or a music tracker. The speaker sounds are intentionally
short so typing remains responsive.

## game is also available on itch.io for free at this [link](https://doctor-retro-g.itch.io/typing-with-typer-felix)
