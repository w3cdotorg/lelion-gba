# Porting plan

Target: the same game as the Godot version, on a 240×160 screen.

| Godot | GBA |
|---|---|
| 2000×648 single screen | 480×72 town, horizontal scrolling camera |
| 128 px lion | 32×32 sprite |
| Jet lands 136 px ahead, 190 px down | 35 px ahead, 45 px down |
| Paint mask 2000×241 + shader | Mode 4 bitmap (8-bit paletted), painted directly |
| Coverage per 8 px cell | Coverage per 4 px cell |
| Saucer 128×54, ladybug 77×62 | 32×16, 16×16 sprites |
| Boss 65 % of screen height | 104 px tall, composite of 64×64 + 32×64 sprites |
| GPU particles | ~10 drop sprites, rainbow gradient |
| Layered WAV chiptune | Sequencer on the Game Boy channels (2 squares, wave, noise) |
| IndexedDB / ConfigFile | 32 KB SRAM |
| Mouse / touch / keyboard | D-pad, A (puke), B, Start (pause), L/R |

## Phases

- [x] 0. Toolchain: Docker build, mode 4 hello ROM, headless emulator check
- [x] 1. Scrolling town: skyline bitmap, camera, lion sprite and movement
- [x] 2. Painting: rainbow jet, paint into the bitmap, coverage and progress bar
- [x] 3. Pickups and enemies: color dots, saucer, ladybug, hearts, invulnerability
- [ ] 4. Sound: sequencer on GB channels, town theme, effects
- [ ] 5. Menus: title, difficulty, level, settings, READY? VOMIT!, CONTINUE?, summary
- [ ] 6. Boss level and arcade mode
- [ ] 7. SRAM saves, CI release, ROM playable on the website
