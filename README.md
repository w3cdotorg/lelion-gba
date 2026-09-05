# LeLion GBA

A Game Boy Advance port of [LeLion](https://github.com/w3cdotorg/LeLion): a lion paints the
town by puking a rainbow while dodging a flying saucer and a ladybug. Homebrew ROM written in C
with [libtonc](https://github.com/devkitPro/libtonc), built with devkitARM.

This is a from-scratch rewrite that keeps the game design and re-scales everything to a
240×160 screen: a 480 px wide town that scrolls with the lion, a 32×32 lion, a bitmap paint
layer in mode 4, and a chiptune soundtrack on the Game Boy sound channels.

## What's in the ROM

- Three levels (Skyline, Metropolis, Village), three difficulties (Easy: 3 hearts and heart
  pickups, paint 85 %; Normal: 3 hearts, 90 %; Hard: 1 heart, 95 %), and an Arcade mode that
  chains the nine stages with a total time.
- The rainbow jet paints the scrolling town; progress is measured on real coverage, with the
  finish line marked on the HUD bar.
- Flying saucer, ladybug, and in the Village the giant painter who walks to the centre of the
  town and back, switching sides.
- READY? VOMIT! intro, pause, CONTINUE? countdown, end-of-level summary with FLAWLESS! and
  NEW BEST!, best times saved to the cartridge SRAM along with your settings.
- Chiptune on the Game Boy channels (melody, arpeggio, bass, drums; the arpeggio and melody
  join in as you paint), boss theme in the Village, PCM sound effects on DirectSound.

| Action | Keys |
|---|---|
| Move | D-pad |
| Puke | A |
| Pause / resume | Start |
| Quit to title (paused) | Select |
| Menus | D-pad to choose, Start or A to confirm, on the summary A goes to the next level, R replays it |

## Building

No local toolchain is required: the build runs inside the official devkitPro Docker image.

```sh
make docker        # produces lelion.gba (release)
make docker-debug  # produces lelion-debug.gba, with the test hooks compiled in
```

With devkitARM installed locally (`DEVKITARM` set), plain `make` works too.

## Running

Download `lelion.gba` from the [releases](https://github.com/w3cdotorg/lelion-gba/releases) or build
it. Any GBA emulator runs it; [mGBA](https://mgba.io) is the reference here. On real hardware,
copy the ROM to a flash cartridge (SRAM save type).

## Project layout

```
src/               C sources (game logic, rendering, audio)
assets/            source art and music; assets/generated holds the C arrays produced by tools/
tools/             Python converters (sprites, skylines, music) — no external dependencies
tests/             headless checks: harness.c (libmgba) runs the ROM, presses keys, reads memory, takes captures;
                   scripts/*.txt are the scenarios, run.sh runs them all
```

## Tests

```sh
tests/run.sh            # needs libmgba (brew install mgba / apt install libmgba-dev)
```

Scenarios run against `lelion-debug.gba`; a last check makes sure the release ROM ignores the hooks.

The game exposes a small debug block at `0x02030000` (lion position, camera, progress, lives,
state, CPU load as the scanline reached when the frame's work is done...) that the scripts assert
on; in the debug build only, `cheat_win` and `cheat_colors` in that block let a test force a
victory or a colour count. The release ROM does not read them.

See `PLAN.md` for the porting plan and progress.
