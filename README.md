# LeLion GBA

A Game Boy Advance port of [LeLion](https://github.com/w3cdotorg/LeLion): a lion paints the
town by puking a rainbow while dodging a flying saucer and a ladybug. Homebrew ROM written in C
with [libtonc](https://github.com/devkitPro/libtonc), built with devkitARM.

This is a from-scratch rewrite that keeps the game design and re-scales everything to a
240×160 screen: a 480 px wide town that scrolls with the lion, a 32×32 lion, a bitmap paint
layer in mode 4, and a chiptune soundtrack on the Game Boy sound channels.

## Building

No local toolchain is required: the build runs inside the official devkitPro Docker image.

```sh
make docker        # produces lelion.gba
```

With devkitARM installed locally (`DEVKITARM` set), plain `make` works too.

## Running

Any GBA emulator runs `lelion.gba`; [mGBA](https://mgba.io) is the reference here. On real
hardware, copy the ROM to a flash cartridge.

## Project layout

```
src/               C sources (game logic, rendering, audio)
assets/            source art and music; assets/generated holds the C arrays produced by tools/
tools/             Python converters (sprites, skylines, music) — no external dependencies
tests/             headless emulator checks
```

See `PLAN.md` for the porting plan and progress.
